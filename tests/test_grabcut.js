import { assert } from 'assert';
import { CV_8UC1, CV_8UC3, Contour, EVENT_FLAG_CTRLKEY, EVENT_FLAG_SHIFTKEY, EVENT_LBUTTONDOWN, EVENT_LBUTTONUP, EVENT_MOUSEMOVE, EVENT_RBUTTONDOWN, EVENT_RBUTTONUP, GC_BGD, GC_FGD, GC_INIT_WITH_MASK, GC_INIT_WITH_RECT, GC_PR_BGD, GC_PR_FGD, IMREAD_COLOR, Mat, Point, Rect, Scalar, WINDOW_AUTOSIZE, addWeighted, copyTo, destroyWindow, drawCircle, drawRect, grabCut, imread, imshow, namedWindow, setMouseCallback, waitKey, } from 'opencv';

const C = console.config({ compact: true });

function help(...argv) {
  console.log('\nThis program demonstrates GrabCut segmentation -- select an object in a region\n' + 'and then grabcut will attempt to segment it out.\n' + 'Call:\n', argv[0], ' <image_name>\n');
  console.log(
    '\nSelect a rectangular area around the object you want to segment\n' +
      '\nHot keys: \n' +
      '\tESC - quit the program\n' +
      '\tr - restore the original image\n' +
      '\tn - next iteration\n' +
      '\n' +
      '\tleft mouse button - set rectangle\n' +
      '\n' +
      '\tCTRL+left mouse button - set GC_BGD pixels\n' +
      '\tSHIFT+left mouse button - set GC_FGD pixels\n' +
      '\n' +
      '\tCTRL+right mouse button - set GC_PR_BGD pixels\n' +
      '\tSHIFT+right mouse button - set GC_PR_FGD pixels\n',
  );
}

const RED = Scalar(0, 0, 255);
const PINK = Scalar(230, 130, 255);
const BLUE = Scalar(255, 0, 0);
const LIGHTBLUE = Scalar(255, 255, 160);
const GREEN = Scalar(0, 255, 0);

const BGD_KEY = EVENT_FLAG_CTRLKEY;
const FGD_KEY = EVENT_FLAG_SHIFTKEY;

function getBinMask(/*const Mat&*/ comMask, /*Mat& */ binMask) {
  if(comMask.empty || comMask.type != CV_8UC1) throw new Error('comMask is empty or has incorrect type (not CV_8UC1)');

  if(binMask.empty || binMask.rows != comMask.rows || binMask.cols != comMask.cols) binMask.create(comMask.size, CV_8UC1);
  binMask = comMask & 1;
}

//const { NOT_SET, IN_PROCESS, SET } = GCApplication;

class GCApplication {
  static NOT_SET = 0;
  static IN_PROCESS = 1;
  static SET = 2;

  static radius = 2;
  static thickness = -1;

  reset() {
    if(!this.mask.empty) this.mask.setTo(Scalar(GC_BGD, GC_BGD, GC_BGD, GC_BGD));

    this.bgdPxls.length = 0;
    this.fgdPxls.length = 0;
    this.prBgdPxls.length = 0;
    this.prFgdPxls.length = 0;

    this.isInitialized = false;
    this.rectState = GCApplication.NOT_SET;
    this.lblsState = GCApplication.NOT_SET;
    this.prLblsState = GCApplication.NOT_SET;
    this.iterCount = 0;
  }

  setImageAndWinName(_image, _winName) {
    if(_image.empty || _winName.empty) return;

    this.image = _image;
    this.winName = _winName;

    this.mask.create(this.image.size, CV_8UC1);
    this.reset();
  }

