# 英数字だけのオリジナルフォント「Braver Stone」を作るスクリプト
# 使い方：pip install fonttools shapely
#         python Tools/MakeFont.py
# Asset/Font/BraverStone.ttf を作る
#
# デザイン：石に彫ったような、角を斜めに落とした（八角形の）文字
#   ・文字は「線の道すじ（点を結んだ折れ線）」で決めて、太さを付けて形にする
#   ・小文字は、大文字を少し小さくした形（スモールキャピタル）にする
#   ・日本語の文字は入っていない（英数字と記号だけ）

import os

from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen
from shapely.geometry import LineString, Polygon
from shapely.geometry.polygon import orient
from shapely.ops import unary_union

FAMILY = "Braver Stone"
UPM = 1000			# 1文字の箱の大きさ（フォントの単位）
CAP = 700			# 大文字の高さ
STROKE = 104		# 線の太さ
SIDE = 70			# 文字の左右の余白
C = 110				# 角を斜めに落とす量
SMALL = 0.8			# 小文字（スモールキャピタル）の大きさ


def octagon(w, h, c=C, y0=0):
	"""角を落とした四角（O や 0 など）"""
	return [(c, y0), (w - c, y0), (w, y0 + c), (w, y0 + h - c), (w - c, y0 + h), (c, y0 + h), (0, y0 + h - c), (0, y0 + c), (c, y0)]


