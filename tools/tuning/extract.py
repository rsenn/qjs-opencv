import sys, io, os, pyarrow.parquet as pq
from PIL import Image
src, dst = sys.argv[1:3]
t = pq.read_table(src).to_pylist()
print(len(t), [k for k in t[0]])
for d in ('img', 'gt'): os.makedirs(os.path.join(dst, d), exist_ok=True)
for i, r in enumerate(t):
    im = Image.open(io.BytesIO(r['image']['bytes'])).convert('RGB')
    m = Image.open(io.BytesIO(r['mask']['bytes'])).convert('L')
    if im.size != m.size: print('size mismatch', i, im.size, m.size)
    im.save(os.path.join(dst, 'img', '%03d.jpg' % i), quality=95)
    m.save(os.path.join(dst, 'gt', '%03d.png' % i))
