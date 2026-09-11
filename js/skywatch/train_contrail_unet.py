#!/usr/bin/env python3
"""
Trains the phase-6 contrail segmentation model on the dataset produced by
export-training-example.js (manifest.jsonl + images/*.png + masks/*.png,
schema documented in skywatch.md's "Weak-label pipeline" section).

Not runnable yet in any meaningful sense - there is no dataset until phase
3/4 (readsb polling, ADS-B matching) is running for real on the Pi and has
accumulated examples, or until export-training-example.js has been run
against enough real photos. This script exists so the model definition,
training loop, and ONNX export path are ready the moment a dataset exists,
rather than being designed from scratch under time pressure then.

Deliberately small model: this runs on a Raspberry Pi 4 via
cv.dnn.readNetFromONNX (see examples/skywatch_contrail_infer_dnn.js),
and realistically there will be hundreds, not millions, of training
examples for a long while - a large model would just overfit.

Usage:
    python3 train_contrail_unet.py --dataset /path/to/dataset \
        --epochs 50 --out contrail_unet.onnx

Requires: torch, numpy, Pillow (CPU is fine for this model size).
"""
import argparse
import json
import numpy as np
from pathlib import Path

import torch
import torch.nn as nn
import torch.nn.functional as F
from torch.utils.data import Dataset, DataLoader

INPUT_SIZE = 256  # resize both image and mask to this square before training


class ConvBlock(nn.Module):
    def __init__(self, in_ch, out_ch):
        super().__init__()
        self.net = nn.Sequential(
            nn.Conv2d(in_ch, out_ch, 3, padding=1), nn.BatchNorm2d(out_ch), nn.ReLU(inplace=True),
            nn.Conv2d(out_ch, out_ch, 3, padding=1), nn.BatchNorm2d(out_ch), nn.ReLU(inplace=True),
        )

    def forward(self, x):
        return self.net(x)


class ContrailUNet(nn.Module):
    """4-level U-Net, single-channel in (whiteness) and out (probability).
    Narrow channel counts (16/32/64/128) - see this file's header comment
    on why small is the right call here, not a limitation to work around.
    """
    CHANNELS = (16, 32, 64, 128)

    def __init__(self):
        super().__init__()
        c1, c2, c3, c4 = self.CHANNELS
        self.enc1 = ConvBlock(1, c1)
        self.enc2 = ConvBlock(c1, c2)
        self.enc3 = ConvBlock(c2, c3)
        self.bottleneck = ConvBlock(c3, c4)
        self.pool = nn.MaxPool2d(2)

        self.up3 = nn.ConvTranspose2d(c4, c3, 2, stride=2)
        self.dec3 = ConvBlock(c4, c3)
        self.up2 = nn.ConvTranspose2d(c3, c2, 2, stride=2)
        self.dec2 = ConvBlock(c3, c2)
        self.up1 = nn.ConvTranspose2d(c2, c1, 2, stride=2)
        self.dec1 = ConvBlock(c2, c1)

        self.out = nn.Conv2d(c1, 1, 1)

    def forward(self, x):
        e1 = self.enc1(x)
        e2 = self.enc2(self.pool(e1))
        e3 = self.enc3(self.pool(e2))
        b = self.bottleneck(self.pool(e3))

        d3 = self.dec3(torch.cat([self.up3(b), e3], dim=1))
        d2 = self.dec2(torch.cat([self.up2(d3), e2], dim=1))
        d1 = self.dec1(torch.cat([self.up1(d2), e1], dim=1))

        return torch.sigmoid(self.out(d1))  # per-pixel contrail probability


class ContrailDataset(Dataset):
    """Reads the manifest.jsonl schema from skywatch.md. `source: "adsb"`
    rows are the trustworthy ones; `source: "classical"` rows are weaker
    labels (the classical detector's own output, not independently
    confirmed) - weighted down via `classical_weight` rather than excluded
    outright, since early on they may be all the data there is.
    """

    def __init__(self, dataset_dir, classical_weight=0.5):
        from PIL import Image  # deferred: only needed if this class is used
        self.Image = Image
        self.dataset_dir = Path(dataset_dir)
        self.rows = []
        manifest = self.dataset_dir / 'manifest.jsonl'
        with open(manifest) as f:
            for line in f:
                line = line.strip()
                if line:
                    self.rows.append(json.loads(line))
        self.classical_weight = classical_weight

    def __len__(self):
        return len(self.rows)

    def __getitem__(self, idx):
        row = self.rows[idx]
        img = self.Image.open(self.dataset_dir / row['image']).convert('L').resize((INPUT_SIZE, INPUT_SIZE))
        mask = self.Image.open(self.dataset_dir / row['mask']).convert('L').resize((INPUT_SIZE, INPUT_SIZE))

        img_t = torch.from_numpy(np.array(img)).float().unsqueeze(0) / 255.0
        mask_t = torch.from_numpy(np.array(mask)).float().unsqueeze(0) / 255.0
        weight = 1.0 if row['source'] == 'adsb' else self.classical_weight
        return img_t, mask_t, weight


def train(dataset_dir, epochs, out_path, batch_size=8, lr=1e-3):
    device = 'cuda' if torch.cuda.is_available() else 'cpu'
    ds = ContrailDataset(dataset_dir)
    if len(ds) == 0:
        raise SystemExit(f'no rows in {dataset_dir}/manifest.jsonl - nothing to train on yet')
    loader = DataLoader(ds, batch_size=batch_size, shuffle=True)

    model = ContrailUNet().to(device)
    opt = torch.optim.Adam(model.parameters(), lr=lr)

    for epoch in range(epochs):
        model.train()
        total_loss = 0.0
        for img, mask, weight in loader:
            img, mask, weight = img.to(device), mask.to(device), weight.to(device)
            pred = model(img)
            # per-example BCE, weighted by label-source confidence (see
            # ContrailDataset's docstring)
            loss = F.binary_cross_entropy(pred, mask, reduction='none').mean(dim=[1, 2, 3])
            loss = (loss * weight).mean()
            opt.zero_grad()
            loss.backward()
            opt.step()
            total_loss += loss.item() * img.size(0)
        print(f'epoch {epoch + 1}/{epochs}  loss={total_loss / len(ds):.4f}')

    model.eval()
    dummy = torch.zeros(1, 1, INPUT_SIZE, INPUT_SIZE, device=device)
    torch.onnx.export(
        model, dummy, out_path,
        input_names=['whiteness'], output_names=['contrail_prob'],
        dynamic_axes={'whiteness': {2: 'h', 3: 'w'}, 'contrail_prob': {2: 'h', 3: 'w'}},
        opset_version=17,
    )
    print(f'exported {out_path}')


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--dataset', required=True, help='dataset dir from export-training-example.js')
    p.add_argument('--epochs', type=int, default=50)
    p.add_argument('--batch-size', type=int, default=8)
    p.add_argument('--lr', type=float, default=1e-3)
    p.add_argument('--out', default='contrail_unet.onnx')
    args = p.parse_args()
    train(args.dataset, args.epochs, args.out, args.batch_size, args.lr)
