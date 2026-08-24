import { Mat, Scalar, FONT_HERSHEY_SIMPLEX, createBackgroundSubtractorCNT, createBackgroundSubtractorGMG, createBackgroundSubtractorGSOC, createBackgroundSubtractorKNN, createBackgroundSubtractorLSBP, createBackgroundSubtractorMOG, createBackgroundSubtractorMOG2, VideoCapture, Point, rectangle, putText, imshow, waitKey, CAP_PROP_POS_FRAMES, } from 'opencv';

function main(input, algo = 'MOG2') {
  const pBackSub = {
    CNT: createBackgroundSubtractorCNT,
    GMG: createBackgroundSubtractorGMG,
    GSOC: createBackgroundSubtractorGSOC,
    KNN: createBackgroundSubtractorKNN,
    LSBP: createBackgroundSubtractorLSBP,
    MOG: createBackgroundSubtractorMOG,
    MOG2: createBackgroundSubtractorMOG2,
  }[algo]();

  const capture = new VideoCapture(input);

  if(!capture.isOpened())
    // error in opening the video input
    throw new Error('Unable to open: ' + input);

  const frame = new Mat(),
    fgMask = new Mat(),masked=new Mat();

  while(true) {
    capture.read(frame);

    if(frame.empty) break;

    // update the background model
    pBackSub.apply(frame, fgMask);

    // get the frame number and write it on the current frame
    rectangle(frame, new Point(10, 2), new Point(100, 20), Scalar(255, 255, 255), -1);
    const frameNumberString = capture.get(CAP_PROP_POS_FRAMES);

    putText(frame, frameNumberString, new Point(15, 15), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0));

    masked.clear();
   frame.copyTo(masked, fgMask);

    // show the current frame and the fg masks
    imshow('Frame', frame);
    imshow('Masked', masked);
    imshow('FG Mask', fgMask);

    // get the input from the keyboard
    let keyboard = waitKey(30);
    //console.log('keyboard',keyboard);
    if(keyboard == 'q' || keyboard == 27 || keyboard == 113) break;
  }

  waitKey(-1);
}

main(...scriptArgs.slice(1));
