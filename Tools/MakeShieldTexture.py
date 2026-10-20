# ガードのエフェクトで使う「盾」と「盾の破片」の画像を作るスクリプト
# 使い方：python Tools/MakeShieldTexture.py
#   → Asset/Data/Effect/Tex/ShieldHex.png（半透明の「（ ）」の形の盾）、ShieldShard.png（砕けた盾のかけら）
# 加算合成で描くので、黒・透明のところは見えない。色はエフェクト側でかけ算する（白っぽい水色で作る）

import math
import os
import struct
import zlib

BASE_COLOR = (0.75, 0.92, 1.0)	# 白っぽい水色


def smooth_step(e0, e1, x):
	t = min(max((x - e0) / (e1 - e0), 0.0), 1.0)
	return t * t * (3.0 - 2.0 * t)


def hex_edge(x, y, size):
	"""六角形（蜂の巣）の模様の線までの近さ（線の上で 1、離れると 0）。size は六角形の中心から角までの長さ"""
	# 一番近い六角形の中心を求める（アキシャル座標を丸める）
	q = (math.sqrt(3) / 3 * x - y / 3) / size
	r = (2 / 3 * y) / size
	cx, cz = q, r
	cy = -cx - cz
	rx, ry, rz = round(cx), round(cy), round(cz)
	dx, dy, dz = abs(rx - cx), abs(ry - cy), abs(rz - cz)
	if dx > dy and dx > dz:
		rx = -ry - rz
	elif dy > dz:
		ry = -rx - rz
	else:
		rz = -rx - ry
	hx = size * math.sqrt(3) * (rx + rz / 2)
	hy = size * 1.5 * rz
	# 六角形の中心からの距離（辺までの距離で測る。辺の上で 1）
	lx, ly = x - hx, y - hy
	apothem = size * math.sqrt(3) / 2
	d = 0.0
	for ang in (0.0, math.pi / 3, 2 * math.pi / 3):
		d = max(d, abs(lx * math.cos(ang) + ly * math.sin(ang)))
	return smooth_step(0.88, 0.98, d / apothem)


def write_png(path, width, height, pixel):
	rows = []
	for j in range(height):
		row = bytearray([0])
		for i in range(width):
			r, g, b, a = pixel((i + 0.5) / width * 2 - 1, 1 - (j + 0.5) / height * 2)
			for v in (r, g, b, a):
				row.append(int(min(max(v, 0.0), 1.0) * 255))
		rows.append(bytes(row))

	def chunk(kind, data):
		body = kind + data
		return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xffffffff)

	with open(path, "wb") as f:
		f.write(b"\x89PNG\r\n\x1a\n")
		f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)))
		f.write(chunk(b"IDAT", zlib.compress(b"".join(rows), 9)))
		f.write(chunk(b"IEND", b""))
	print("作成:", os.path.normpath(path))


def shield(x, y):
	"""半透明の「（ ）」の形の盾：左右のふちが強く光り、真ん中は薄く透ける（丸く曲がった盾を正面から見た感じ）"""
	r = math.hypot(x, y)
	if r > 1.0:
		return 0, 0, 0, 0
	side = abs(x)
	# 左右のふちの光る弧（上下に行くほど細く・暗くして「（ ）」の形にする）
	arc = smooth_step(0.72, 0.95, r) * smooth_step(1.0, 0.95, r) * smooth_step(0.15, 0.85, side / max(r, 1e-6))
	# 曲面の照り返し（外側ほど明るい。左右を強く）
	fresnel = 0.55 * (side ** 4) + 0.12 * r ** 3
	grid = hex_edge(x, y, 0.13) * (0.06 + 0.3 * side ** 2)	# 六角形の模様は外側ほど見える
	base = 0.06												# 真ん中もうっすら見える
	v = min(arc + fresnel + grid + base, 1.0) * smooth_step(1.0, 0.96, r)
	c = BASE_COLOR
	white = arc * 0.6
	return (c[0] + (1 - c[0]) * white) * v, (c[1] + (1 - c[1]) * white) * v, (c[2] + (1 - c[2]) * white) * v, v


def shard(x, y):
	"""盾のかけら：ぎざぎざの三角形。ふちが光り、中に模様"""
	# 三角形の3つの頂点（少しゆがませる）
	pts = [(-0.85, -0.7), (0.9, -0.45), (-0.1, 0.9)]
	# 各辺の内側までの距離
	dmin = 1e9
	inside = True
	for k in range(3):
		(x0, y0), (x1, y1) = pts[k], pts[(k + 1) % 3]
		ex, ey = x1 - x0, y1 - y0
		n = math.hypot(ex, ey)
		# 内向きの距離（反時計回りの頂点なので左側が内側）
		d = (ex * (y - y0) - ey * (x - x0)) / n
		# 辺をぎざぎざにする
		d += 0.025 * math.sin((x * ex + y * ey) / n * 9.0 + k * 2.0)
		if d < 0:
			inside = False
		dmin = min(dmin, d)
	if not inside:
		return 0, 0, 0, 0
	edge = smooth_step(0.12, 0.0, dmin)
	grid = hex_edge(x + 0.3, y, 0.4) * 0.5
	v = min(0.25 + edge + grid, 1.0) * smooth_step(0.0, 0.03, dmin)
	c = BASE_COLOR
	return c[0] * v + edge * 0.25, c[1] * v + edge * 0.1, c[2] * v, v


def main():
	root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
	out_dir = os.path.join(root, "Asset", "Data", "Effect", "Tex")
	os.makedirs(out_dir, exist_ok=True)
	write_png(os.path.join(out_dir, "ShieldHex.png"), 256, 256, shield)
	write_png(os.path.join(out_dir, "ShieldShard.png"), 64, 64, shard)


if __name__ == "__main__":
	main()
