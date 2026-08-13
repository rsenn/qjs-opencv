// cv/marshal.js
//
// Marshalling between a cv.Mat and a structured-clone-safe message used to
// ship frames between the GUI thread and an os.Worker. Zero-copy on the wire
// (mat.buffer is a view onto the Mat's pixel data); postMessage's structured
// clone makes the copy on its own.

import { Mat } from 'opencv.so';

export function matToMsg(mat) {
  return { rows: mat.rows, cols: mat.cols, type: mat.type(), buffer: mat.buffer };
}

export function msgToMat({ rows, cols, type, buffer }) {
  return new Mat(rows, cols, type, buffer);
}
