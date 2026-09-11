/**
 * WGS84-sphere approximation of bearing/elevation from a fixed ground
 * camera to an ADS-B-reported aircraft position. Good to well under a
 * pixel of fisheye error at aviation ranges/altitudes - full ellipsoid
 * geodesy isn't worth the complexity here.
 */

const EARTH_RADIUS_M = 6371000;

function toRad(deg) {
  return (deg * Math.PI) / 180;
}

function toDeg(rad) {
  return (rad * 180) / Math.PI;
}

/** Great-circle distance in meters between two lat/lon points (haversine). */
export function groundDistance(lat1, lon1, lat2, lon2) {
  const φ1 = toRad(lat1), φ2 = toRad(lat2);
  const dφ = toRad(lat2 - lat1), dλ = toRad(lon2 - lon1);
  const a = Math.sin(dφ / 2) ** 2 + Math.cos(φ1) * Math.cos(φ2) * Math.sin(dλ / 2) ** 2;
  return 2 * EARTH_RADIUS_M * Math.asin(Math.sqrt(a));
}

/** Initial bearing in degrees (0=N, 90=E, ...) from point 1 to point 2. */
export function bearing(lat1, lon1, lat2, lon2) {
  const φ1 = toRad(lat1), φ2 = toRad(lat2), dλ = toRad(lon2 - lon1);
  const y = Math.sin(dλ) * Math.cos(φ2);
  const x = Math.cos(φ1) * Math.sin(φ2) - Math.sin(φ1) * Math.cos(φ2) * Math.cos(dλ);
  return (toDeg(Math.atan2(y, x)) + 360) % 360;
}

/**
 * Azimuth (degrees, 0=N/90=E) and elevation (degrees, 0=horizon,
 * 90=directly overhead) of an aircraft as seen from a fixed camera.
 *
 * camera: { lat, lon, altM }  aircraft: { lat, lon, altM }
 */
export function azimuthElevation(camera, aircraft) {
  const dist = groundDistance(camera.lat, camera.lon, aircraft.lat, aircraft.lon);
  const az = bearing(camera.lat, camera.lon, aircraft.lat, aircraft.lon);
  const heightDiff = aircraft.altM - camera.altM;
  const el = toDeg(Math.atan2(heightDiff, dist));
  return { azimuth: az, elevation: el };
}
