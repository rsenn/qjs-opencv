#!/bin/sh
# One-at-a-time sweep of the edge constants: 1 tap, N images, one CSV per setting.
#   OPENCV_IMGPROC=../opencv-imgproc.js PY=python sh sweep.sh DATA_DIR OUT_DIR [N]
data=$1; out=$2; n=${3:-60}; py=${PY:-python3}
mkdir -p "$out"
run() { name=$1; shift; "$py" -I run.py "$data" "$out/$name.csv" --n "$n" --taps 1 --jobs 3 -- "$@" 2>/dev/null; }
run control
for v in 0.003 0.012 0.02; do run radius-$v --guide-radius $v; done
for v in 2 8; do run contrast-$v --contrast $v; done
for v in 1 4 8; do run slope-$v --soft-slope $v; done
for v in 0.0001 0.01; do run eps-$v --guide-eps $v; done
