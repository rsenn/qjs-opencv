// Reports how much of a C++ library's API is actually pulled in by a given
// QuickJS native module (any qjs-* project's <module>.so).
//
// Method: the target module's undefined dynamic symbols are the mangled
// names it *imports* from the candidate libraries. For each candidate
// library (given via --lib / --lib-dir) we list its *exported* mangled
// function symbols (via nm), demangle them (via c++filt) and classify each
// one as either:
//   - a class constructor (mangled name's innermost scope name repeats the
//     class name, e.g. "cv::Mat::Mat(...)") -> the class counts as
//     "implemented" if any of its constructor symbols is imported by the
//     target module.
//   - a free function (does not belong to a class that also exports a
//     constructor or destructor symbol in the same library) -> counts as
//     implemented if that exact mangled symbol is imported.
// Plain (non-constructor) member functions are not scored individually:
// classes exposed only through static factories (e.g. cv::ml::SVM::create())
// have no constructor symbol, so the factory function itself is reported
// under "functions" instead.
//
// Usage:
//   qjs scripts/binding_coverage.js --module=PATH (--lib=PATH | --lib-dir=DIR)... [options]
//
// Options:
//   --module=PATH      path to the QuickJS native module .so to analyze (required)
//   --lib=PATH         a single shared library to scan for exported symbols (repeatable)
//   --lib-dir=DIR      a directory of shared libraries to scan (repeatable)
//   --lib-pattern=RE   regex matching library filenames inside --lib-dir (default: ^lib[^.]+\.so$)
//   --namespace=NAME   restrict classification to this C++ namespace, e.g. "cv" (default: no filter)
//   --json=PATH        JSON report output path (default: ./binding_coverage.json)
//   --out=PATH         human-readable report output path (default: stdout)
//   --verbose          list missing classes/functions even for 0%-bound libraries
//                      (JSON output always contains the full per-symbol lists)

import * as std from 'std';
import * as os from 'os';

function die(msg) {
  std.err.puts(msg + '\n');
  std.exit(1);
}

function parseArgs(argv) {
  const opts = {
    module: null,
    libs: [],
    libDirs: [],
    libPattern: '^lib[^.]+\\.so$',
    namespace: null,
    json: './binding_coverage.json',
    out: null,
    verbose: false,
  };
  for(let i = 1; i < argv.length; i++) {
    const a = argv[i];
    if(a === '--verbose') {
      opts.verbose = true;
      continue;
    }
    const m = /^--([a-z-]+)=(.*)$/.exec(a);
    if(!m) die(`unrecognized argument: ${a}`);
    const key = m[1],
      val = m[2];
    switch(key) {
      case 'module':
        opts.module = val;
        break;
      case 'lib':
        opts.libs.push(val);
        break;
      case 'lib-dir':
        opts.libDirs.push(val);
        break;
      case 'lib-pattern':
        opts.libPattern = val;
        break;
      case 'namespace':
        opts.namespace = val;
        break;
      case 'json':
        opts.json = val;
        break;
      case 'out':
        opts.out = val;
        break;
      default:
        die(`unknown option: --${key}`);
    }
  }
  return opts;
}

function basename(path) {
  const i = path.lastIndexOf('/');
  return i === -1 ? path : path.slice(i + 1);
}

