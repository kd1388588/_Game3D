# GAME CLEAR / GAME OVER の見出しを、画像として作るスクリプト
# 使い方：python Tools/MakeHeadingImages.py   （必要なもの：pip install pillow）
#   → Asset/Textures/UI/Result/Heading_GameClear.png・Heading_GameOver.png ができる
#
# Windows の文字の描画（GDI）で Cinzel の「M」を描くと、輪郭の重なる所がしま模様に欠ける
# （フォントを作り直しても直らなかった）。そこで見出しだけは、同じフォントで文字を
# きれいに描ける FreeType（Pillow）であらかじめ画像にして、それを表示する。
# 白い文字（透明度つき）で作り、色はゲーム側でかける。

import os
from PIL import Image, ImageDraw, ImageFont

FONT = os.path.join("Asset", "Font", "CinzelStatic.ttf")	# Tools/MakeHeadingFont.py で作ったフォント
CELL_HEIGHT = 96	# 前に UIText で使っていた文字の高さ（GDI の高さ ＝ フォントの上下の幅）
PADDING = 6			# 文字のまわりの余白（ピクセル）

HEADINGS = {
	"Heading_GameClear.png": "GAME CLEAR",
	"Heading_GameOver.png": "GAME OVER",
}


def main():
	root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
	font_path = os.path.join(root, FONT)
	out_dir = os.path.join(root, "Asset", "Textures", "UI", "Result")
	os.makedirs(out_dir, exist_ok=True)

	# GDI の「高さ 96」と同じ大きさにする（文字の大きさ ＝ 高さ × 1000 ÷ (上の幅 + 下の幅)）
	from fontTools.ttLib import TTFont
	tt = TTFont(font_path)
	os2 = tt["OS/2"]
	em = round(CELL_HEIGHT * tt["head"].unitsPerEm / (os2.usWinAscent + os2.usWinDescent))
	font = ImageFont.truetype(font_path, em)

	for name, text in HEADINGS.items():
		left, top, right, bottom = font.getbbox(text)
		w = right - left + PADDING * 2
		h = bottom - top + PADDING * 2
		# 白い文字。透明度（A）に文字の形を入れる
		mask = Image.new("L", (w, h), 0)
		ImageDraw.Draw(mask).text((PADDING - left, PADDING - top), text, font=font, fill=255)
		image = Image.new("RGBA", (w, h), (255, 255, 255, 0))
		image.putalpha(mask)
		path = os.path.join(out_dir, name)
		image.save(path)
		print("作成:", os.path.normpath(path), f"({w}x{h})")


if __name__ == "__main__":
	main()
