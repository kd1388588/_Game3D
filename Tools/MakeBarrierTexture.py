# ボスの広間の入口をふさぐバリアの画像を作るスクリプト
# 使い方：python Tools/MakeBarrierTexture.py
# Asset/Textures/Stage/Barrier.png（512x256）を作る
#
# 加算合成で描くので、黒いところは透明に見える
# 六角形の網目と、下から上へ薄くなるグラデーション、ふちの光

import os
import struct
import zlib

import numpy as np

W, H = 512, 256
COLOR = np.array([0.55, 0.35, 1.0])		# 紫がかった青（魔法の結界）


def write_png(path, rgb):
	data = (np.clip(rgb, 0, 1) * 255).astype(np.uint8)
	raw = b"".join(b"\x00" + data[y].tobytes() for y in range(H))

	def chunk(kind, body):
		return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body) & 0xffffffff)

	with open(path, "wb") as f:
		f.write(b"\x89PNG\r\n\x1a\n")
		f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 2, 0, 0, 0)))
		f.write(chunk(b"IDAT", zlib.compress(raw, 9)))
		f.write(chunk(b"IEND", b""))


def hex_edge(x, y, size):
	"""六角形の網目の線（線の上ほど 1 に近い）"""
	q = (np.sqrt(3) / 3 * x - 1 / 3 * y) / size
	r = (2 / 3 * y) / size
	cx, cz = q, r
	cy = -cx - cz
	rx, ry, rz = np.round(cx), np.round(cy), np.round(cz)
	dx, dy, dz = np.abs(rx - cx), np.abs(ry - cy), np.abs(rz - cz)
	fix_x = (dx > dy) & (dx > dz)
	fix_y = ~fix_x & (dy > dz)
	rx = np.where(fix_x, -ry - rz, rx)
	ry = np.where(fix_y, -rx - rz, ry)
	rz = np.where(~fix_x & ~fix_y, -rx - ry, rz)
	# 六角形の中心からの距離（ふちで 1）
	ox, oy, oz = cx - rx, cy - ry, cz - rz
	d = np.maximum(np.maximum(np.abs(ox), np.abs(oy)), np.abs(oz)) * 2
	return np.clip((d - 0.82) / 0.18, 0, 1)


def main():
	ys, xs = np.mgrid[0:H, 0:W].astype(float)
	v = ys / H	# 0：上、1：下
	edge = hex_edge(xs, ys, 26.0)

	glow = 0.12 + 0.88 * edge					# 網目の線を明るく
	fade = 0.25 + 0.75 * v ** 1.2				# 下ほど明るい
	side = np.minimum(xs, W - 1 - xs) / W
	side_glow = np.exp(-side * 40) * 0.8		# 左右のふちを光らせる
	bottom_glow = np.exp(-(1 - v) * 25) * 0.6	# 床との境目を光らせる

	intensity = glow * fade * 0.6 + side_glow + bottom_glow
	rgb = intensity[..., None] * COLOR[None, None, :]
	rgb += (edge * fade)[..., None] * 0.25		# 網目の線は白っぽく

	root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
	out_dir = os.path.join(root, "Asset", "Textures", "Stage")
	os.makedirs(out_dir, exist_ok=True)
	write_png(os.path.join(out_dir, "Barrier.png"), rgb)
	print("作成:", os.path.normpath(os.path.join(out_dir, "Barrier.png")))


if __name__ == "__main__":
	main()
