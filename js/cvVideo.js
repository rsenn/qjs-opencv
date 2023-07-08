import { BindMethods } from './cvUtils.js';
import { BindMethodsTo } from './cvUtils.js';
import { Define } from './cvUtils.js';
import { FindKey } from './cvUtils.js';
import { Lookup } from './cvUtils.js';
import { Modulo } from './cvUtils.js';
import { WeakAssign } from './cvUtils.js';
import { WeakMapper } from './cvUtils.js';
import { Mat } from 'opencv';
import { Rect } from 'opencv';
import { Size } from 'opencv';
import { VideoCapture } from 'opencv';
import * as cv from 'opencv';

const Crop = (() => {
  const mapper = WeakMapper(() => new Mat());
  return function Crop(mat, rect) {
    let tmp = mapper(mat);
    mat.roi(rect).copyTo(tmp);
    return tmp;
  };
})();

function ImageSize(
  src,
  dst,
  dsize,
  action = (name, arg1, arg2) => console.debug(`${name} ${arg1} -> ${arg2}`)
) {
  let s,
    roi,
    f,
    ssize = src.size;
  if(!ssize.equals(dsize)) {
    let [fx, fy] = dsize.div(ssize);
    if(fx != fy) {
      roi = new Rect(0, 0, ...ssize);
      if(fx < fy) {
        s = dsize.div((f = fy));
        roi.size = s.round();
        roi.x = Math.floor((src.cols - s.width) / 2);
      } else {
        s = dsize.div((f = fx));
        roi.size = s.round();
        roi.y = Math.floor((src.rows - s.height) / 2);
      }
      if(roi.x || roi.y) {
        action('Crop', ssize, roi);
        let cropped = Crop(src, roi);
        src = cropped;
        ssize = src.size;
        [fx, fy] = dsize.div(ssize);
      }
    }
    let aspects = [ssize, dsize].map(s => s.aspect);
    if(!ssize.equals(dsize)) {
      let factors = dsize.div(ssize);
      [fx, fy] = factors;
      factors.sort((a, b) => b - a);
      if(fx != fy) {
        dsize = ssize.mul(factors[0]);
        factors = dsize.div(ssize);
        [fx, fy] = factors;
        dsize = dsize.round();
      }
      action(`Scale (ₓ${factors[0].toFixed(5)})`, ssize, dsize);
      dst.reset();
      cv.resize(src, dst, dsize, 0, 0, cv.INTER_CUBIC);
      dst.resize(dsize.height);
      return;
    }
  }

  console.warn(`copyTo ${src} -> ${dst}`);
  src.copyTo(dst);
}

export class ImageSequence {
  constructor(images = [], dimensions) {
    const imgs = this;
    if(typeof images == 'string') {
      let gen = FilterImages(ReadDirRecursive(images));
      let entries = [...SortFiles(StatFiles(gen), 'ctime')];
      images = entries.map(e => e + '');
    }
    this.images = images;
    this.frame = null;
    this.index = 0;
    this.props = {
      get frame_count() {
        return imgs.images.length;
      },
      fps: 1,
      backend: 'imread',
      get pos_frames() {
        return imgs.index;
      },
      set pos_frames(value) {
        imgs.index = Modulo(value, images.length);
      },
      get pos_msec() {
        const { pos_frames, fps } = this;
        return (pos_frames * 1000) / fps;
      }
    };

    if(!dimensions) {
      let mat = cv.imread(images[0], cv.IMREAD_IGNORE_ORIENTATION);
      dimensions = mat.size;
      const { cols, rows } = mat;
      dimensions = new Size(mat.cols, mat.rows);
    }

    this.set('frame_width', dimensions.width);
    this.set('frame_height', dimensions.height);
  }

  getBackendName() {
    return 'imread';
  }

  isOpened() {
    return true;
  }

  get(prop) {
    return this.props[prop.toLowerCase()];
  }

  set(prop, value) {
    const { props } = this;
    this.props[prop.toLowerCase()] = value;
  }

  get size() {
    return new Size(this.get('frame_width'), this.get('frame_height'));
  }

  grab() {
    const { images, props } = this;
    if(this.index >= images.length) this.index = 0;
    this.framePos = this.index++;
    this.frameFile = images[this.framePos];
    const { index, framePos, frameFile } = this;
    let ret = !!(this.frame = cv.imread(frameFile));
    return ret;
  }

