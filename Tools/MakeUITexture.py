# UI の画像（HP ゲージ）を作るスクリプト
# 使い方：python Tools/MakeUITexture.py
# Asset/Textures/UI/Gage/ に HPGage.png（ゲージの中身）と BackGage.png（ゲージの枠と背景）を作る
#
# ゲージは横に伸び縮みさせて使うので、横方向には模様を入れず、縦方向のグラデーションだけで立体感を出す

import os
import struct
import zlib

import numpy as np

W, H = 256, 32


def write_png_rgba(path, rgba):
	h, w, _ = rgba.shape
	data = (np.clip(rgba, 0, 1) * 255).astype(np.uint8)
	raw = b"".join(b"\x00" + data[y].tobytes() for y in range(h))

	def chunk(kind, body):
		return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body) & 0xffffffff)

	with open(path, "wb") as f:
		f.write(b"\x89PNG\r\n\x1a\n")
		f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)))
		f.write(chunk(b"IDAT", zlib.compress(raw, 9)))
		f.write(chunk(b"IEND", b""))


def gradient(top, mid, bottom):
	"""上 → 真ん中 → 下 の縦のグラデーション"""
	v = np.linspace(0, 1, H)[:, None, None]
	top, mid, bottom = (np.array(c, dtype=float) for c in (top, mid, bottom))
	col = np.where(v < 0.5, top + (mid - top) * (v / 0.5), mid + (bottom - mid) * ((v - 0.5) / 0.5))
	return np.broadcast_to(col, (H, W, 3)).copy()


def make_hp():
	# 明るい緑（上はつやのある明るさ、下は暗く）
	rgb = gradient((0.55, 1.0, 0.65), (0.12, 0.78, 0.38), (0.04, 0.42, 0.20))
	# 上の方に細い光の筋（ガラスのようなつや）
	ys = np.arange(H)[:, None]
	shine = np.exp(-((ys - H * 0.22) / (H * 0.08)) ** 2) * 0.35
	rgb = rgb + shine[..., None]
	# 上下のふちを少し暗く
	edge = np.minimum(ys, H - 1 - ys)
	rgb *= np.where(edge < 2, 0.6, 1.0)[..., None]
	alpha = np.ones((H, W, 1))
	return np.concatenate([np.broadcast_to(rgb, (H, W, 3)), alpha], axis=2)


def make_back():
	# 暗い背景（減った HP の所に見える）＋ 金色の細い枠
	rgb = gradient((0.10, 0.06, 0.07), (0.05, 0.03, 0.04), (0.02, 0.01, 0.02))
	alpha = np.full((H, W, 1), 0.85)
	ys, xs = np.mgrid[0:H, 0:W]
	border = (ys < 2) | (ys >= H - 2) | (xs < 2) | (xs >= W - 2)
	gold = np.array([0.85, 0.68, 0.32])
	rgb[border] = gold
	alpha[border] = 1.0
	return np.concatenate([rgb, alpha], axis=2)


def main():
	root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
	out_dir = os.path.join(root, "Asset", "Textures", "UI", "Gage")
	os.makedirs(out_dir, exist_ok=True)
	write_png_rgba(os.path.join(out_dir, "HPGage.png"), make_hp())
	write_png_rgba(os.path.join(out_dir, "BackGage.png"), make_back())
	print("作成:", os.path.normpath(out_dir))


if __name__ == "__main__":
	main()