def glyph_defs():
	"""文字ごとの「幅」と「線の道すじ」。座標は（左下が原点、上が＋）"""
	W = 520
	H = CAP
	M = H / 2
	g = {}

	g["A"] = (560, [[(0, 0), (280, H), (560, 0)], [(110, 250), (450, 250)]])
	g["B"] = (W, [[(0, 0), (0, H), (W - 140, H), (W - 40, H - 100), (W - 40, M + 90), (W - 140, M), (0, M)],
				  [(W - 140, M), (W, M - 90), (W, 100), (W - 110, 0), (0, 0)]])
	g["C"] = (W, [[(W, H - 110), (W - C, H), (C, H), (0, H - C), (0, C), (C, 0), (W - C, 0), (W, 110)]])
	g["D"] = (W, [[(0, 0), (0, H), (W - 170, H), (W, H - 170), (W, 170), (W - 170, 0), (0, 0)]])
	g["E"] = (480, [[(480, H), (0, H), (0, 0), (480, 0)], [(0, M), (400, M)]])
	g["F"] = (470, [[(470, H), (0, H), (0, 0)], [(0, M), (390, M)]])
	g["G"] = (W, [[(W, H - 110), (W - C, H), (C, H), (0, H - C), (0, C), (C, 0), (W - C, 0), (W, C), (W, 320), (270, 320)]])
	g["H"] = (W, [[(0, 0), (0, H)], [(W, 0), (W, H)], [(0, M), (W, M)]])
	g["I"] = (300, [[(150, 0), (150, H)], [(0, H), (300, H)], [(0, 0), (300, 0)]])
	g["J"] = (480, [[(480, H), (480, C), (480 - C, 0), (C, 0), (0, C), (0, 220)]])
	g["K"] = (W, [[(0, 0), (0, H)], [(W, H), (40, 300)], [(210, 430), (W, 0)]])
	g["L"] = (460, [[(0, H), (0, 0), (460, 0)]])
	g["M"] = (660, [[(0, 0), (0, H), (330, 260), (660, H), (660, 0)]])
	g["N"] = (W, [[(0, 0), (0, H), (W, 0), (W, H)]])
	g["O"] = (560, [octagon(560, H)])
	g["P"] = (W, [[(0, 0), (0, H), (W - C, H), (W, H - C), (W, 430), (W - C, 320), (0, 320)]])
	g["Q"] = (560, [octagon(560, H), [(300, 190), (600, -90)]])
	g["R"] = (W, [[(0, 0), (0, H), (W - C, H), (W, H - C), (W, 430), (W - C, 320), (0, 320)], [(230, 320), (W, 0)]])
	g["S"] = (W, [[(W, H - 110), (W - C, H), (C, H), (0, H - C), (0, 470), (C, 375), (W - C, 325), (W, 230), (W, C), (W - C, 0), (C, 0), (0, 110)]])
	g["T"] = (540, [[(0, H), (540, H)], [(270, H), (270, 0)]])
	g["U"] = (W, [[(0, H), (0, C), (C, 0), (W - C, 0), (W, C), (W, H)]])
	g["V"] = (560, [[(0, H), (280, 0), (560, H)]])
	g["W"] = (760, [[(0, H), (170, 0), (380, 460), (590, 0), (760, H)]])
	g["X"] = (540, [[(0, H), (540, 0)], [(0, 0), (540, H)]])
	g["Y"] = (540, [[(0, H), (270, 330), (540, H)], [(270, 330), (270, 0)]])
	g["Z"] = (500, [[(0, H), (500, H), (0, 0), (500, 0)]])

	D = 480
	g["0"] = (D, [octagon(D, H), [(130, 170), (D - 130, H - 170)]])
	g["1"] = (340, [[(20, 540), (200, H), (200, 0)], [(20, 0), (340, 0)]])
	g["2"] = (D, [[(0, H - C), (C, H), (D - C, H), (D, H - C), (D, 440), (0, 90), (0, 0), (D, 0)]])
	g["3"] = (D, [[(0, H - C), (C, H), (D - C, H), (D, H - C), (D, 460), (D - C, M), (130, M)],
				  [(D - C, M), (D, 260), (D, C), (D - C, 0), (C, 0), (0, C)]])
	g["4"] = (D, [[(340, 0), (340, H), (0, 220), (D, 220)]])
	g["5"] = (D, [[(D, H), (0, H), (0, 390), (D - C, 390), (D, 290), (D, C), (D - C, 0), (C, 0), (0, C)]])
	g["6"] = (D, [[(D, H - C), (D - C, H), (C, H), (0, H - C), (0, C), (C, 0), (D - C, 0), (D, C), (D, 290), (D - C, 390), (0, 390)]])
	g["7"] = (D, [[(0, H), (D, H), (140, 0)]])
	g["8"] = (D, [octagon(D - 40, H - M + 10, c=95, y0=M - 10)[:], octagon(D, M + 10)])
	g["8"] = (D, [[(p[0] + 20, p[1]) for p in octagon(D - 40, H - M + 10, c=95, y0=M - 10)], octagon(D, M + 10)])
	g["9"] = (D, [[(0, C), (C, 0), (D - C, 0), (D, C), (D, H - C), (D - C, H), (C, H), (0, H - C), (0, 410), (C, 310), (D, 310)]])

	# 記号
	g["-"] = (300, [[(0, 320), (300, 320)]])
	g["+"] = (440, [[(0, 320), (440, 320)], [(220, 100), (220, 540)]])
	g["/"] = (380, [[(0, -40), (380, H + 40)]])
	g[":"] = (120, [[(60, 80), (60, 120)], [(60, 420), (60, 460)]])
	g["."] = (120, [[(60, 30), (60, 70)]])
	g[","] = (140, [[(80, 70), (80, 20), (20, -110)]])
	g["!"] = (120, [[(60, H), (60, 230)], [(60, 30), (60, 70)]])
	g["?"] = (440, [[(0, H - C), (C, H), (440 - C, H), (440, H - C), (440, 470), (220, 330), (220, 230)], [(220, 30), (220, 70)]])
	g["'"] = (120, [[(60, H), (60, 500)]])
	g["("] = (260, [[(260, H + 40), (60, H - 160), (60, 120), (260, -80)]])
	g[")"] = (260, [[(0, H + 40), (200, H - 160), (200, 120), (0, -80)]])
	g["%"] = (620, [[(40, -10), (580, H + 10)], [p for p in octagon(200, 230, c=60, y0=H - 230)],
				  [(p[0] + 420, p[1]) for p in octagon(200, 230, c=60)]])
	g["#"] = (560, [[(150, 0), (230, H)], [(330, 0), (410, H)], [(20, 470), (540, 470)], [(20, 230), (540, 230)]])
	g["&"] = (580, [[(580, 0), (90, 430), (30, 520), (30, 620), (110, H), (300, H), (360, 620), (360, 530), (0, 230), (0, C), (C, 0), (360, 0), (580, 260)]])
	return g