  retrieve(mat) {
    let { framePos, frame, size: targetSize } = this;
    if(mat) {
      let { size: frameSize } = frame;
      let doResize = !frameSize.equals(targetSize);
      if(doResize)
        ImageSize(frame, mat, targetSize, (name, arg1, arg2) =>
          console.debug(
            `ImageSize[${this.framePos}] ${name} ${arg1.toString()} -> ${arg2.toString()}`
          )
        );
      else frame.copyTo(mat);
      return !mat.emtpy;
    }
    return this.frame;
  }

  read(mat) {
    if(this.grab()) return this.retrieve(mat);
  }

  *[Symbol.iterator]() {
    for(let image of this.images) {
      yield cv.imread(image);
    }
  }
}

const isVideoPath = arg =>
  /\.(3gp|avi|f4v|flv|m4v|m2v|mkv|mov|mp4|mpeg|mpg|ogm|vob|webm|wmv)$/i.test(arg);

export class VideoSource {
  static backends = Object.fromEntries(
    [
      'ANY',
      'VFW',
      'V4L',
      'V4L2',
      'FIREWIRE',
      'FIREWARE',
      'IEEE1394',
      'DC1394',
      'CMU1394',
      'QT',
      'UNICAP',
      'DSHOW',
      'PVAPI',
      'OPENNI',
      'OPENNI_ASUS',
      'ANDROID',
      'XIAPI',
      'AVFOUNDATION',
      'GIGANETIX',
      'MSMF',
      'WINRT',
      'INTELPERC',
      'REALSENSE',
      'OPENNI2',
      'OPENNI2_ASUS',
      'GPHOTO2',
      'GSTREAMER',
      'FFMPEG',
      'IMAGES',
      'ARAVIS',
      'OPENCV_MJPEG',
      'INTEL_MFX',
      'XINE'
    ].map(name => [name, cv['CAP_' + name]])
  );

  constructor(...args) {
    if(args.length > 0) {
      let [device, backend = 'ANY', loop = true] = args;
      const driverId = VideoSource.backends[backend];
      let isVideo = (args.length <= 2 && backend in VideoSource.backends) || isVideoPath(device);
      if(isVideo) {
        if(typeof device == 'string' && isVideoPath(device)) {
          if(backend == 'ANY') backend = 'FFMPEG';
        } else {
          if(backend == 'ANY') backend = 'V4L2';
        }
        this.capture(device, driverId);
      } else {
        this.fromImages(args);
      }
      this.isVideo = true;
      this.doLoop = loop;
    }
  }

  props = new Lookup(
    prop => this.cap.get(this.propId(prop)),
    (prop, value) => this.cap.set(this.propId(prop), value),
    () => [
      'pos_msec',
      'pos_frames',
      'pos_avi_ratio',
      'frame_width',
      'frame_height',
      'fps',
      'fourcc',
      'frame_count',
      'format',
      'mode',
      'brightness',
      'contrast',
      'saturation',
      'hue',
      'gain',
      'exposure',
      'convert_rgb',
      'white_balance_blue_u',
      'rectification',
      'monochrome',
      'sharpness',
      'auto_exposure',
      'gamma',
      'temperature',
      'trigger',
      'trigger_delay',
      'white_balance_red_v',
      'zoom',
      'focus',
      'guid',
      'iso_speed',
      ,
      'backlight',
      'pan',
      'tilt',
      'roll',
      'iris',
      'settings',
      'buffersize',
      'autofocus',
      'sar_num',
      'sar_den',
      'backend',
      'channel',
      'auto_wb',
      'wb_temperature',
      'codec_pixel_format'
    ].filter(s => typeof s == 'string').filter(s => /pos_/.test(s) || this.cap.get(this.propId(s)) !== 0)
  );

  capture(device, driverId) {
    let cap = new VideoCapture(device, driverId);
    this.cap = cap;
    this.propId = prop => {
      if(typeof prop == 'string') {
        prop = prop.toUpperCase();
        if(!prop.startsWith('CAP_PROP_')) prop = 'CAP_PROP_' + prop;
        prop = cv[prop];
      }
      return prop;
    };
    this.read = function(mat) {
      const { cap } = this;
      const lastFrame = this.get('CAP_PROP_FRAME_COUNT') - 1;
      if(!mat) mat = new Mat();
      for(;;) {
        const ok = cap.read(mat);
        if(!ok && this.get('CAP_PROP_POS_FRAMES') == lastFrame) {
          if(this.doLoop) {
            this.set('CAP_PROP_POS_FRAMES', 0);
            continue;
          }
        }
        break;
      }
      return mat;
    };
    this.retrieve = function(mat) {
      const { cap } = this;
      if(!mat) mat = new Mat();
      if(cap.retrieve(mat)) return mat;
    };
    WeakAssign(this, BindMethods(this.cap, VideoCapture.prototype));
  }

