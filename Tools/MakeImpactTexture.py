# 叩きつけのエフェクトで使う「地面のひび」の画像を作るスクリプト
# 使い方：python Tools/MakeImpactTexture.py
#   → Asset/Data/Effect/Tex/GroundCrack.png
# 半透明（アルファブレンド）で描くので、色は暗く、ひびの所だけ不透明にする
# 真ん中から外へ、枝分かれしながらのびるひびと、真ん中の焦げたような暗い跡

import math
import os
import random
import struct
import zlib

import numpy as np

SIZE = 256
SEED = 1021


def write_png_rgba(path, rgba):
	h, w, _ = rgba.shape
	data = (np.clip(rgba, 0.0, 1.0) * 255).astype(np.uint8)
	raw = b"".join(b"\x00" + data[y].tobytes() for y in range(h))

	def chunk(kind, body):
		return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body) & 0xffffffff)

	with open(path, "wb") as f:
		f.write(b"\x89PNG\r\n\x1a\n")
		f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)))
		f.write(chunk(b"IDAT", zlib.compress(raw, 9)))
		f.write(chunk(b"IEND", b""))


def draw_line(alpha, x0, y0, x1, y1, width):
	"""太さのある線を描く（線までの距離で濃さを決め、ふちは少しぼかす）"""
	ys, xs = np.mgrid[0:SIZE, 0:SIZE] + 0.5
	dx, dy = x1 - x0, y1 - y0
	length2 = dx * dx + dy * dy
	if length2 < 1e-6:
		return
	t = np.clip(((xs - x0) * dx + (ys - y0) * dy) / length2, 0.0, 1.0)
	d = np.hypot(xs - (x0 + t * dx), ys - (y0 + t * dy))
	line = np.clip(1.0 - (d - width * 0.5) / 1.2, 0.0, 1.0)
	np.maximum(alpha, line, out=alpha)


def crack(alpha, rng, x, y, angle, length, width, depth):
	"""ジグザグにのびるひび。途中で枝分かれする"""
	step = 9.0
	traveled = 0.0
	while traveled < length:
		angle += rng.uniform(-0.28, 0.28)
		nx = x + math.cos(angle) * step
		ny = y + math.sin(angle) * step
		w = max(width * (1.0 - traveled / length), 0.6)	# 先ほど細く
		draw_line(alpha, x, y, nx, ny, w)
		x, y = nx, ny
		traveled += step
		if depth < 2 and rng.random() < 0.08:
			crack(alpha, rng, x, y, angle + rng.choice((-1, 1)) * rng.uniform(0.35, 0.7),
				(length - traveled) * rng.uniform(0.4, 0.7), w * 0.7, depth + 1)


def main():
	rng = random.Random(SEED)
	c = SIZE / 2
	alpha = np.zeros((SIZE, SIZE))

	# 真ん中から放射状にのびるひび（9本）
	count = 9
	for i in range(count):
		angle = 2 * math.pi * i / count + rng.uniform(-0.25, 0.25)
		length = rng.uniform(SIZE * 0.3, SIZE * 0.46)
		crack(alpha, rng, c, c, angle, length, rng.uniform(2.5, 3.5), 0)

	# 真ん中のへこんだ暗い跡（ふちはやわらかく消える）
	ys, xs = np.mgrid[0:SIZE, 0:SIZE] + 0.5
	r = np.hypot(xs - c, ys - c) / (SIZE / 2)
	scorch = np.clip(1.0 - r / 0.32, 0.0, 1.0) ** 1.5 * 0.75
	# 外側ほどひびを薄く（板のふちで切れて見えないように）
	fade = np.clip((1.0 - r) / 0.25, 0.0, 1.0)
	a = np.maximum(alpha * 0.95 * fade, scorch)

	color = np.zeros((SIZE, SIZE, 4))
	color[..., 0] = 0.07
	color[..., 1] = 0.055
	color[..., 2] = 0.045
	color[..., 3] = a

	root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
	path = os.path.join(root, "Asset", "Data", "Effect", "Tex", "GroundCrack.png")
	write_png_rgba(path, color)
	print("作成:", os.path.normpath(path))


if __name__ == "__main__":
	main()
