import { tests, assert, eq } from './tinytest.js';
import { groundDistance, bearing, azimuthElevation } from '../../js/skywatch/geometry.js';
import { azElToPixel, pixelToAzEl } from '../../js/skywatch/fisheye-projection.js';

function approx(a, b, tol, msg) {
  assert(Math.abs(a - b) <= tol, `${msg}: ${a} vs ${b} (tol ${tol})`);
}

tests({
  'groundDistance: same point is zero'() {
    eq(0, groundDistance(50, 8, 50, 8));
  },

  'groundDistance: one degree of longitude at the equator is ~111.3km'() {
    approx(groundDistance(0, 0, 0, 1), 111319, 200, 'equator degree of longitude');
  },

  'bearing: due east and due north'() {
    approx(bearing(0, 0, 0, 1), 90, 0.5, 'due east');
    approx(bearing(0, 0, 1, 0), 0, 0.5, 'due north');
  },

  'azimuthElevation: aircraft directly overhead is elevation 90'() {
    const camera = { lat: 50, lon: 8, altM: 300 };
    const aircraft = { lat: 50, lon: 8, altM: 10300 };
    const { elevation } = azimuthElevation(camera, aircraft);
    approx(elevation, 90, 0.01, 'directly overhead');
  },

  'azimuthElevation: far aircraft at cruise altitude is near the horizon'() {
    const camera = { lat: 50, lon: 8, altM: 300 };
    // ~500km north, same altitude gain as a typical cruise flight (~10km) -
    // elevation should be small and positive.
    const aircraft = { lat: 54.5, lon: 8, altM: 10300 };
    const { azimuth, elevation } = azimuthElevation(camera, aircraft);
    approx(azimuth, 0, 1, 'due north bearing');
    assert(elevation > 0 && elevation < 5, `expected low elevation, got ${elevation}`);
  },

  'fisheye projection: zenith maps to image center'() {
    const calib = { cx: 500, cy: 400, fx: 300, northOffsetDeg: 0, clockwise: true };
    const p = azElToPixel({ azimuth: 123, elevation: 90 }, calib);
    approx(p.x, 500, 1e-6, 'zenith x');
    approx(p.y, 400, 1e-6, 'zenith y');
  },

  'fisheye projection: north-at-horizon maps along the north-offset axis'() {
    const calib = { cx: 500, cy: 400, fx: 300, northOffsetDeg: 0, clockwise: true };
    const p = azElToPixel({ azimuth: 0, elevation: 0 }, calib);
    approx(p.x, 500, 1e-6, 'north x at horizon');
    approx(p.y, 400 - 300 * (Math.PI / 2), 1e-6, 'north y at horizon');
  },

  'fisheye projection: az/el -> pixel -> az/el round-trips'() {
    const calib = { cx: 512, cy: 384, fx: 280, northOffsetDeg: 37, clockwise: true };
    for (const [azimuth, elevation] of [[10, 80], [90, 45], [200, 10], [350, 60]]) {
      const pixel = azElToPixel({ azimuth, elevation }, calib);
      const back = pixelToAzEl(pixel, calib);
      approx(back.azimuth, azimuth, 1e-6, `azimuth round-trip at az=${azimuth},el=${elevation}`);
      approx(back.elevation, elevation, 1e-6, `elevation round-trip at az=${azimuth},el=${elevation}`);
    }
  },

  'fisheye projection: counter-clockwise mount flips azimuth handedness'() {
    const cw = { cx: 500, cy: 400, fx: 300, northOffsetDeg: 0, clockwise: true };
    const ccw = { cx: 500, cy: 400, fx: 300, northOffsetDeg: 0, clockwise: false };
    const pCw = azElToPixel({ azimuth: 90, elevation: 0 }, cw);
    const pCcw = azElToPixel({ azimuth: 90, elevation: 0 }, ccw);
    approx(pCw.x, -pCcw.x + 1000, 1e-6, 'mirrored around cx');
  },
});
