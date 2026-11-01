#!/usr/bin/env python3
# Summarise a run.py CSV: mIoU per tap count for each pick policy, the best
# possible candidate (oracle), NoC@85 / NoC@90 and mean latency.
#
#   python -I report.py results/baseline.csv [other.csv ...]
import csv, sys
from collections import defaultdict

WAND_MAX_COVER = 0.6

def load(path):
    by = defaultdict(dict)
    for r in csv.DictReader(open(path)):
        by[(r['image'], int(r['taps']))][int(r['cand'])] = {k: float(r[k]) for k in ('score', 'coverage', 'iou', 'bf', 'ms')}
    return by

# the policies compared: what the editor does now, SAM's own score, a fixed index, the oracle
def editor(c): return c[2] if c[2]['coverage'] <= WAND_MAX_COVER or 1 not in c else c[1]
def best_score(c): return max(c.values(), key=lambda v: v['score'])
def oracle(c): return max(c.values(), key=lambda v: v['iou'])

POLICIES = {'editor': editor, 'score': best_score, 'cand0': lambda c: c[0], 'cand1': lambda c: c[1], 'cand2': lambda c: c[2], 'oracle': oracle}

def noc(by, images, pick, thr):
    out = []
    for im in images:
        ks = sorted(k for (i, k) in by if i == im)
        hit = next((k for k in ks if pick(by[(im, k)])['iou'] >= thr), None)
        out.append(hit if hit else (ks[-1] + 1 if ks else 5))
    return sum(out) / len(out)

for path in sys.argv[1:]:
    by = load(path)
    images = sorted({i for (i, _) in by})
    maxk = max(k for (_, k) in by)
    print('%s: %d images' % (path, len(images)))
    print('%-8s %s  NoC@85 NoC@90  bf@1   ms' % ('policy', ' '.join('mIoU@%d' % k for k in range(1, maxk + 1))))

    for name, pick in POLICIES.items():
        cells = []
        for k in range(1, maxk + 1):
            # an image that stopped early keeps its last mask
            ious = []
            for im in images:
                ks = [x for x in range(1, k + 1) if (im, x) in by]
                ious.append(pick(by[(im, ks[-1])])['iou'] if ks else 0)
            cells.append(sum(ious) / len(ious))

        bf = sum(pick(by[(im, 1)])['bf'] for im in images if (im, 1) in by) / len(images)
        ms = sum(by[(im, 1)][2]['ms'] for im in images if (im, 1) in by) / len(images)
        print('%-8s %s  %5.2f  %5.2f  %.3f %5.0f' % (name, ' '.join('%6.3f ' % c for c in cells), noc(by, images, pick, .85), noc(by, images, pick, .90), bf, ms))
