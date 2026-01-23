import { Mat, Point, Line, Size, VideoWriter, CAP_GSTREAMER, CV_8UC3, LINE_AA, VIDEOWRITER_PROP_FRAMEBYTES, drawLine } from 'opencv';

function repeat(n, fn, ...args) {
  for(let i = 0; i < n; i++) fn(...args);
}

function main() {
  const size = new Size(1024, 768);
  const mat = new Mat(size, CV_8UC3);

  const rate = 25;
  const fourcc = VideoWriter.fourcc('avc1');
  const vw = new VideoWriter(scriptArgs[0].replace(/.*\/([^.]*)\..*/g, '$1') + '.mp4', fourcc, rate, size);

  drawLine(mat, new Point(0, size.height), new Point(size.width, 0), [0xdd, 0xdd, 0xdd], 2, LINE_AA);

  repeat(rate, () => vw.write(mat));

  for(const c of [
    [0, 0, 255],
    [0, 255, 0],
    [255, 0, 0],
    [0, 0, 0],
  ]) {
    mat.setTo(c);
    drawLine(mat, new Line(0, 0, ...size), [0xdd, 0xdd, 0xdd], 2, LINE_AA);
    repeat(rate, () => vw.write(mat));

    console.log(vw.get(VIDEOWRITER_PROP_FRAMEBYTES));
  }

  console.log(VIDEOWRITER_PROP_FRAMEBYTES);
  console.log(vw.getBackendName());
  return {};
}

main();
