# 剣の軌跡（トレイル）の画像を作るスクリプト
# 使い方：python Tools/MakeTrailTexture.py
# Asset/Textures/Trail/ に、オペレーター用（青）とパニッシャー用（オレンジ）の画像を作る
#
# 画像の見方
#   横（左→右）：刃の先端 → 刃の根元
#   縦（上→下）：新しい位置（剣のすぐ後ろ） → 古い位置
# 加算合成で描くので、黒いところは透明に見える

import os
import struct
import zlib

WIDTH = 64
HEIGHT = 256
BASE_BRIGHTNESS = 0.15			# 刃の根元側の明るさ（先端を1.0とした割合）
CORE_COLOR = (1.0, 1.0, 1.0)	# 刃先付近の芯の色

TEXTURES = {
	"SwordTrail_Operator.png": (0.25, 0.65, 1.0),	# 青
	"SwordTrail_Punisher.png": (1.0, 0.4, 0.08),	# オレンジ
}


def smooth_step(edge0, edge1, x):
	t = min(max((x - edge0) / (edge1 - edge0), 0.0), 1.0)
	return t * t * (3.0 - 2.0 * t)


def make_pixels(color):
	rows = []
	for y in range(HEIGHT):
		v = (y + 0.5) / HEIGHT
		fade_old = (1.0 - v) ** 1.5				# 古いほど暗く
		fade_new = smooth_step(0.0, 0.03, v)	# 剣のすぐ後ろは少しだけぼかす
		fade_v = fade_old * fade_new

		row = bytearray([0])	# PNGの各行の先頭はフィルタの種類（0＝なし）
		for x in range(WIDTH):
			u = (x + 0.5) / WIDTH

			# 先端ほど明るく、両端はふちをぼかす
			bright = BASE_BRIGHTNESS + (1.0 - BASE_BRIGHTNESS) * (1.0 - u) ** 1.5
			edge = smooth_step(0.0, 0.06, u) * smooth_step(1.0, 0.8, u)
			intensity = bright * edge * fade_v

			# 先端付近の新しい部分は白っぽい芯にする
			core = (1.0 - u) ** 6.0 * fade_old
			for c, w in zip(color, CORE_COLOR):
				value = (c + (w - c) * core) * intensity
				row.append(int(min(max(value, 0.0), 1.0) * 255.0))
			row.append(255)		# アルファは常に1（UnLitのアルファテストで切られないように）
		rows.append(bytes(row))
	return b"".join(rows)


def write_png(path, pixels):
	def chunk(kind, data):
		body = kind + data
		return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xffffffff)

	header = struct.pack(">IIBBBBB", WIDTH, HEIGHT, 8, 6, 0, 0, 0)	# 8bit RGBA
	with open(path, "wb") as f:
		f.write(b"\x89PNG\r\n\x1a\n")
		f.write(chunk(b"IHDR", header))
		f.write(chunk(b"IDAT", zlib.compress(pixels, 9)))
		f.write(chunk(b"IEND", b""))


def main():
	root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
	out_dir = os.path.join(root, "Asset", "Textures", "Trail")
	os.makedirs(out_dir, exist_ok=True)

	for name, color in TEXTURES.items():
		path = os.path.join(out_dir, name)
		write_png(path, make_pixels(color))
		print("作成:", os.path.normpath(path))


if __name__ == "__main__":
	main()