  showImage() {
    if(this.image.empty || this.winName.empty) return;

    const res = new Mat();
    const binMask = new Mat();
    this.image.copyTo(res);

    if(this.isInitialized) {
      getBinMask(this.mask, binMask);

      const black = new Mat(binMask.rows, binMask.cols, CV_8UC3, Scalar(0, 0, 0));
      black.setTo(Scalar(255, 255, 255), binMask);

      addWeighted(black, 0.5, res, 0.5, 0.0, res);
    }

    for(let pt of this.bgdPxls) drawCircle(res, pt, GCApplication.radius, BLUE, GCApplication.thickness);
    for(let pt of this.fgdPxls) drawCircle(res, pt, GCApplication.radius, RED, GCApplication.thickness);
    for(let pt of this.prBgdPxls) drawCircle(res, pt, GCApplication.radius, LIGHTBLUE, GCApplication.thickness);
    for(let pt of this.prFgdPxls) drawCircle(res, pt, GCApplication.radius, PINK, GCApplication.thickness);

    if(this.rectState == GCApplication.IN_PROCESS || this.rectState == GCApplication.SET)
      drawRect(res, new Point(this.rect.x, this.rect.y), new Point(this.rect.x + this.rect.width, this.rect.y + this.rect.height), GREEN, 2);

    imshow(this.winName, res);
  }

  setRectInMask() {
    const { rect, mask } = this;

    assert(!mask.empty);
    mask.setTo(GC_BGD);
    rect.x = Math.max(0, rect.x);
    rect.y = Math.max(0, rect.y);
    rect.width = Math.min(rect.width, this.image.cols - rect.x);
    rect.height = Math.min(rect.height, this.image.rows - rect.y);
    mask(rect).setTo(Scalar(GC_PR_FGD));
  }

  /**0
   * @param {Number} flags
   * @param {Point} p
   * @param {Boolean} irPr
   */
  setLblsInMask(flags, p, isPr) {
    let bpxls, fpxls;
    let bvalue, fvalue;

    if(!isPr) {
      bpxls = this.bgdPxls;
      fpxls = this.fgdPxls;
      bvalue = GC_BGD;
      fvalue = GC_FGD;
    } else {
      bpxls = this.prBgdPxls;
      fpxls = this.prFgdPxls;
      bvalue = GC_PR_BGD;
      fvalue = GC_PR_FGD;
    }

    if(flags & BGD_KEY) {
      bpxls.push(p);
      drawCircle(this.mask, p, GCApplication.radius, bvalue, GCApplication.thickness);
    }
    
    if(flags & FGD_KEY) {
      fpxls.push(p);
      drawCircle(this.mask, p, GCApplication.radius, fvalue, GCApplication.thickness);
    }
  }

  /**
   * @param {Number} event
   * @param {Number} x
   * @param {Number} y
   * @param {Number} flags
   */
  mouseClick(event, x, y, flags, user) {
    // TODO add bad args check
    switch (event) {
      // set rect or GC_BGD(GC_FGD) labels
      case EVENT_LBUTTONDOWN: {
        let isb = (flags & BGD_KEY) != 0,
          isf = (flags & FGD_KEY) != 0;

        if(this.rectState == GCApplication.NOT_SET && !isb && !isf) {
          this.rectState = GCApplication.IN_PROCESS;
          this.rect = new Rect(x, y, 1, 1);
        }

        if((isb || isf) && this.rectState == GCApplication.SET) this.lblsState = GCApplication.IN_PROCESS;

        break;
      }

      // set GC_PR_BGD(GC_PR_FGD) labels
      case EVENT_RBUTTONDOWN: {
        let isb = (flags & BGD_KEY) != 0,
          isf = (flags & FGD_KEY) != 0;
        if((isb || isf) && this.rectState == GCApplication.SET) this.prLblsState = GCApplication.IN_PROCESS;
        
        break;
      }

      case EVENT_LBUTTONUP: {
        if(this.rectState == GCApplication.IN_PROCESS) {
          if(this.rect.x == x || this.rect.y == y) {
            this.rectState = GCApplication.NOT_SET;
          } else {
            this.rect = new Rect(new Point(this.rect.x, this.rect.y), new Point(x, y));
            this.rectState = GCApplication.SET;
            this.setRectInMask();

            assert(this.bgdPxls.length == 0 && this.fgdPxls.length == 0 && this.prBgdPxls.length == 0 && this.prFgdPxls.length == 0);
          }

          this.showImage();
        }

        if(this.rectState == GCApplication.SET) {
          console.log('rect', C, this.rect);
        }

        if(this.lblsState == GCApplication.IN_PROCESS) {
          this.setLblsInMask(flags, new Point(x, y), false);
          this.lblsState = GCApplication.SET;
          this.nextIter();
          this.showImage();
        } else if(this.rectState == GCApplication.SET) {
          this.nextIter();
          this.showImage();
        }

        break;
      }

      case EVENT_RBUTTONUP: {
        if(this.prLblsState == GCApplication.IN_PROCESS) {
          this.setLblsInMask(flags, new Point(x, y), true);
          this.prLblsState = GCApplication.SET;
        }
        if(this.rectState == GCApplication.SET) {
          this.nextIter();
          this.showImage();
        }
        break;
      }

      case EVENT_MOUSEMOVE: {
        if(this.rectState == GCApplication.IN_PROCESS) {
          this.rect = new Rect(new Point(this.rect.x, this.rect.y), new Point(x, y));
          assert(this.bgdPxls.length == 0 && this.fgdPxls.length == 0 && this.prBgdPxls.length == 0 && this.prFgdPxls.length == 0);
          this.showImage();
        } else if(this.lblsState == GCApplication.IN_PROCESS) {
          this.setLblsInMask(flags, new Point(x, y), false);
          this.showImage();
        } else if(this.prLblsState == GCApplication.IN_PROCESS) {
          this.setLblsInMask(flags, new Point(x, y), true);
          this.showImage();
        }

        break;
      }
    }
  }

