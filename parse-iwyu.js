import fs from 'fs';
import path from 'path';
import Console from 'console';

function IsInclude(line) {
  return /^\s*#\s*include/i.test(line);
}
class File {
  constructor(name) {
    const data = fs.readFileSync(name, 'utf-8');
    this.name = name;

    if(typeof data == 'string') Object.defineProperty(this, 'lines', { value: data.split(/\r?\n/g) });
  }

  /* prettier-ignore */ get includes() {
  return  new Map([...this.lines.entries()].filter(([i,l]) => IsInclude(l)));
}

  removeIncludes() {
    this.lines.splice(0, this.lines.length, ...this.lines.filter(l => !IsInclude(l)));
  }

  replaceIncludes(includes) {
    let index = this.lines.findIndex(IsInclude);

    includes = includes.filter(IsInclude);

    if(index != -1) {
      this.removeIncludes();
      this.lines.splice(index, 0, ...includes);
    } else {
      this.lines.unshift(...includes);
    }
  }

  save() {
    let r = fs.writeFileSync(this.name, this.lines.join('\n'));
    console.log(`Saved '${this.name}'...`, r);
  }
}

Object.defineProperty(File.prototype, Symbol.toStringTag, { value: 'File' });

function main(...args) {
  globalThis.console = new Console({
    inspectOptions: { compact: false, colors: true, depth: 3 }
  });
  console.log(`main`, ...args);

  const mydir = path.dirname(args[0]);
  console.log(`mydir`, mydir);
  process.chdir(mydir);
  console.log(`process.cwd()`, process.cwd());

  let data = '\n' + fs.readFileSync('iwyu.txt');
  // console.log(`data`, data);
  let i = 0;
  let paragraphs = data.split(/\n\n+/g).filter(p => !/has correct/.test(p));
  let files = {};

  for(let paragraph of paragraphs) {
    let p = paragraph.trim();
    if(p == '') continue;

    p = p.replace('The full include-list for ', '');

    let lines = p
      .replace(/-\s#include/g, '#include')
      .split(/\n/g)
      .filter(line => typeof line == 'string' && !/^(-+$|\+ )/.test(line))
      .map(line =>
        fs.existsSync(IncludeName(line))
          ? AngleBracketsToQuotes(line)
          : /opencv2|quickjs|cutils/.test(line)
          ? QuotesToAngleBrackets(line)
          : line
      )
      .map(ReplaceStd);

    if(lines.length == 0) continue;

    if(!/:$/.test(lines[0])) throw new Error(`Invalid paragraph: ${lines[0]}`);

    let fields = lines.shift().split(/[\s:]+/g);
    let [name, , what = 'full'] = fields;
    const file = new File(name);
    if(!files[name]) files[name] = {};

    // console.log(`p#${i}`, console.config({ compact: 0 }), { name, what, lines });

    files[name].file = file;
    files[name].includes = file.includes;
    files[name][what] = lines;

    i++;
  }
  console.log('files', console.config({ compact: 0 }), Object.keys(files));

  for(let name in files) {
    const entry = files[name];

    console.log('entry', console.config({ compact: 1 }), entry);
    if(entry.full) {
      const { file, full } = entry;

      file.replaceIncludes(full.map(line => line.replace(/\s*\/\/.*/g, '')));
      file.save();
    }
  }
}

function ReplaceStd(str) {
  const re = /^(.*<)([^>]*)(>.*)|(.*")([^"]*)(".*)^/g;
  let matches = re.exec(str);
  if(matches) {
    let [pre, name, post] = [...matches].slice(1, 4);

    if(
      [
        'assert.h',
        'complex.h',
        'ctype.h',
        'errno.h',
        'fenv.h',
        'inttypes.h',
        'limits.h',
        'locale.h',
        'math.h',
        'setjmp.h',
        'signal.h',
        'stdint.h',
        'stdio.h',
        'stdlib.h',
        'string.h',
        'tgmath.h',
        'time.h',
        'uchar.h',
        'wchar.h',
        'wctype.h'
      ].indexOf(name) != -1
    )
      name = 'c' + path.basename(name, '.h');
    return [pre, name, post].join('');
  }
  return str;
}

function IncludeName(str) {
  return str.replace(/.*["<]([^">]*)[">].*/g, '$1');
}
function QuotesToAngleBrackets(str) {
  return str.replace(/"([^"]*)"/g, '<$1>');
}
function AngleBracketsToQuotes(str) {
  return str.replace(/<([^>]*)>/g, '"$1"');
}

main(process.argv.slice(1));