function shQuote(s) {
  return "'" + s.replace(/'/g, "'\\''") + "'";
}

function run(cmd) {
  const f = std.popen(cmd, 'r');
  if(!f) throw new Error('popen failed: ' + cmd);
  const out = f.readAsString();
  const status = f.close();
  if(status !== 0) std.err.puts(`warning: command exited ${status}: ${cmd}\n`);
  return out;
}

// Undefined (imported) mangled symbols of a shared object, via objdump -T.
function getUndefinedSymbols(path) {
  const out = run(`objdump -T ${shQuote(path)}`);
  const set = new Set();
  for(const line of out.split('\n')) {
    if(line.indexOf('*UND*') === -1) continue;
    const parts = line.trim().split(/\s+/);
    const name = parts[parts.length - 1];
    if(name.startsWith('_Z')) set.add(name);
  }
  return set;
}

// Exported mangled function symbols (text symbols only) of a shared object, via nm -D.
function getDefinedFunctionSymbols(path) {
  const out = run(`nm -D --defined-only ${shQuote(path)}`);
  const names = [];
  for(const line of out.split('\n')) {
    const parts = line.trim().split(/\s+/);
    if(parts.length < 3) continue;
    const type = parts[1];
    if(!/^[TtWw]$/.test(type)) continue;
    let name = parts[2];
    const at = name.indexOf('@');
    if(at !== -1) name = name.slice(0, at);
    if(name.startsWith('_Z')) names.push(name);
  }
  return names;
}

// Demangles a batch of mangled names in one c++filt invocation, preserving order.
function demangleAll(names) {
  if(names.length === 0) return [];
  const tmpPath = `/tmp/binding_coverage_${os.getpid()}.txt`;
  const f = std.open(tmpPath, 'w');
  for(const n of names) f.puts(n + '\n');
  f.close();
  const out = run(`c++filt < ${shQuote(tmpPath)}`);
  os.remove(tmpPath);
  const lines = out.split('\n');
  if(lines.length === names.length + 1 && lines[lines.length - 1] === '') lines.pop();
  if(lines.length !== names.length) throw new Error('c++filt output line count mismatch');
  return lines;
}

function stripTrailingQualifiers(s) {
  let changed = true;
  while(changed) {
    changed = false;
    for(const suf of [' const', ' volatile', ' &&', ' &']) {
      if(s.endsWith(suf)) {
        s = s.slice(0, -suf.length);
        changed = true;
      }
    }
  }
  return s;
}

// Finds the '(' that matches the final ')' of s (paren nesting only, no
// angle-bracket ambiguity to worry about since parens are always balanced).
function findMatchingOpenParen(s) {
  let depth = 0;
  for(let i = s.length - 1; i >= 0; i--) {
    const c = s[i];
    if(c === ')') depth++;
    else if(c === '(') {
      depth--;
      if(depth === 0) return i;
    }
  }
  return -1;
}

// Splits on `sep` at angle-bracket depth 0, so template args are not split.
function splitTopLevel(s, sep) {
  const parts = [];
  let depth = 0,
    start = 0;
  for(let i = 0; i < s.length; i++) {
    const c = s[i];
    if(c === '<') depth++;
    else if(c === '>') {
      if(depth > 0) depth--;
    } else if(depth === 0 && s.startsWith(sep, i)) {
      parts.push(s.slice(start, i));
      i += sep.length - 1;
      start = i + 1;
    }
  }
  parts.push(s.slice(start));
  return parts;
}

function baseName(component) {
  const i = component.indexOf('<');
  return i === -1 ? component : component.slice(0, i);
}

// Classifies a c++filt-demangled symbol into constructor / destructor /
// function / other. If `namespace` is given, restricts to names under that
// top-level namespace (e.g. "cv"); otherwise no namespace filtering is done.
function classify(demangled, namespace) {
  const s = stripTrailingQualifiers(demangled.trim());
  if(!s.endsWith(')')) return { kind: 'other' };
  const openIdx = findMatchingOpenParen(s);
  if(openIdx < 0) return { kind: 'other' };
  const declarator = s.slice(0, openIdx);

  // Strip a leading return type, if any (present for template instantiations).
  let depth = 0,
    lastSpace = -1;
  for(let i = 0; i < declarator.length; i++) {
    const c = declarator[i];
    if(c === '<') depth++;
    else if(c === '>') {
      if(depth > 0) depth--;
    } else if(c === ' ' && depth === 0) lastSpace = i;
  }
  const namePath = lastSpace >= 0 ? declarator.slice(lastSpace + 1) : declarator;

  const components = splitTopLevel(namePath, '::');
  if(components.length === 0) return { kind: 'other' };
  if(namespace && components[0] !== namespace) return { kind: 'other' };

  const last = components[components.length - 1];
  if(last.startsWith('~')) {
    return { kind: 'destructor', components, classFullName: components.slice(0, -1).join('::') };
  }
  if(components.length >= 2) {
    const enclosing = baseName(components[components.length - 2]);
    if(enclosing.length > 0 && baseName(last) === enclosing) {
      return { kind: 'constructor', components, classFullName: components.slice(0, -1).join('::') };
    }
  }
  return {
    kind: 'function',
    components,
    enclosingScope: components.length >= 2 ? components.slice(0, -1).join('::') : null,
  };
}

function pct(implemented, total) {
  return total === 0 ? null : Math.round((implemented / total) * 10000) / 100;
}

function buildLibraryReport(libPath, undefinedSet, namespace) {
  const mangledNames = getDefinedFunctionSymbols(libPath);
  const demangledNames = demangleAll(mangledNames);
  const entries = mangledNames.map((m, i) => ({ mangled: m, demangled: demangledNames[i], info: classify(demangledNames[i], namespace) }));

  const knownClasses = new Set();
  for(const e of entries) {
    if(e.info.kind === 'constructor' || e.info.kind === 'destructor') knownClasses.add(e.info.classFullName);
  }

  const classesMap = new Map();
  const functions = [];

  for(const e of entries) {
    if(e.info.kind === 'constructor') {
      const cn = e.info.classFullName;
      if(!classesMap.has(cn)) classesMap.set(cn, { name: cn, constructors: [] });
      classesMap.get(cn).constructors.push({
        mangled: e.mangled,
        demangled: e.demangled,
        implemented: undefinedSet.has(e.mangled),
      });
    } else if(e.info.kind === 'function') {
      if(e.info.enclosingScope !== null && knownClasses.has(e.info.enclosingScope)) continue; // plain member function
      functions.push({
        name: e.info.components.join('::'),
        mangled: e.mangled,
        demangled: e.demangled,
        implemented: undefinedSet.has(e.mangled),
      });
    }
  }

  const classesList = Array.from(classesMap.values())
    .map(c => ({ name: c.name, implemented: c.constructors.some(x => x.implemented), constructors: c.constructors }))
    .sort((a, b) => a.name.localeCompare(b.name));
  functions.sort((a, b) => a.name.localeCompare(b.name) || a.mangled.localeCompare(b.mangled));

  const classesImplemented = classesList.filter(c => c.implemented).length;
  const functionsImplemented = functions.filter(f => f.implemented).length;

  return {
    classes: { total: classesList.length, implemented: classesImplemented, percentage: pct(classesImplemented, classesList.length), list: classesList },
    functions: { total: functions.length, implemented: functionsImplemented, percentage: pct(functionsImplemented, functions.length), list: functions },
    overall: {
      total: classesList.length + functions.length,
      implemented: classesImplemented + functionsImplemented,
      percentage: pct(classesImplemented + functionsImplemented, classesList.length + functions.length),
    },
  };
}

function formatPct(p) {
  return p === null ? 'n/a' : `${p.toFixed(2)}%`;
}

const ANSI_GREEN = '\x1b[32m';
const ANSI_RED = '\x1b[31m';
const ANSI_RESET = '\x1b[0m';

function colorize(text, implemented, color) {
  if(!color) return text;
  return (implemented ? ANSI_GREEN : ANSI_RED) + text + ANSI_RESET;
}

function renderText(report, verbose, color) {
  const lines = [];
  lines.push(`QuickJS native module binding coverage report`);
  lines.push(`generated:   ${report.generatedAt}`);
  lines.push(`module:      ${report.module}`);
  lines.push(`lib sources: ${report.libSources.join(', ')}`);
  if(report.namespace) lines.push(`namespace:   ${report.namespace}`);
  lines.push('');

  const names = Object.keys(report.libraries).sort();
  const w = Math.max(...names.map(n => n.length), 20);
  lines.push(`${'library'.padEnd(w)}  classes            functions          overall`);
  for(const name of names) {
    const lib = report.libraries[name];
    const c = lib.classes,
      fn = lib.functions,
      o = lib.overall;
    lines.push(
      `${name.padEnd(w)}  ${`${c.implemented}/${c.total}`.padEnd(8)} ${formatPct(c.percentage).padStart(7)}  ` +
        `${`${fn.implemented}/${fn.total}`.padEnd(8)} ${formatPct(fn.percentage).padStart(7)}  ` +
        `${`${o.implemented}/${o.total}`.padEnd(8)} ${formatPct(o.percentage).padStart(7)}`,
    );
  }
  lines.push('');
  const s = report.summary;
  lines.push(`TOTAL classes:   ${s.classes.implemented}/${s.classes.total} (${formatPct(s.classes.percentage)})`);
  lines.push(`TOTAL functions: ${s.functions.implemented}/${s.functions.total} (${formatPct(s.functions.percentage)})`);
  lines.push(`TOTAL overall:   ${s.overall.implemented}/${s.overall.total} (${formatPct(s.overall.percentage)})`);

  for(const name of names) {
    const lib = report.libraries[name];
    if(lib.overall.total === 0) continue;
    lines.push('');
    const header = lib.overall.implemented === 0 ? 'not bound' : `bound (${formatPct(lib.overall.percentage)})`;
    lines.push(`--- ${name}: ${header} ---`);
    if(!verbose && lib.overall.implemented === 0) continue;
    if(lib.classes.list.length) {
      lines.push(`  classes (${lib.classes.implemented}/${lib.classes.total}):`);
      for(const c of lib.classes.list) lines.push(`    ${colorize(c.name, c.implemented, color)}`);
    }
    if(lib.functions.list.length) {
      lines.push(`  functions (${lib.functions.implemented}/${lib.functions.total}):`);
      for(const f of lib.functions.list) lines.push(`    ${f.implemented ? ' ' : '*'}${colorize(f.demangled, f.implemented, color)}`);
    }
  }
  return lines.join('\n') + '\n';
}

function main() {
  const opts = parseArgs(scriptArgs);
  if(!opts.module) die('--module=PATH is required (the QuickJS native module .so to analyze)');
  if(opts.libs.length === 0 && opts.libDirs.length === 0) die('specify at least one --lib=PATH or --lib-dir=DIR');

  if(std.loadFile(opts.module) === null) die(`cannot read module: ${opts.module}`);

  const libPattern = new RegExp(opts.libPattern);
  const libPaths = [...opts.libs];
  for(const dir of opts.libDirs) {
    const [entries, err] = os.readdir(dir);
    if(err) die(`cannot read lib dir: ${dir}`);
    for(const name of entries.filter(n => libPattern.test(n)).sort()) libPaths.push(`${dir}/${name}`);
  }
  if(libPaths.length === 0) die(`no libraries found (pattern ${opts.libPattern})`);

  std.err.puts(`scanning module imports: ${opts.module}\n`);
  const undefinedSet = getUndefinedSymbols(opts.module);
  std.err.puts(`  ${undefinedSet.size} imported mangled symbols\n`);

  const report = {
    generatedAt: new Date().toISOString(),
    module: opts.module,
    namespace: opts.namespace,
    libSources: [...opts.libs, ...opts.libDirs.map(d => `${d}/${opts.libPattern}`)],
    libraries: {},
  };

  let sc = 0,
    tc = 0,
    sf = 0,
    tf = 0;
  for(const libPath of libPaths) {
    const name = basename(libPath);
    std.err.puts(`  ${name} ...\n`);
    const libReport = buildLibraryReport(libPath, undefinedSet, opts.namespace);
    report.libraries[name] = libReport;
    sc += libReport.classes.implemented;
    tc += libReport.classes.total;
    sf += libReport.functions.implemented;
    tf += libReport.functions.total;
  }

  report.summary = {
    classes: { total: tc, implemented: sc, percentage: pct(sc, tc) },
    functions: { total: tf, implemented: sf, percentage: pct(sf, tf) },
    overall: { total: tc + tf, implemented: sc + sf, percentage: pct(sc + sf, tc + tf) },
  };

  const jsonOut = std.open(opts.json, 'w');
  jsonOut.puts(JSON.stringify(report, null, 2));
  jsonOut.close();
  std.err.puts(`wrote ${opts.json}\n`);

  const color = opts.out ? false : true; //os.isatty(1);
  const text = renderText(report, opts.verbose, color);
  if(opts.out) {
    const f = std.open(opts.out, 'w');
    f.puts(text);
    f.close();
    std.err.puts(`wrote ${opts.out}\n`);
  } else {
    std.out.puts(text);
  }
}

main();
