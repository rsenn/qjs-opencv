import * as cv from "opencv";

function main() {
  const DEG2RAD = Math.PI / 180;

  let center = new cv.Point(100, 50);
  let size = new cv.Size(20, 20);
  let angle = 45 * DEG2RAD;
  console.log("center", center);
  console.log("size", size);
  console.log("angle", angle);
  let rr = new cv.RotatedRect(center, size, angle);
  console.log("rr", rr);
  console.log("rr.center", rr.center);
  console.log("rr.size", rr.size);
  console.log("rr.angle", rr.angle);
  let points;
  rr.points((points = []));
  console.log("rr.points()", console.config({ compact: 1 }), points);

  return {};
}

main();
