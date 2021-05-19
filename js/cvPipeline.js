import { Mat } from 'opencv';
import { isObject } from 'util';
import { WeakMapper, Modulo } from './cvUtils.js';

export class Pipeline extends Function {
  constructor(processors = [], callback) {
    let self;
    self = function(mat, end) {
      let processors = [...self.processors.entries()];
      if(typeof mat == 'number') {
        end ??= mat + 1;
        processors = processors.slice(mat, end);
      }
      if(!(mat instanceof Mat)) mat = null;

      for(let [i, processor] of processors) {
        self.currentProcessor = i;
        let args = [mat ?? self.images[i - 1], self.images[i]];
        self.invokeCallback('before', ...args);
        //console.log(`Pipeline \x1b[38;5;112m#${i} \x1b[38;5;32m'${processor.name}'\x1b[m`);
        mat = processor.call(self, ...args);
        self.invokeCallback('after', ...args);
        //console.log(`Pipeline`, { i, mat, isObj: isObject(mat) });

        if(isObject(mat) && mat instanceof Mat) self.images[i] = mat;
        mat = self.images[i];
        if(typeof callback == 'function') callback.call(self, i, self.processors.length);
      }
      // self.currentProcessor = -1;
      return mat;
    };
    processors = processors.map(processor =>
      processor instanceof Processor ? processor : Processor(processor)
    );
    Object.assign(self, {
      processors,
      currentProcessor: -1,
      images: new Array(processors.length),
      callback
    });
    self.times = new Array(processors.length);
    return Object.setPrototypeOf(self, Pipeline.prototype);
  }

  step(direction = 1) {
    let { currentProcessor } = this;

    return this(Modulo(currentProcessor + direction, this.size));
  }

  recalc(up_to) {
    let { currentProcessor } = this;
    up_to ??= currentProcessor;
    console.log(`Pipeline recalc \x1b[38;5;112m#${up_to} \x1b[38;5;32m'${this.names[up_to]}'\x1b[m`
    );

    return this(0, up_to + 1);
  }

  get size() {
    return this.processors.length;
  }
  get names() {
    return this.processors.map((p) => p.functionName ?? p.fnName ?? p.name);
  }

  get processor() {
    return this.processors[this.currentProcessor];
  }

  get current() {
    const { currentProcessor, processors,names}= this;
    return [names[currentProcessor],processors[currentProcessor]];
  }

  *imageEntries() {
    const { size, names, images } = this;
    for(let i = 0; i < size; i++) yield [names[i], images[i]];
  }

  *processorEntries() {
    const { size, names, processors } = this;
    for(let i = 0; i < size; i++) yield [names[i], processors[i]];
  }

  getName(id) {
    id = this.processorIndex(id);
    if(typeof id == 'number') return this.names[id];
  }

  getProcessor(id) {
    id = this.processorIndex(id);
    if(typeof id == 'number') return this.processors[id];
  }

  getImage(id) {
    id = this.processorIndex(id);
    if(typeof id == 'number') return this.images[id];
  }

  processorIndex(id) {
    if(typeof id == 'undefined') id = this.currentProcessor;
    if(typeof id == 'function') id = this.processors.indexOf(id);
    if(typeof id == 'string') id = this.names.indexOf(id);
    return id;
  }

  inputOf(processor) {
    let index = this.processorIndex(processor);
    return this.processors[index].in ?? this.images[index - 1];
  }
  outputOf(processor) {
    let index = this.processorIndex(processor);
    //console.log('outputOf', { index, processor, names: this.names });
    return this.processors[index].out ?? this.images[index];
  }

  *[Symbol.iterator]() {
    for(let i = 0; i < this.processors.length; i++) yield [this.names[i], this.images[i]];
  }

  get cache() {
    return new Map([...this]);
  }

  invokeCallback(name, ...args) {
    if(typeof this[name] == 'function') {
      this[name].call(this, ...args);
    }
  }
}

export function Processor(fn, ...args) {
  let self;
  let mapper = WeakMapper((fn, ...args) => {
    let mat = new Mat();
    // console.log('new Mat(', ...args, ')');
    return mat;
  });

  self = function(src, dst, i) {
    if(dst && mapper.get(self) && dst !== mapper.get(self))
      throw new Error(`Duplicate output Mat for processor '${self.name}`);

    if(dst) mapper.set(self, dst);
    else if(!dst) dst = mapper(self, src?.size, src?.type);

    if(!('in' in self)) self.in = src;
    if(!('out' in self)) self.out = dst;

    fn.call(this, src, dst, ...args);
    return dst;
  };
  Object.assign(self, { fnName: fn.name });
  Object.setPrototypeOf(self, Processor.prototype);
  return self;
}
Object.setPrototypeOf(Processor.prototype, Function.prototype);
Object.assign(Processor.prototype, {
  set functionName(name) {
    this.fnName = name;
  },
  get functionName() {
    return this.fnName;
  }
});
