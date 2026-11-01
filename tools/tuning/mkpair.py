#!/usr/bin/env python3
# Two-object test set from a matting set: two cut-out objects (photo times
# matte) side by side on a third photo; the ground truth is the union of the
# two mattes. Same layout as the source (img/NNN.jpg, gt/NNN.png), so run.py
# reads it unchanged: tap 1 lands in the thicker object, the largest error
# after it is the other object.
#
#   python -I mkpair.py SRC_DIR DST_DIR [--n 60] [--seed 7]
import argparse, glob, os, random
import cv2, numpy as np

ap = argparse.ArgumentParser()
ap.add_argument('src'); ap.add_argument('dst')
ap.add_argument('--n', type=int, default=60); ap.add_argument('--seed', type=int, default=7)
a = ap.parse_args()
W, H = 1024, 768
rnd = random.Random(a.seed)
names = sorted(os.path.basename(f)[:-4] for f in glob.glob(os.path.join(a.src, 'img', '*.jpg')))
for d in ('img', 'gt'): os.makedirs(os.path.join(a.dst, d), exist_ok=True)

def cutout(name):
    img = cv2.imread(os.path.join(a.src, 'img', name + '.jpg'))
    al = cv2.imread(os.path.join(a.src, 'gt', name + '.png'), cv2.IMREAD_GRAYSCALE)
    share = (al > 127).mean()
    if not .05 < share < .5: return None
    ys, xs = np.where(al > 127)
    x0, x1, y0, y1 = xs.min(), xs.max() + 1, ys.min(), ys.max() + 1
    return img[y0:y1, x0:x1], al[y0:y1, x0:x1]

def place(canvas, alpha, obj, left):
    img, al = obj
    k = min(0.42 * W / img.shape[1], 0.8 * H / img.shape[0])
    img = cv2.resize(img, None, fx=k, fy=k, interpolation=cv2.INTER_AREA)
    al = cv2.resize(al, (img.shape[1], img.shape[0]), interpolation=cv2.INTER_AREA).astype(np.float32) / 255
    x = rnd.randint(int(.04 * W), int(.46 * W) - img.shape[1]) if left else rnd.randint(int(.54 * W), int(.96 * W) - img.shape[1])
    y = rnd.randint(0, H - img.shape[0])
    roi = canvas[y:y + img.shape[0], x:x + img.shape[1]]
    canvas[y:y + img.shape[0], x:x + img.shape[1]] = (roi * (1 - al[..., None]) + img * al[..., None]).astype(np.uint8)
    alpha[y:y + img.shape[0], x:x + img.shape[1]] = np.maximum(alpha[y:y + img.shape[0], x:x + img.shape[1]], al)

made = 0
while made < a.n:
    pa, pb, pc = rnd.sample(names, 3)
    oa, ob = cutout(pa), cutout(pb)
    if oa is None or ob is None: continue
    bg = cv2.imread(os.path.join(a.src, 'img', pc + '.jpg'))
    k = max(W / bg.shape[1], H / bg.shape[0])
    bg = cv2.resize(bg, None, fx=k, fy=k, interpolation=cv2.INTER_AREA)[:H, :W].copy()
    alpha = np.zeros((H, W), np.float32)
    place(bg, alpha, oa, True); place(bg, alpha, ob, False)
    cv2.imwrite(os.path.join(a.dst, 'img', '%03d.jpg' % made), bg, [cv2.IMWRITE_JPEG_QUALITY, 95])
    cv2.imwrite(os.path.join(a.dst, 'gt', '%03d.png' % made), (alpha * 255).astype(np.uint8))
    made += 1
print(made, 'pairs in', a.dst)
