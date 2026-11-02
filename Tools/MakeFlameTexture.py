# ステージの松明・かがり火・燭台の「炎の光」の画像を作るスクリプト
# 使い方：python Tools/MakeFlameTexture.py
#   → Asset/Textures/Stage/Flame.png（ゲーム側の StageLights が、カメラの方を向けて加算合成で描く）
# 下が丸く、上に向かって細くとがる、しずくの形。中心ほど白っぽく、外側はオレンジから黒へ消える
# 加算合成で描くので、黒いところは透明に見える（アルファは常に 1）

import math
import os
import struct
import zlib

WIDTH = 64
HEIGHT = 128


def smooth_step(e0, e1, x):
	t = min(max((x - e0) / (e1 - e0), 0.0), 1.0)
	return t * t * (3.0 - 2.0 * t)


def pixel(u, v):
	"""u：横 -1～1、v：縦 0（下）～1（上）"""
	# しずくの形：下の方は丸く太く、上へ行くほど細くなる
	width = 0.75 * math.sin(math.pi * min(v * 1.1, 1.0)) ** 0.6 * (1.0 - v) ** 0.35
	if width <= 0.0:
		return (0.0, 0.0, 0.0)
	d = abs(u) / width
	body = smooth_step(1.0, 0.2, d) * smooth_step(0.0, 0.12, v)
	core = smooth_step(0.6, 0.0, d) * smooth_step(0.75, 0.15, v)	# 根元の白い芯
	r = body * 1.0 + core * 0.2
	g = body * 0.55 + core * 0.45
	b = body * 0.15 + core * 0.35
	return (min(r, 1.0), min(g, 1.0), min(b, 1.0))


def main():
	rows = []
	for y in range(HEIGHT):
		v = 1.0 - (y + 0.5) / HEIGHT
		row = bytearray([0])
		for x in range(WIDTH):
			u = (x + 0.5) / WIDTH * 2.0 - 1.0
			r, g, b = pixel(u, v)
			row += bytes([int(r * 255), int(g * 255), int(b * 255), 255])
		rows.append(bytes(row))

	def chunk(kind, data):
		return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xffffffff)

	root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
	path = os.path.join(root, "Asset", "Textures", "Stage", "Flame.png")
	os.makedirs(os.path.dirname(path), exist_ok=True)
	with open(path, "wb") as f:
		f.write(b"\x89PNG\r\n\x1a\n")
		f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", WIDTH, HEIGHT, 8, 6, 0, 0, 0)))
		f.write(chunk(b"IDAT", zlib.compress(b"".join(rows), 9)))
		f.write(chunk(b"IEND", b""))
	print("作成:", os.path.normpath(path))


if __name__ == "__main__":
	main()
