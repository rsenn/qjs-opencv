import { Mat, Point, Size, VideoWriter, CAP_GSTREAMER, CV_8UC3, LINE_AA, VIDEOWRITER_PROP_FRAMEBYTES, drawLine } from 'opencv';

function repeat(n, fn, ...args) {
  for(let i = 0; i < n; i++) fn(...args);
}

function main() {
  const frameSize = new Size(1024, 768);
  const mat = new Mat(frameSize, CV_8UC3);
  const fourcc = VideoWriter.fourcc('avc1');
  const vw = new VideoWriter('test.mp4', /* CAP_GSTREAMER,*/ fourcc, 29.7, frameSize);

  drawLine(mat, new Point(0, frameSize.height), new Point(frameSize.width, 0), [0xdd, 0xdd, 0xdd], 2, LINE_AA);

  repeat(30, () => vw.write(mat));

  for(let c of [
    [0, 0, 255],
    [0, 255, 0],
    [255, 0, 0],
    [0, 0, 0],
  ]) {
    mat.setTo(c);
    drawLine(mat, new Point(0, 0), new Point(...frameSize), [0xdd, 0xdd, 0xdd], 2, LINE_AA);
    repeat(30, () => vw.write(mat));

    console.log(vw.get(VIDEOWRITER_PROP_FRAMEBYTES));
  }
  console.log(VIDEOWRITER_PROP_FRAMEBYTES);
  console.log(vw.getBackendName());
  return {};
}

main();
