/**
 * Turns one run of the classical detector (js/skywatch/detect.js) into one
 * row of phase 6's training dataset - see js/skywatch/skywatch.md's
 * "Weak-label pipeline" section for the schema and why this exists: it's
 * what lets dataset collection start today (source: "classical") instead
 * of waiting for phase 4's ADS-B matching to exist (source: "adsb").
 *
 * Writes, under <dataset-dir>:
 *   images/<id>.png   - the whiteness channel (model input; same channel
 *                        detect.js's top-hat is computed from, so the
 *                        network starts from the representation we already
 *                        know carries the signal, not raw RGB)
 *   masks/<id>.png     - acceptedMask (model target: 255 = contrail pixel)
 *   manifest.jsonl      - one JSON row appended per call (schema in
 *                        skywatch.md)
 *
 * Usage:
 *   qjsm js/skywatch/export-training-example.js <input.jpg> <dataset-dir> \
 *     [aircraft-json]
 *
 *   aircraft-json, if given, is a JSON string like
 *   '{"hex":"4b1806","flight":"SWR123","altM":10668}' and marks this
 *   example source:"adsb" instead of source:"classical". Phase 4 doesn't
 *   exist yet, so nothing currently produces this automatically - passing
 *   it by hand is a placeholder until that matching step is wired up to
 *   call this script itself.
 */
import * as cv from 'opencv';
import * as std from 'std';
import * as os from 'os';
import { detectContrails } from './detect.js';

function ensureDir(dir) {
  os.mkdir(dir); // no-op (returns an error code, not a throw) if it already exists
}

function main(inputPath, datasetDir, aircraftJson) {
  const bgr = cv.imread(inputPath);
  if (bgr.empty()) throw new Error(`could not read ${inputPath}`);

  const { whiteness, acceptedMask, results } = detectContrails(bgr);
  if (results.length === 0) {
    console.log(JSON.stringify({ input: inputPath, skipped: 'no accepted candidates' }));
    return;
  }

  const imagesDir = `${datasetDir}/images`, masksDir = `${datasetDir}/masks`;
  ensureDir(datasetDir);
  ensureDir(imagesDir);
  ensureDir(masksDir);

  const now = new Date();
  const stamp = now.toISOString().replace(/[-:]/g, '').replace(/\.\d+Z$/, 'Z');
  const base = inputPath.split('/').pop().replace(/\.[^.]+$/, '');
  const id = `${stamp}-${base}`;

  const imagePath = `${imagesDir}/${id}.png`;
  const maskPath = `${masksDir}/${id}.png`;
  cv.imwrite(imagePath, whiteness);
  cv.imwrite(maskPath, acceptedMask);

  const row = {
    id,
    image: `images/${id}.png`,
    mask: `masks/${id}.png`,
    source: aircraftJson ? 'adsb' : 'classical',
    capturedAt: now.toISOString(),
  };
  if (aircraftJson) row.aircraft = JSON.parse(aircraftJson);

  const manifestPath = `${datasetDir}/manifest.jsonl`;
  const f = std.open(manifestPath, 'a');
  f.puts(JSON.stringify(row) + '\n');
  f.close();

  console.log(JSON.stringify({ input: inputPath, exported: id, trailsFound: results.length }));
}

main(...scriptArgs.slice(1));
