# 鍵の画像（Asset/Textures/Stage/Key.png）と、回復の泉の光の画像（FountainGlow.png）を作るスクリプト
# 使い方：python Tools/MakeKeyTexture.py
#
# ・FountainGlow.png  ：立ちのぼる光・水しぶきに使う丸いにじみ（真ん中ほど明るい水色。加算で重ねる）
# ・FountainWater.png ：水面（青緑の水の色と波の模様。丸の外はアルファ 0 にして、シェーダーのアルファテストで丸く切り抜く）
# ・FountainShine.png ：水面のきらめき（波の明るい筋だけ。加算で重ねる。丸のふちに向かって暗くする）
# ・FountainStream.png：上の皿から流れ落ちる水（縦の筋。上下につながる模様にして、UV を動かして流す。加算）
# ・FountainRing.png  ：水が落ちた所に広がる波紋の輪（加算）
#
# ・ゲームでは、カメラの方を向いた板（ビルボード）に貼り、加算合成で描く（KeyItem.cpp）
# ・ゲームのシェーダーはアルファが 0.55 より小さい所を描かないので、アルファは使わず、
#   色の明るさで形と光のにじみを表す（黒＝何も無い。加算なので黒は見えない）
# ・形：上に丸い輪（持つ所）、下へまっすぐな軸、軸の先に 2 本の歯

import os

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

SIZE = 256
GOLD = np.array((1.0, 0.78, 0.32))		# 鍵の色（金色）
GLOW = np.array((1.0, 0.55, 0.18))		# まわりのにじみの色（少し赤い金色）


FOUNTAIN_COLOR = np.array((0.35, 0.85, 1.0))	# 泉の光の色（水色）


def make_fountain_glow(root):
	"""泉の光：真ん中ほど明るい丸。ゆらめくように、少しだけ波の模様を入れる"""
	ys, xs = np.mgrid[0:SIZE, 0:SIZE]
	dx = (xs - SIZE / 2 + 0.5) / (SIZE / 2)
	dy = (ys - SIZE / 2 + 0.5) / (SIZE / 2)
	d = np.sqrt(dx * dx + dy * dy)
	a = np.clip(1.0 - d, 0.0, 1.0) ** 1.6
	ripple = 0.85 + 0.15 * np.cos(d * 22.0)		# 水の輪の模様
	color = FOUNTAIN_COLOR[None, None, :] * (a * ripple)[..., None]
	color += (np.clip(1.0 - d * 3.0, 0.0, 1.0) ** 2 * 0.5)[..., None]	# 真ん中は白っぽく
	color = np.clip(color, 0.0, 1.0)
	path = os.path.join(root, "Asset", "Textures", "Stage", "FountainGlow.png")
	Image.fromarray((color * 255).astype(np.uint8), "RGB").save(path)
	print("作成:", os.path.normpath(path))


def tile_waves(size, rng, count=7, min_k=2, max_k=6):
	"""上下・左右につながる（タイルできる）波の模様。整数回の波を重ねると、端と端がつながる"""
	ys, xs = np.mgrid[0:size, 0:size] / size * 2 * np.pi
	acc = np.zeros((size, size))
	for _ in range(count):
		kx = rng.integers(-max_k, max_k + 1)
		ky = rng.integers(min_k, max_k + 1)
		phase = rng.uniform(0, 2 * np.pi)
		acc += np.sin(xs * kx + ys * ky + phase)
	acc = (acc - acc.min()) / (acc.max() - acc.min())
	return acc


def save_rgb(root, name, color):
	path = os.path.join(root, "Asset", "Textures", "Stage", name)
	Image.fromarray((np.clip(color, 0, 1) * 255).astype(np.uint8), "RGB").save(path)
	print("作成:", os.path.normpath(path))