  nextIter() {
    const { image, mask, rect, bgdModel, fgdModel } = this;

    if(this.isInitialized) {
      grabCut(image, mask, rect, bgdModel, fgdModel, 1);
    } else {
      if(this.rectState != GCApplication.SET) return this.iterCount;

      if(this.lblsState == GCApplication.SET || this.prLblsState == GCApplication.SET) {
        grabCut(image, mask, rect, bgdModel, fgdModel, 1, GC_INIT_WITH_MASK);
      } else {
        grabCut(image, mask, rect, bgdModel, fgdModel, 1, GC_INIT_WITH_RECT);
      }

      this.isInitialized = true;
    }

    this.iterCount++;

    this.bgdPxls.length = 0;
    this.fgdPxls.length = 0;
    this.prBgdPxls.length = 0;
    this.prFgdPxls.length = 0;

    return this.iterCount;
  }

  /*private:
     setRectInMask();
     setLblsInMask( flags, p, isPr);*/

  winName;
  image;
  mask = new Mat();
  bgdModel = new Mat();
  fgdModel = new Mat();

  rectState = GCApplication.NOT_SET;
  lblsState = GCApplication.NOT_SET;
  prLblsState = GCApplication.NOT_SET;
  isInitialized = false;

  rect;
  fgdPxls = new Contour();
  bgdPxls = new Contour();
  prFgdPxls = new Contour();
  prBgdPxls = new Contour();
  iterCount = 0;
}

const gcapp = new GCApplication();

function on_mouse(event, x, y, flags, param) {
  if(event) console.log('on_mouse', C, { event, x, y, flags });

  gcapp.mouseClick(event, x, y, flags, param);
}

function main(input) {
  let filename = input;

  if(!filename) {
    console.log('\nDurn, empty filename');
    return 1;
  }

  const image = imread(filename, IMREAD_COLOR);

  console.log('image', image);

  if(image.empty) {
    console.log("\n Durn, couldn't read image filename ", filename);
    return 1;
  }

  const winName = 'image';
  namedWindow(winName, WINDOW_AUTOSIZE);
  setMouseCallback(winName, on_mouse, 0);

  gcapp.setImageAndWinName(image, winName);
  gcapp.showImage();

  for(;;) {
    const c = waitKey(1000);

    switch (c) {
      case '\x1b':
      case 113: {
        console.log('Exiting ...');
        destroyWindow(winName);
        return 0;
      }

      case 'r':
      case 114: {
        console.log('RESET');
        gcapp.reset();
        gcapp.showImage();
        break;
      }

      case 'n':
      case 233: {
        const { iterCount } = gcapp;

        console.log('<', iterCount, '... ');

        const newIterCount = gcapp.nextIter();

        if(newIterCount > iterCount) {
          gcapp.showImage();
          console.log(iterCount, '>');
        } else {
          console.log('rect must be determined>');
        }

        break;
      }


      default: {
        console.log('keycode', c);
      }
    }
  }

  destroyWindow(winName);
  return 0;
}

try {
  main(...scriptArgs.slice(1));
} catch(e) {
  console.log('error', e);
}
