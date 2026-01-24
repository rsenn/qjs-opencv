import { Mat, Scalar, FONT_HERSHEY_SIMPLEX, createBackgroundSubtractorMOG2, createBackgroundSubtractorKNN, VideoCapture, Point, drawRect, putText, imshow, waitKey, CAP_PROP_POS_FRAMES } from 'opencv';

function main(input, algo = 'MOG2') {
  const pBackSub = algo == 'MOG2' ? createBackgroundSubtractorMOG2() : createBackgroundSubtractorKNN();

  const capture = new VideoCapture(input);

  if(!capture.isOpened())
    // error in opening the video input
    throw new Error('Unable to open: ' + input);

  const frame = new Mat(),
    fgMask = new Mat();

  while(true) {
    capture.read(frame);

    if(frame.empty) break;

    // update the background model
    pBackSub.apply(frame, fgMask);

    // get the frame number and write it on the current frame
    drawRect(frame, new Point(10, 2), new Point(100, 20), Scalar(255, 255, 255), -1);
    const frameNumberString = capture.get(CAP_PROP_POS_FRAMES);

    putText(frame, frameNumberString, new Point(15, 15), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0));

    // show the current frame and the fg masks
    imshow('Frame', frame);
    imshow('FG Mask', fgMask);

    // get the input from the keyboard
    let keyboard = waitKey(30);
    if(keyboard == 'q' || keyboard == 27) break;
  }
  waitKey(-1);
}

main(...scriptArgs.slice(1));