  fromImages(...args) {
    let cap = new ImageSequence(...args);
    this.cap = cap;
    this.propId = prop => {
      if(typeof prop == 'string') {
        prop = prop.toLowerCase();
        if(prop.startsWith('cap_prop_')) prop = prop.slice(9);
      }
      return prop;
    };
    BindMethodsTo(this, this.cap, ImageSequence.prototype);
    this.read = function(mat) {
      let ret = ImageSequence.prototype.read.call(this, mat);
      if(!ret) return null;
    };
  }

  set size(s) {
    this.set('CAP_PROP_FRAME_WIDTH', s.width);
    this.set('CAP_PROP_FRAME_HEIGHT', s.height);
  }

  get size() {
    return new Size(this.get('CAP_PROP_FRAME_WIDTH'), this.get('CAP_PROP_FRAME_HEIGHT'));
  }

  get(prop) {
    const { cap } = this;
    if(cap && typeof cap.get == 'function') return this.cap.get(this.propId(prop));
  }

  set(prop, value) {
    const { cap } = this;
    if(cap && typeof cap.set == 'function') return this.cap.set(this.propId(prop), value);
  }

  get backend() {
    const { cap } = this;
    if(cap && typeof cap.getBackendName == 'function') return cap.getBackendName();
    if(typeof this.get == 'function') {
      const id = this.get('BACKEND');
      return FindKey(VideoSource.backends, id);
    }
  }

  get fps() {
    return this.get('fps');
  }

  dump(
    props = [
      'frame_count',
      'frame_width',
      'frame_height',
      'fps',
      'format',
      'fourcc',
      'backend',
      'pos_frames',
      'pos_msec'
    ]
  ) {
    return new Map(
      props.map(propName => [propName, this.get(propName)]).filter(([k, v]) => v !== undefined)
    );
  }

  seekFrames(relative) {
    const pos = this.get('pos_frames') + relative;
    this.set('pos_frames', pos);
    return this.get('pos_frames');
  }

  seekMsecs(relative) {
    const msec_per_frame = 1000 / this.fps;
    this.seekFrames(relative / msec_per_frame);
    return this.get('pos_msec');
  }

  get durationMsecs() {
    const msec_per_frame = 1000 / this.fps;
    return +(this.get('frame_count') * msec_per_frame).toFixed(3);
  }

  get tellMsecs() {
    const msec_per_frame = 1000 / this.fps;
    return +(this.get('pos_frames') * msec_per_frame).toFixed(3);
  }

  position(type = 'Frames') {
    if(type.indexOf('rame') != -1)
      return Define([this.get('pos_frames'), this.get('frame_count')], {
        toString() {
          return '#' + this[0] + '/' + this[1];
        }
      });
    if(type.indexOf('ercent') != -1 || type == '%')
      return (this.get('pos_frames') * 100) / this.get('frame_count');
    return Define([this.tellMsecs, this.durationMsecs], {
      toString() {
        return FormatTime(this[0]) + '/' + FormatTime(this[1]);
      }
    });
  }

  get size() {
    let size = new Size(this.get('frame_width'), this.get('frame_height'));
    return size;
  }

  set size(size) {
    size = size instanceof Size ? size : new Size(size);
    this.set('frame_width', size.width);
    this.set('frame_height', size.height);
  }

  get time() {
    let [pos, duration] = this.position('msec');
    return FormatTime(pos);
  }
}

function FormatTime(pos) {
  let ms, s, m, h;
  ms = Modulo(pos, 1000);
  s = pos / 1000;
  m = Math.floor(s / 60);
  s = Modulo(s, 60);
  h = Math.floor(m / 60);
  m = Modulo(m, 60);
  h = Math.floor(h);
  const pad = (i, n, frac) => {
    const s = (frac !== undefined ? i.toFixed(frac) : i) + '';
    const a = s.split('.');
    return '0'.repeat(Math.max(0, n - a[0].length)) + s;
  };
  return pad(h, 2) + ':' + pad(m, 2) + ':' + pad(s, 2, 3);
}

export default VideoSource;