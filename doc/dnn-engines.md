# DNN engines and ONNX Runtime

`cv.readNetFromONNX(path | ArrayBuffer[, size], engine)` takes the engine as its last argument (OpenCV 5.x). The constants are
under `cv.dnn`: `ENGINE_AUTO` (0, resolves to `ENGINE_OPENCV`), `ENGINE_OPENCV` (1, the builtin importer and engine) and
`ENGINE_ORT` (2, ONNX Runtime, ONNX models only). On OpenCV 4.x the constants are `ENGINE_CLASSIC`/`ENGINE_NEW`/`ENGINE_AUTO`.

```js
import * as cv from 'opencv';
const net = cv.readNetFromONNX('model.onnx', cv.dnn.ENGINE_ORT);
```

## Why use ORT

OpenCV 5.0's builtin ONNX importer rejects graphs exported with dynamic `batch_size`/`sequence_length` axes (HuggingFace
optimum default): "dynamic 'zero' shapes are not supported" at the first `Shape` node. ONNX Runtime has no such limit. The
alternative is re-exporting the model with static shapes (`examples/models/setup-embedding-model.py`).

## Requirements

OpenCV must be built with `WITH_ONNXRUNTIME=ON` (`build-opencv.sh` fetches ONNX Runtime for every target, see `ORT_VERSION`).
Without it, `ENGINE_ORT` falls back to `ENGINE_OPENCV` and OpenCV logs "built without ONNX Runtime ... Falling back" at
WARN level, so check the log when a dynamic-shape model still fails.

Status: the argument and constants are tested (`tests/test_dnn_engine.js`); loading a dynamic-shape graph through ORT has not
been run yet, because no ORT-enabled OpenCV was installed on the development host when this was written.
