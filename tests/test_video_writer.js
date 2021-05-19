import * as cv from "opencv";
import {VideoWriter,Mat,Size} from "opencv";

function repeat(n, fn,...args) {
  for(let i = 0; i < n; i++)
    fn(...args);
}

function main() {
   const frameSize = new Size(640,480);
   const mat = new Mat(frameSize, cv.CV_8UC3);
   const fourcc = VideoWriter.fourcc('M','P','4','2');

const vw=   new VideoWriter('test.avi',cv.CAP_GSTREAMER, fourcc, 29.7, frameSize);

repeat(30, () => vw.write(mat));

for(let c of [[0,0,255],[0,255,0],[255,0,0]]) {
mat.setTo(c);
repeat(30, () => vw.write(mat));
}

  return {};
}

main();
