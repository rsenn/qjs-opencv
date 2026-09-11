/**
 * Equidistant fisheye projection (r = f*theta) between sky-relative
 * azimuth/elevation and image pixel coordinates, for a zenith-mounted
 * (straight up) camera. This is the standard model for consumer/raspi
 * fisheye lenses and is what cv.fisheye.calibrate's focal length maps to
 * at the image center; it degrades for lenses with strong equisolid
 * distortion, in which case fx here should be a locally-fit slope rather
 * than the raw calibrated focal length.
 *
 * calib: {
 *   cx, cy,            // pixel coords of the zenith point (image center)
 *   fx,                // pixels per radian of zenith angle
 *   northOffsetDeg,     // compass azimuth (deg, 0=N) that maps to image +y
 *   clockwise,          // does azimuth increase clockwise in the image?
 *                       // (looking up through the lens flips handedness
 *                       // vs. looking down at a map - depends on mount)
 * }
 */

function toRad(deg) {
  return (deg * Math.PI) / 180;
}

function toDeg(rad) {
  return (rad * 180) / Math.PI;
}

/** azimuth/elevation (degrees) -> { x, y } pixel coordinates. */
export function azElToPixel({ azimuth, elevation }, calib) {
  const zenithAngle = toRad(90 - elevation);
  const r = calib.fx * zenithAngle;
  const rot = toRad(azimuth - calib.northOffsetDeg) * (calib.clockwise ? 1 : -1);
  return {
    x: calib.cx + r * Math.sin(rot),
    y: calib.cy - r * Math.cos(rot),
  };
}

/** { x, y } pixel coordinates -> azimuth/elevation (degrees). */
export function pixelToAzEl({ x, y }, calib) {
  const dx = x - calib.cx, dy = y - calib.cy;
  const r = Math.hypot(dx, dy);
  const rot = Math.atan2(dx, -dy) * (calib.clockwise ? 1 : -1);
  const azimuth = ((toDeg(rot) + calib.northOffsetDeg) % 360 + 360) % 360;
  const elevation = 90 - toDeg(r / calib.fx);
  return { azimuth, elevation };
}
