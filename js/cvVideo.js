import { Mat, VideoCapture, Size, Rect } from 'opencv';
import * as cv from 'opencv';
import { WeakMapper, Modulo, WeakAssign, BindMethods, FindKey } from './cvUtils.js';

const Crop = (() => {
  const mapper = WeakMapper(() => new Mat());
  return function Crop(mat, rect) {
    let tmp = mapper(mat);
    mat.roi(rect).copyTo(tmp);
    return tmp;
  };
})();

function ImageSize(src,
  dst,
  dsize,
  action = (name, arg1, arg2) => console.debug(`${name} ${arg1} -> ${arg2}`)
) {
  let s,
    roi,
    f,
    ssize = src.size;
  //console.debug('ImageSize', { src, dst, ssize, dsize });
  if(!ssize.equals(dsize)) {
    let [fx, fy] = dsize.div(ssize);
    if(fx != fy) {
      roi = new Rect(0, 0, ...ssize);

      //console.warn(`Aspect mismatch ${ssize} -> ${dsize}`);
      if(fx < fy) {
        s = dsize.div((f = fy));

        roi.size = s.round();

        roi.x = Math.floor((src.cols - s.width) / 2);
      } else {
        s = dsize.div((f = fx));
        roi.size = s.round();
        roi.y = Math.floor((src.rows - s.height) / 2);
      }
      //console.debug('VideoSource resize', { ssize, dsize, s, roi });

      if(roi.x || roi.y) {
        action('Crop', ssize, roi);
        let cropped = Crop(src, roi);
        src = cropped;
        ssize = src.size;
        [fx, fy] = dsize.div(ssize);
        //console.debug('VideoSource cropped', src.size, ' -> ', cropped.size, { fx, fy });
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
      //console.debug(`Scale ${src} -> ${dst}`);
      return;
    }
  }
  console.warn(`copyTo ${src} -> ${dst}`);
  src.copyTo(dst);
}

export class ImageSequence {
  constructor(images = [], dimensions) {
    const imgs = this;
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
      } /*,
      get frame_width() {
        if(imgs.frame) return imgs.frame.cols;
      },
      get frame_height() {
        if(imgs.frame) return imgs.frame.rows;
      }*/
    };

    if(!dimensions) {
      let mat = cv.imread(images[0]);
      dimensions = mat.size;
    }
    //console.debug('dimensions', dimensions);
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
    //console.debug('ImageSequence.set', { prop, value, props });
    // if(prop == 'pos_frames') throw new Error(`ImageSequence.set ${prop} = ${value}`);
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
    //console.debug(`ImageSequence.grab[${this.framePos}] ${frameFile}`);

    let ret = !!(this.frame = cv.imread(frameFile));

    return ret;
  }
  retrieve(mat) {
    let { framePos, frame, size: targetSize } = this;
    if(mat) {
      let { size: frameSize } = frame;
      let doResize = !frameSize.equals(targetSize);
      //console.debug(`ImageSequence.retrieve[${framePos}]`, { frame, frameSize, mat, targetSize, doResize });
      if(doResize)
        ImageSize(frame, mat, targetSize, (name, arg1, arg2) =>
          console.debug(`ImageSize[${this.framePos}] ${name} ${arg1.toString()} -> ${arg2.toString()}`
          )
        );
      else frame.copyTo(mat);
      //console.debug(`ImageSequence.retrieve[${framePos}]`, { mat });
      return !mat.emtpy;
    }
    return this.frame;
  }
  read(mat) {
    if(this.grab()) return this.retrieve(mat);
  }
}

const isVideoPath = arg =>
  /\.(3gp|avi|f4v|flv|m4v|m2v|mkv|mov|mp4|mpeg|mpg|ogm|vob|webm|wmv)$/i.test(arg);

export class VideoSource {
  static backends = Object.fromEntries([
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
    console.log('VideoSource.constructor(',
      ...args.reduce((acc, arg) => [...acc, ', ', arg], []),
      ')'
    );
    if(args.length > 0) {
      let [device, backend = 'ANY', loop = true] = args;
      const driverId = VideoSource.backends[backend];
      let isVideo = (args.length <= 2 && backend in VideoSource.backends) || isVideoPath(device);

      // if(cv.imread(args[0])) isVideo = false;
      console.log('VideoSource', { args, backend, driverId, isVideo });

      if(isVideo) {
        if(typeof device == 'string' && isVideoPath(device))
          if(backend == 'ANY') backend = 'FFMPEG';

        //console.debug('VideoSource', { device, backend, driverId, args });

        this.capture(device, driverId);
      } else {
        this.fromImages(args);
      }
      this.isVideo = true;
      this.doLoop = loop;
    }
  }

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
        //if(!ok) mat = null;

        if(!ok && this.get('CAP_PROP_POS_FRAMES') == lastFrame) {
          if(this.doLoop) {
            this.set('CAP_PROP_POS_FRAMES', 0);
            continue;
          }
        }

        //console.debug('VideoSource.read', {cap, mat, framePos: this.get('CAP_PROP_POS_FRAMES'), frameCount: this.get('CAP_PROP_FRAME_COUNT') });
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

    Object.assign(this, BindMethods(this.cap, ImageSequence.prototype));

    // this.size = new Size(1280, 720);

    this.read = function(mat) {
      let ret = ImageSequence.prototype.read.call(this, mat);
      if(!ret) return null;
    };
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
    if(cap && typeof cap.getBackendName == 'function')
      return cap.getBackendName();

    if(typeof this.get == 'function') {
      const id = this.get('BACKEND');
      return FindKey(VideoSource.backends, id);
    }
  }

  get fps() {
    return this.get('fps');
  }

  dump(props = [
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
    return new Map(props.map(propName => [propName, this.get(propName)]).filter(([k, v]) => v !== undefined)
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

  position(type = 'frames') {
    if(type.startsWith('frame')) return [this.get('pos_frames'), this.get('frame_count')];
    if(type.startsWith('percent') || type == '%')
      return (this.get('pos_frames') * 100) / this.get('frame_count');

    return [(+this.get('pos_msec')).toFixed(3), this.durationMsecs];
  }

  get size() {
    let size =  new Size(this.get('frame_width'), this.get('frame_height'));
    //console.debug(`VideoCapture.size = ${size}`);
    return size;
  }

  set size(size) {
    size = size instanceof Size ?  size : new Size(size);
    this.set('frame_width', size.width);
    this.set('frame_height', size.height);
  }

  get time() {
    let [pos, duration] = this.position('msec');

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

    return pad(h, 2) + ':' + pad(m, 2) + ':' + pad(s, 2, 3); //+ '.' + pad(ms, 3);
  }
}

export default VideoSource;
