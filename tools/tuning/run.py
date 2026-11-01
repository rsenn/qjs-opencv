#!/usr/bin/env python3
# Replay simulated taps through `opencv-imgproc segment` on a matting set and
# write one CSV row per (image, tap count, candidate). See doc/segment-tuning.md.
#
#   python -I run.py DATA_DIR OUT.csv [--n 100] [--taps 4] [--jobs 2] -- EXTRA SEGMENT ARGS
#
# DATA_DIR holds img/NNN.jpg and gt/NNN.png (extract.py). Taps follow the
# usual protocol: the first at the interior point farthest from the boundary,
# each next at the centre of the largest error region of the mask the editor
# would show (candidate 2, or 1 when it covers over WAND_MAX_COVER).
import argparse, csv, glob, os, random, subprocess, sys, tempfile, json, shutil
from concurrent.futures import ThreadPoolExecutor
import cv2, numpy as np

WAND_MAX_COVER = 0.6

def peak(region):
    # centre of the thickest part of a binary region, as x, y
    # the photo's edge counts as a boundary, a tap in the very corner is no tap
    d = cv2.distanceTransform(np.pad(region.astype(np.uint8), 1), cv2.DIST_L2, 5)[1:-1, 1:-1]
    y, x = np.unravel_index(np.argmax(d), d.shape)
    return x, y, d[y, x]

def iou(a, b):
    u = np.logical_or(a, b).sum()
    return float(np.logical_and(a, b).sum() / u) if u else 1.0

def boundary_f(a, b, tol=3):
    k = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (2 * tol + 1, 2 * tol + 1))
    edge = lambda m: m.astype(np.uint8) - cv2.erode(m.astype(np.uint8), np.ones((3, 3), np.uint8))
    ea, eb = edge(a), edge(b)
    if not ea.sum() and not eb.sum(): return 1.0
    p = (ea & cv2.dilate(eb, k)).sum() / max(ea.sum(), 1)
    r = (eb & cv2.dilate(ea, k)).sum() / max(eb.sum(), 1)
    return float(2 * p * r / (p + r)) if p + r else 0.0

def run_tool(img, points, extra, cache, tmp):
    args = [os.environ.get('OPENCV_IMGPROC', 'opencv-imgproc'), 'segment', '--in', img, '--units', 'norm', '--out', os.path.join(tmp, 'cut.png'),
            '--cache-dir', cache, '--candidates', '3', '--pick', '2']
    for x, y, l in points: args += ['--point', '%.5f,%.5f,%d' % (x, y, l)]
    p = subprocess.run(args + extra, capture_output=True, text=True, env={**os.environ, 'LANG': 'C'})
    last = [l for l in p.stdout.splitlines() if l.startswith('{')]
    if not last: return None
    r = json.loads(last[-1])
    return r if r.get('ok') else None

def one(job):
    name, data, extra, taps, cache = job
    img = os.path.join(data, 'img', name + '.jpg')
    gt = cv2.imread(os.path.join(data, 'gt', name + '.png'), cv2.IMREAD_GRAYSCALE) > 127
    h, w = gt.shape
    rows, points = [], []
    tmp = tempfile.mkdtemp(prefix='seg')
    try:
        x, y, _ = peak(gt)
        points.append((x / w, y / h, 1))
        for k in range(1, taps + 1):
            t0 = None
            r = run_tool(img, points, extra, cache, tmp)

            if not r:
                # the tool found no subject: the editor shows nothing, which scores 0
                rows += [dict(image=name, taps=k, cand=c, score=0, coverage=0, iou=0, bf=0, ms=0) for c in range(3)]
                break
            masks = {}
            for c in r['candidates']:
                a = cv2.imread(c['path'], cv2.IMREAD_UNCHANGED)
                m = (a[..., 3] > 127) if a is not None and a.ndim == 3 and a.shape[2] == 4 else None
                if m is None: continue
                if m.shape != gt.shape: m = cv2.resize(m.astype(np.uint8), (w, h), interpolation=cv2.INTER_NEAREST) > 0
                masks[c['index']] = m
                rows.append(dict(image=name, taps=k, cand=c['index'], score=c['score'], coverage=round(c['coverage'], 5),
                                 iou=round(iou(m, gt), 5), bf=round(boundary_f(m, gt), 5), ms=r['timings']['totalMs']))
            shown = 2 if r['candidates'][2]['coverage'] <= WAND_MAX_COVER else 1
            m = masks.get(shown)
            if m is None: break
            fn, fp = gt & ~m, m & ~gt
            if max(fn.sum(), fp.sum()) < 0.002 * w * h: break
            region, label = (fn, 1) if fn.sum() >= fp.sum() else (fp, 0)
            x, y, _ = peak(region)
            points.append((x / w, y / h, label))
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    return rows

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('data'); ap.add_argument('out')
    ap.add_argument('--n', type=int, default=100); ap.add_argument('--skip', type=int, default=0); ap.add_argument('--taps', type=int, default=4)
    ap.add_argument('--jobs', type=int, default=2); ap.add_argument('--seed', type=int, default=1)
    a, extra = ap.parse_known_args()
    extra = [e for e in extra if e != '--']
    names = sorted(os.path.basename(f)[:-4] for f in glob.glob(os.path.join(a.data, 'img', '*.jpg')))
    random.Random(a.seed).shuffle(names)
    names = sorted(names[a.skip:a.skip + a.n])
    cache = os.path.join(a.data, 'cache'); os.makedirs(cache, exist_ok=True)
    cols = ['image', 'taps', 'cand', 'score', 'coverage', 'iou', 'bf', 'ms']
    with open(a.out, 'w', newline='') as f, ThreadPoolExecutor(a.jobs) as ex:
        wr = csv.DictWriter(f, cols); wr.writeheader()
        for i, rows in enumerate(ex.map(one, [(n, a.data, extra, a.taps, cache) for n in names])):
            wr.writerows(rows); f.flush()
            print('\r%d/%d' % (i + 1, len(names)), end='', file=sys.stderr)
    print(file=sys.stderr)

main()