def build_outline(paths, scale=1.0):
	"""折れ線に太さを付けて、1つの形にまとめる"""
	shapes = []
	for path in paths:
		pts = [(x * scale, y * scale) for x, y in path]
		line = LineString(pts)
		# 端は四角く、曲がり角はとがらせる（石に彫った角ばった感じ）
		shapes.append(line.buffer(STROKE / 2, cap_style=3, join_style=2, mitre_limit=2.2))
	return unary_union(shapes)


def draw_shape(shape, offset_x, pen):
	"""形をフォントの点（TrueType は外側の線が時計回り）にして書き込む"""
	polys = [shape] if shape.geom_type == "Polygon" else list(shape.geoms)
	for poly in polys:
		poly = orient(poly, sign=-1.0)	# 外側を時計回り、穴を反時計回りにする
		for ring in [poly.exterior] + list(poly.interiors):
			pts = [(round(x + offset_x), round(y)) for x, y in list(ring.coords)[:-1]]
			# 同じ点が続くとフォントが壊れるので取り除く
			clean = []
			for p in pts:
				if not clean or clean[-1] != p:
					clean.append(p)
			if len(clean) >= 2 and clean[0] == clean[-1]:
				clean.pop()
			if len(clean) < 3:
				continue
			pen.moveTo(clean[0])
			for p in clean[1:]:
				pen.lineTo(p)
			pen.closePath()


def main():
	defs = glyph_defs()

	# 小文字は、大文字を小さくしたもの
	for ch in "ABCDEFGHIJKLMNOPQRSTUVWXYZ":
		w, paths = defs[ch]
		defs[ch.lower()] = (w * SMALL, [[(x * SMALL, y * SMALL) for x, y in p] for p in paths])

	glyph_order = [".notdef", "space"]
	cmap = {32: "space"}
	glyphs = {}
	metrics = {}

	# .notdef（無い文字の代わりに出す四角）
	pen = TTGlyphPen(None)
	draw_shape(Polygon([(SIDE, 0), (SIDE + 400, 0), (SIDE + 400, CAP), (SIDE, CAP)]).difference(
		Polygon([(SIDE + 80, 80), (SIDE + 320, 80), (SIDE + 320, CAP - 80), (SIDE + 80, CAP - 80)])), 0, pen)
	glyphs[".notdef"] = pen.glyph()
	metrics[".notdef"] = (400 + SIDE * 2, SIDE)

	pen = TTGlyphPen(None)
	glyphs["space"] = pen.glyph()
	metrics["space"] = (300, 0)

	for ch, (width, paths) in defs.items():
		name = "uni%04X" % ord(ch)
		shape = build_outline(paths)
		minx = shape.bounds[0]
		offset = SIDE - minx	# 左の余白をそろえる
		pen = TTGlyphPen(None)
		draw_shape(shape, offset, pen)
		glyphs[name] = pen.glyph()
		advance = int(round(width + STROKE + SIDE * 2))
		metrics[name] = (advance, SIDE)
		glyph_order.append(name)
		cmap[ord(ch)] = name

	fb = FontBuilder(UPM, isTTF=True)
	fb.setupGlyphOrder(glyph_order)
	fb.setupCharacterMap(cmap)
	fb.setupGlyf(glyphs)
	fb.setupHorizontalMetrics(metrics)
	fb.setupHorizontalHeader(ascent=900, descent=-200)
	fb.setupNameTable({
		"familyName": FAMILY,
		"styleName": "Regular",
		"uniqueFontIdentifier": FAMILY.replace(" ", "") + "-Regular",
		"fullName": FAMILY + " Regular",
		"psName": FAMILY.replace(" ", "") + "-Regular",
		"version": "Version 1.000",
	})
	fb.setupOS2(sTypoAscender=900, sTypoDescender=-200, sTypoLineGap=0, usWinAscent=950, usWinDescent=250,
		sCapHeight=CAP, sxHeight=int(CAP * SMALL), usWeightClass=400, fsType=0)
	fb.setupPost()

	root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
	out_path = os.path.join(root, "Asset", "Font", "BraverStone.ttf")
	fb.save(out_path)
	print("作成:", os.path.normpath(out_path), "（", len(defs), "文字 ）")


if __name__ == "__main__":
	main()
