export const Modulo = (a, b) => ((a % b) + b) % b;

export const WeakMapper = (createFn, map = new WeakMap(), hitFn) => {
  let self = function(obj, ...args) {
    let ret;
    if(map.has(obj)) {
      ret = map.get(obj);
      if(typeof hitFn == 'function') hitFn(obj, ret);
    } else {
      ret = createFn(obj, ...args);
      //if(ret !== undefined)
      map.set(obj, ret);
    }
    return ret;
  };
  self.set = (k, v) => map.set(k, v);
  self.get = k => map.get(k);
  self.map = map;
  return self;
};

export function WeakAssign(...args) {
  let obj = args.shift();
  args.forEach(other => {
    for(let key in other) {
      if(obj[key] === undefined && other[key] !== undefined) obj[key] = other[key];
    }
  });
  return obj;
}

export const GetMethodNames = (obj, depth = 1, start = 0) =>
  Object.getOwnPropertyNames(obj).filter(name => typeof obj[name] == 'function');

export const BindMethods = (obj, methods) => BindMethodsTo({}, obj, methods || obj);

export function BindMethodsTo(dest, obj, methods) {
  if(Array.isArray(methods)) {
    for(let name of methods) if(typeof obj[name] == 'function') dest[name] = obj[name].bind(obj);
    return dest;
  }
  let names = GetMethodNames(methods);

  for(let name of names)
    if(typeof methods[name] == 'function') dest[name] = methods[name].bind(obj);
  return dest;
}

export function FindKey(obj, pred, thisVal) {
  let fn = typeof pred == 'function' ? value : v => v === pred;
  for(let k in obj) if(fn.call(thisVal, obj[k], k)) return k;
}

export const Define = (obj, ...args) => {
  if(typeof args[0] == 'object') {
    const [arg, overwrite = true] = args;
    let adecl = Object.getOwnPropertyDescriptors(arg);
    let odecl = {};
    for(let prop in adecl) {
      if(prop in obj) {
        if(!overwrite) continue;
        else delete obj[prop];
      }
      if(Object.getOwnPropertyDescriptor(obj, prop)) delete odecl[prop];
      else
        odecl[prop] = {
          ...adecl[prop],
          enumerable: false,
          configurable: true,
          writeable: true
        };
    }
    Object.defineProperties(obj, odecl);
    return obj;
  }
  const [key, value, enumerable = false] = args;
  Object.defineProperty(obj, key, {
    enumerable,
    configurable: true,
    writable: true,
    value
  });
  return obj;
};

export const Once = (fn, thisArg, memoFn) => {
  let ran = false;
  let ret;

  return function(...args) {
    if(!ran) {
      ran = true;
      ret = fn.call(thisArg || this, ...args);
    } else if(typeof memoFn == 'function') {
      ret = memoFn(ret);
    }
    return ret;
  };
};

export const GetOpt = (options = {}, args) => {
  let short, long;
  let result = {};
  let positional = (result['@'] = []);
  if(!(options instanceof Array)) options = Object.entries(options);
  const findOpt = arg =>
    options.find(([optname, option]) =>
        (Array.isArray(option) ? option.indexOf(arg) != -1 : false) || arg == optname
    );
  let [, params] = options.find(opt => opt[0] == '@') || [];
  if(typeof params == 'string') params = params.split(',');
  for(let i = 0; i < args.length; i++) {
    const arg = args[i];
    let opt;
    if(arg[0] == '-') {
      let name, value, start, end;
      if(arg[1] == '-') long = true;
      else short = true;
      start = short ? 1 : 2;
      if(short) end = 2;
      else if((end = arg.indexOf('=')) == -1) end = arg.length;
      name = arg.substring(start, end);
      if((opt = findOpt(name))) {
        const [has_arg, handler] = opt[1];
        if(has_arg) {
          if(arg.length > end) value = arg.substring(end + (arg[end] == '='));
          else value = args[++i];
        } else {
          value = true;
        }
        try {
          value = null;
          value = handler(value, result[opt[0]], options, result);
        } catch(e) {}
        result[opt[0]] = value;
        continue;
      }
    }
    if(params.length) {
      const param = params.shift();
      if((opt = findOpt(param))) {
        const [, [, handler]] = opt;
        let value = arg;
        if(typeof handler == 'function') {
          try {
            value = handler(value, result[opt[0]], options, result);
          } catch(e) {}
        }
        const name = opt[0];
        result[opt[0]] = value;
        continue;
      }
    }
    result['@'] = result['@'].concat([arg]);
  }
  return result;
};

export function RoundTo(value, prec) {
  if(!isFinite(value)) return value;
  return Math.round(value * prec) / prec;
}

export function Range(...args) {
  let [start, end, step = 1] = args;
  let ret;
  start /= step;
  end /= step;
  if(start > end) {
    ret = [];
    while(start >= end) ret.push(start--);
  } else {
    ret = Array.from({ length: end - start + 1 }, (v, k) => k + start);
  }
  if(step != 1) ret = ret.map(n => n * step);
  return ret;
}

export const BitsToNames = (flags, map = (name, flag) => name) => {
  const entries = [...Object.entries(flags)];

  return function* (value) {
    for(let [name, flag] of entries)
      if(value & flag && (value & flag) == flag) yield map(name, flag);
  };
};
