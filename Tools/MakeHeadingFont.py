# 見出しのフォント（Cinzel）を、ゲームできれいに表示できる形に作り直すスクリプト
# 使い方：python Tools/MakeHeadingFont.py
#   → Asset/Font/CinzelStatic.ttf（フォント名「Cinzel Static」）ができる
# 必要なもの：pip install fonttools skia-pathops
#
# 元の Tools/FontSource/Cinzel.ttf は「可変フォント」（太さを自由に変えられる形式）で、
# 文字の輪郭が重なったまま作られている。Windows の文字の描画（GDI）はこの形式が苦手で、
# 「M」などの輪郭が重なる所が、しま模様に欠けて表示されてしまう。
# そこで、太さを1つに決めた普通のフォント（静的フォント）にして、重なった輪郭を1つにまとめる。
# ※Cinzel は SIL Open Font License なので、作り直して同梱してよい（名前は元と変える）

import os
from fontTools.ttLib import TTFont
from fontTools.varLib import instancer

WEIGHT = 400					# 太さ（400〜900。今の見出しと同じ 400）
FAMILY = "Cinzel Static"		# 新しいフォントの名前（UIText.cpp の kHeadingFontName と合わせる）


def main():
	root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
	src = os.path.join(root, "Tools", "FontSource", "Cinzel.ttf")	# 元の可変フォント（ゲームでは使わないので Asset には置かない）
	dst = os.path.join(root, "Asset", "Font", "CinzelStatic.ttf")

	font = TTFont(src)
	# 太さを決めて静的フォントにし、重なった輪郭を1つにまとめる
	font = instancer.instantiateVariableFont(font, {"wght": WEIGHT}, overlap=instancer.OverlapMode.REMOVE)

	# フォントの名前を変える（元の Cinzel と区別するため。OFL の決まりでもある）
	name = font["name"]
	for record in list(name.names):
		if record.nameID in (16, 17, 25):
			name.removeNames(nameID=record.nameID)
	name.setName(FAMILY, 1, 3, 1, 0x409)
	name.setName("Regular", 2, 3, 1, 0x409)
	name.setName(FAMILY, 4, 3, 1, 0x409)
	name.setName(FAMILY.replace(" ", "") + "-Regular", 6, 3, 1, 0x409)
	for record in list(name.names):
		if record.platformID != 3:
			name.removeNames(platformID=record.platformID)

	font.save(dst)
	print("作成:", os.path.normpath(dst))


if __name__ == "__main__":
	main()
