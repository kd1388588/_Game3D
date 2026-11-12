# 鍵の画像（Asset/Textures/Stage/Key.png）と、回復の泉の光の画像（FountainGlow.png）を作るスクリプト
# 使い方：python Tools/MakeKeyTexture.py
#
# ・FountainGlow.png：泉の水面と、立ちのぼる光に使う丸いにじみ（真ん中ほど明るい水色。加算で重ねる）
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


if __name__ == "__main__":
	main()