def make_fountain_water(root):
	"""水面：青緑の水の色に、波の明るい筋（コースティクス）。丸の外は透明（アルファテストで切り抜く）"""
	rng = np.random.default_rng(31)
	w = tile_waves(SIZE, rng, count=9)
	caustic = (1.0 - np.abs(w * 2.0 - 1.0)) ** 6		# 波の山と谷の境目を、細い明るい筋にする
	deep = np.array((0.05, 0.16, 0.22))
	light = np.array((0.25, 0.55, 0.62))
	color = deep[None, None, :] * (1.0 - w[..., None]) + light[None, None, :] * w[..., None] * 0.6
	color += caustic[..., None] * np.array((0.35, 0.6, 0.65))[None, None, :]
	ys, xs = np.mgrid[0:SIZE, 0:SIZE]
	d = np.sqrt((xs - SIZE / 2 + 0.5) ** 2 + (ys - SIZE / 2 + 0.5) ** 2) / (SIZE / 2)
	color *= (1.0 - 0.35 * d)[..., None]		# ふちほど少し暗く（器の影）
	alpha = (d <= 1.0).astype(np.float32)
	rgba = np.concatenate([np.clip(color, 0, 1), alpha[..., None]], axis=-1)
	path = os.path.join(root, "Asset", "Textures", "Stage", "FountainWater.png")
	Image.fromarray((rgba * 255).astype(np.uint8), "RGBA").save(path)
	print("作成:", os.path.normpath(path))

	# きらめき（加算用）：別の波の筋。ふちへ向かってなめらかに消す
	rng2 = np.random.default_rng(57)
	w2 = tile_waves(SIZE, rng2, count=8)
	shine = (1.0 - np.abs(w2 * 2.0 - 1.0)) ** 10
	shine *= np.clip(1.0 - d, 0.0, 1.0) ** 0.5
	save_rgb(root, "FountainShine.png", shine[..., None] * np.array((0.6, 0.95, 1.0))[None, None, :])


def make_fountain_stream(root):
	"""流れ落ちる水：細い縦の筋をたくさん並べる。上下がつながるので、UV を下へ動かすと流れて見える"""
	rng = np.random.default_rng(73)
	w, h = 64, 256
	ys = np.arange(h)[:, None] / h
	xs = np.arange(w)[None, :] / w
	img = np.zeros((h, w))
	for _ in range(14):
		cx = rng.uniform(0.2, 0.8)
		width = rng.uniform(0.02, 0.06)
		k = rng.integers(1, 4)		# 上下の明るさの波（整数回なので上下がつながる）
		phase = rng.uniform(0, 2 * np.pi)
		line = np.exp(-((xs - cx) / width) ** 2)
		bright = 0.5 + 0.5 * np.sin(ys * 2 * np.pi * k + phase)
		img += line * bright * rng.uniform(0.4, 1.0)
	img = img / img.max()
	edge = np.exp(-((xs - 0.5) / 0.32) ** 4)		# 左右のふちは消す（板の四角が見えないように）
	img *= edge
	save_rgb(root, "FountainStream.png", img[..., None] * np.array((0.55, 0.85, 1.0))[None, None, :])


def make_fountain_ring(root):
	"""波紋の輪：ふちだけが明るい丸"""
	ys, xs = np.mgrid[0:SIZE, 0:SIZE]
	d = np.sqrt((xs - SIZE / 2 + 0.5) ** 2 + (ys - SIZE / 2 + 0.5) ** 2) / (SIZE / 2)
	ring = np.exp(-((d - 0.85) / 0.06) ** 2)
	save_rgb(root, "FountainRing.png", ring[..., None] * np.array((0.6, 0.9, 1.0))[None, None, :])


def main():
	# 鍵の形（白黒の型）
	mask = Image.new("L", (SIZE, SIZE), 0)
	d = ImageDraw.Draw(mask)
	cx = SIZE // 2
	# 輪（外の円から、内の円をくりぬく）
	d.ellipse([cx - 46, 22, cx + 46, 114], fill=255)
	d.ellipse([cx - 24, 44, cx + 24, 92], fill=0)
	# 軸
	d.rectangle([cx - 10, 110, cx + 10, 226], fill=255)
	# 歯（2 本）
	d.rectangle([cx + 10, 184, cx + 40, 198], fill=255)
	d.rectangle([cx + 10, 208, cx + 32, 222], fill=255)

	core = np.asarray(mask, dtype=np.float32) / 255.0
	glow = np.asarray(mask.filter(ImageFilter.GaussianBlur(14)), dtype=np.float32) / 255.0
	soft = np.asarray(mask.filter(ImageFilter.GaussianBlur(3)), dtype=np.float32) / 255.0

	color = soft[..., None] * GOLD[None, None, :] + glow[..., None] * GLOW[None, None, :] * 0.9
	# 輪と軸の真ん中に、白っぽい光の筋（金属の光り）
	color += (core * 0.25)[..., None]
	color = np.clip(color, 0.0, 1.0)

	root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
	path = os.path.join(root, "Asset", "Textures", "Stage", "Key.png")
	Image.fromarray((color * 255).astype(np.uint8), "RGB").save(path)
	print("作成:", os.path.normpath(path))

	make_fountain_glow(root)
	make_fountain_water(root)
	make_fountain_stream(root)
	make_fountain_ring(root)


if __name__ == "__main__":
	main()
