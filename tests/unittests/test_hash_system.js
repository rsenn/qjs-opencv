import { tests, assert, eq } from './tinytest.js';
import * as cv from 'opencv';

// cv.hash.sha1/sha256 (third_party/Hash-Algorithms) against known
// vectors, and the process-wide cv:: settings in js_utility.cpp.

const ABC = 'abc';
const SHA1_ABC = 'a9993e364706816aba3e25717850c26c9cd0d89d';
const SHA256_ABC = 'ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad';
const SHA1_EMPTY = 'da39a3ee5e6b4b0d3255bfef95601890afd80709';
const SHA256_EMPTY = 'e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855';

const bytes = s => Uint8Array.from(s, c => c.charCodeAt(0));

tests({
  'sha1 and sha256 of a string'() {
    eq(cv.hash.sha1(ABC), SHA1_ABC);
    eq(cv.hash.sha256(ABC), SHA256_ABC);
  },
  'sha of the empty string'() {
    eq(cv.hash.sha1(''), SHA1_EMPTY);
    eq(cv.hash.sha256(''), SHA256_EMPTY);
  },
  'sha of an ArrayBuffer and a Uint8Array'() {
    eq(cv.hash.sha256(bytes(ABC).buffer), SHA256_ABC);
    eq(cv.hash.sha1(bytes(ABC)), SHA1_ABC);
  },
  'sha of a typed array view hashes only the view'() {
    const u8 = bytes('xxabcyy');
    eq(cv.hash.sha256(u8.subarray(2, 5)), SHA256_ABC);
    eq(cv.hash.sha1(u8.subarray(2, 5)), SHA1_ABC);
  },
  'sha across the padding boundaries'() {
    // reference values from sha256sum of that many 'a' bytes
    const ref = { 55: '9f4390f8d30c2dd92ec9f095b65e2b9ae9b0a925a5258e241c9f1e910f734318',
                  56: 'b35439a4ac6f0948b6d6f9e3c6af0f5f590ce20f1bde7090ef7970686ec6738a',
                  64: 'ffe054fe7ae0cb6dc65c3af9b61d5209f439851db43d0ba5997337df154668eb' };
    for(const n in ref) eq(cv.hash.sha256('a'.repeat(n)), ref[n]);
  },
  'sha rejects a non-buffer'() {
    let threw = false;
    try { cv.hash.sha256(42); } catch(e) { threw = true; }
    assert(threw, 'number must throw');
  },
  'setNumThreads round trip'() {
    const before = cv.getNumThreads();
    assert(before >= 1, 'getNumThreads >= 1');
    cv.setNumThreads(1);
    eq(cv.getNumThreads(), 1);
    cv.setNumThreads(before);
  },
  'getBuildInformation names OpenCV'() {
    const info = cv.getBuildInformation();
    assert(typeof info == 'string' && /OpenCV/.test(info), 'contains OpenCV');
  },
  'setRNGSeed makes randu reproducible'() {
    const run = () => {
      cv.setRNGSeed(7);
      const m = new cv.Mat(8, 8, cv.CV_8UC1);
      cv.randu(m, new cv.Scalar(0), new cv.Scalar(255));
      return Array.from(m.data).join(',');
    };
    eq(run(), run());
  },
});
