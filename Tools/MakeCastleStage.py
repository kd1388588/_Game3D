# ステージのモデル（部屋を通路でつないだ、夜の廃墟の城）を作るスクリプト
# 使い方：python Tools/MakeCastleStage.py
# Asset/Models/GameObject/Stage/RuinedRoad/ に、今までと同じ名前で書き出す
#   RuinedRoad.gltf（見た目と当たり判定）、RuinedRoadCollision.gltf（柱のまわりの見えない壁）、
#   RuinedRoadLights.csv（松明などの光の位置）
#   （ゲームの側のクラス名・ファイル名を変えなくて済むように、同じ名前にしている）
# 一緒に、上から見た地図（Docs/StageMap.png）も書き出す
#
# 部品（床・壁の箱・松明・かがり火・燭台・テクスチャ・glTF の書き出し）は MakeStageModel.py の物を使う
# （MakeStageModel.py は前のステージ（一本道）を作るスクリプト。今は部品の置き場として使っている）
#
# ■ 部屋の並び（モデルの座標。ゲームの中ではステージの拡大率 1.25 倍。+Z が奥、+X が右）
#
#                    ┌──── B ボスの部屋 ────┐
#                    └──────────┬──────────┘   ← 鍵の扉（N の通路の出口）
#   ┌ K 鍵の部屋 ┐══════════ T ═══════════┌ R 右の部屋 ┐
#   └────────────┘                         └─────┬──────┘
#               ┌ H 真ん中の部屋 ┐══ E ══╗       │ V（中ボスを倒すまでバリア）
#               └────────────────┘       ╚═══════╡
#   ┌ S スタート ┐═ A・B ═╝                    ┌ M 中ボス ┐
#   └────────────┘                             └──────────┘
#
#   S スタート → A・B の通路 → H 真ん中（敵なし）→ E の通路 → 分かれ道
#   → 下へ：M 中ボスの部屋（倒すと、分かれ道の上のバリアが開く）
#   → 上へ：V の通路 → R 右の部屋 → T の通路 → K 鍵の部屋（全員倒すと鍵が出る）
#   → T の通路の真ん中から N の通路 → 鍵の扉 → B ボスの部屋
#
# ■ 作り方（グリッド）
#   1. 歩ける所（部屋と通路の長方形）を、1m のマス目に塗る
#   2. 床：歩けるマスに石畳を敷く
#   3. 壁：歩けるマスのまわり 2 マス（斜めも）を「壁のマス」にして、箱を積む
#      → 通路と部屋のつなぎ目・曲がり角・分かれ道も、すき間なく自動で囲める
#      → 部屋の入口は、通路の歩けるマスが続いているので、自動で穴が開く
#   4. 壁の高さは、となりの部屋（通路）の高さを使う（部屋の壁は高く、通路の壁は低い）
#   5. 入口の上には石の梁（まぐさ）と両わきの太い柱を置く
#
# ■ ステージを作る時の注意（Docs/CurrentState.txt の 3-2）
#   ・地面はゲーム側の Ground が広く敷く（Ground.cpp）。壁の外にも地面がある
#   ・壁は 2m の厚さ（ゲームでは 2.5m）。薄いと、勢いよく動いた時にすり抜ける
#   ・壁の一番低い所も、ジャンプで乗り越えられない高さ（5m 以上）にする

import math
import os
import random

import numpy as np
from PIL import Image, ImageDraw

import MakeStageModel as base
from MakeStageModel import (MeshBuilder, add_wall_torch, add_brazier, add_candelabra, build_wall_box,
	write_gltf, write_png, make_stone_texture, make_door_texture, make_iron_texture,
	STONE_TILE_METER, PAVE_TILE_METER, ROAD_TOP_Y, PILLAR_GUARD_MARGIN, PILLAR_GUARD_HEIGHT)

SEED = 1112
STAGE_SCALE = 1.25			# ゲームの中での拡大率（RuinedRoad.cpp・StageLights.cpp と同じ）

WALL_CELLS = 2				# 壁の厚さ（マス）
DOOR_HEIGHT = 5.4			# 入口の高さ（この上に石の梁）
CORRIDOR_WALL_HEIGHT = 6.0	# 通路の壁の高さ

# 歩ける所：(名前, x0, x1, z0, z1, 壁の高さ)。座標はすべて偶数（石畳 2m を、すき間なく並べるため）
ROOMS = {
	"S": (-36, -16,  -8,   8,  8.0),	# スタート
	"H": (-12,  12,  16,  36,  9.0),	# 真ん中（敵なし）
	"M": ( 22,  46, -14,   8, 10.0),	# 中ボス
	"R": ( 24,  44,  48,  68,  9.0),	# 右の部屋
	"K": (-44, -24,  48,  68,  9.0),	# 鍵の部屋
	"B": (-18,  18,  78, 108, 13.0),	# ボス
}
CORRIDORS = {
	"A": (-29, -23,   8,  30),		# スタートの上の入口から上へ
	"Bc": (-29, -12,  24,  30),		# 右へ曲がって、真ん中の部屋の左の入口へ
	"E": ( 12,  37,  24,  30),		# 真ん中の部屋の右の入口から、分かれ道へ
	"V": ( 31,  37,   8,  48),		# 中ボスの部屋の上の入口から、右の部屋の下の入口へ
	"T": (-24,  24,  56,  62),		# 鍵の部屋と右の部屋をつなぐ
	"N": ( -3,   3,  62,  78),		# T の真ん中から、ボスの部屋の入口へ
}
# 入口（石の梁を置く所）：(部屋の名前, 向き, 開いている所の始まり, 終わり, 壁の線の位置)
#   向き "z"：壁が x の向きに伸びている（上・下の壁）。"x"：壁が z の向きに伸びている（左・右の壁）
DOORWAYS = [
	("S", "z", -29, -23,   8),		# スタートの上
	("H", "x",  24,  30, -12),		# 真ん中の左
	("H", "x",  24,  30,  12),		# 真ん中の右
	("M", "z",  31,  37,   8),		# 中ボスの上
	("R", "z",  31,  37,  48),		# 右の部屋の下
	("R", "x",  56,  62,  24),		# 右の部屋の左
	("K", "x",  56,  62, -24),		# 鍵の部屋の右
	("B", "z",  -3,   3,  78),		# ボスの部屋の下（鍵の扉）
]

# ゲーム側に書く位置（モデルの座標。GameScene.cpp では × 1.25 する）
PLAYER_START = (-26.0, -3.0)		# スタートの部屋の下の方（上の入口を向いて始まる）
KEY_POS = (-34.0, 58.0)			# 鍵が出る所（鍵の部屋の真ん中の床の上。ジャンプしなくても拾えるように、台には置かない）
FOUNTAIN_POS = (34.0, -10.0)		# 回復の泉（中ボスの部屋の奥。入口の反対側）
FOUNTAIN_RADIUS = 1.8			# 泉の器の半径
BOSS_THRONE_Z = 104.0


def build_grid():
	"""マス目に、歩ける所（部屋・通路の名前）を塗る"""
	rects = [(k, *v[:4]) for k, v in ROOMS.items()] + [(k, *v) for k, v in CORRIDORS.items()]
	xs = [r[1] for r in rects] + [r[2] for r in rects]
	zs = [r[3] for r in rects] + [r[4] for r in rects]
	pad = WALL_CELLS + 2
	gx0, gx1 = min(xs) - pad, max(xs) + pad
	gz0, gz1 = min(zs) - pad, max(zs) + pad
	w, h = gx1 - gx0, gz1 - gz0
	grid = np.full((w, h), "", dtype=object)
	# 通路を先に、部屋を後に塗る（重なった所は部屋になる＝部屋の壁の高さになる）
	for k, x0, x1, z0, z1 in [r for r in rects if r[0] in CORRIDORS] + [r for r in rects if r[0] in ROOMS]:
		grid[x0 - gx0:x1 - gx0, z0 - gz0:z1 - gz0] = k
	return grid, gx0, gz0


def region_height(name):
	if name in ROOMS:
		return ROOMS[name][4]
	return CORRIDOR_WALL_HEIGHT


def build_floor_from_grid(road, rng, grid, gx0, gz0):
	"""歩けるマスに石畳を敷く（2m の板。座標が偶数なので、2×2 マスずつ必ずそろう）"""
	w, h = grid.shape
	for ix in range(0, w, 2):
		for iz in range(0, h, 2):
			x, z = gx0 + ix, gz0 + iz
			cells = grid[ix:ix + 2, iz:iz + 2]
			if not all(c != "" for c in cells.flatten()):
				# 2×2 のうち一部だけ歩ける所は、1 マスずつ
				for dx in range(2):
					for dz in range(2):
						if ix + dx < w and iz + dz < h and grid[ix + dx, iz + dz] != "":
							shade = rng.uniform(0.7, 0.92)
							road.add_box((x + dx + 0.5, ROAD_TOP_Y - 0.1, z + dz + 0.5), (1.0, 0.2, 1.0), PAVE_TILE_METER,
								shade=shade, bottom_shade=shade * 0.6)
				continue
			name = cells[0, 0]
			lo, hi = (0.62, 0.85) if name in ("B", "M") else (0.72, 0.95)	# ボス・中ボスの部屋は少し暗い石
			shade = rng.uniform(lo, hi)
			road.add_box((x + 1.0, ROAD_TOP_Y + rng.uniform(-0.02, 0.0) - 0.1, z + 1.0), (2.0, 0.2, 2.0), PAVE_TILE_METER,
				shade=shade, bottom_shade=shade * 0.6)


def build_walls_from_grid(stone, rng, grid, gx0, gz0):
	"""歩けるマスのまわり（斜めも含めて WALL_CELLS マス）を壁にして、箱を積む"""
	w, h = grid.shape
	walk = grid != ""
	wall_h = np.zeros((w, h))
	r = WALL_CELLS
	for ix in range(w):
		for iz in range(h):
			if walk[ix, iz]:
				continue
			best = 0.0
			for dx in range(-r, r + 1):
				for dz in range(-r, r + 1):
					jx, jz = ix + dx, iz + dz
					if 0 <= jx < w and 0 <= jz < h and walk[jx, jz]:
						best = max(best, region_height(grid[jx, jz]))
			wall_h[ix, iz] = best

	# 横（x）の向きに並んだ壁のマスを 2 マスずつまとめて、1 つの箱にする（上の端はギザギザに崩す）
	for iz in range(h):
		ix = 0
		while ix < w:
			if wall_h[ix, iz] <= 0:
				ix += 1
				continue
			jx = ix + 1
			while jx < w and jx - ix < 2 and wall_h[jx, iz] == wall_h[ix, iz]:
				jx += 1
			height = wall_h[ix, iz] + rng.uniform(-1.2, 0.4)
			x0, x1 = gx0 + ix, gx0 + jx
			z0, z1 = gz0 + iz, gz0 + iz + 1
			build_wall_box(stone, rng, x0, x1, z0 - 0.01, z1 + 0.01, 0.0, height, broken=0.7,
				shade=rng.uniform(0.72, 0.92))
			ix = jx
	return wall_h


def build_doorways(stone, rng):
	"""入口の上の石の梁と、両わきの太い柱"""
	for room, axis, a, b, line in DOORWAYS:
		top = ROOMS[room][4]
		t = WALL_CELLS
		# 壁の線は、部屋の外側へ t マスの所まで（部屋の端の線から外へ）
		if axis == "z":
			room_z0, room_z1 = ROOMS[room][2], ROOMS[room][3]
			z0, z1 = (line - t, line) if line == room_z0 else (line, line + t)
			stone.add_box(((a + b) / 2, (DOOR_HEIGHT + top) / 2, (z0 + z1) / 2), (b - a + 0.1, top - DOOR_HEIGHT, t),
				STONE_TILE_METER, shade=0.88, bottom_shade=0.6)
			for x in (a - 0.5, b + 0.5):
				stone.add_box((x, (top + 0.8) / 2, (z0 + z1) / 2), (1.2, top + 0.8, t + 0.8), STONE_TILE_METER,
					shade=0.95, bottom_shade=0.5)
				base.pillar_guards.append((x, (z0 + z1) / 2, 1.0))
		else:
			room_x0 = ROOMS[room][0]
			x0, x1 = (line - t, line) if line == room_x0 else (line, line + t)
			stone.add_box(((x0 + x1) / 2, (DOOR_HEIGHT + top) / 2, (a + b) / 2), (t, top - DOOR_HEIGHT, b - a + 0.1),
				STONE_TILE_METER, shade=0.88, bottom_shade=0.6)
			for z in (a - 0.5, b + 0.5):
				stone.add_box(((x0 + x1) / 2, (top + 0.8) / 2, z), (t + 0.8, top + 0.8, 1.2), STONE_TILE_METER,
					shade=0.95, bottom_shade=0.5)
				base.pillar_guards.append(((x0 + x1) / 2, z, 1.0))


def is_doorway(room, side, pos):
	"""部屋のその辺の、その位置が入口（または入口のすぐ近く）か"""
	x0, x1, z0, z1, _ = ROOMS[room]
	for r, axis, a, b, line in DOORWAYS:
		if r != room:
			continue
		if side in ("south", "north") and axis == "z" and line == (z0 if side == "south" else z1) and a - 2.5 <= pos <= b + 2.5:
			return True
		if side in ("west", "east") and axis == "x" and line == (x0 if side == "west" else x1) and a - 2.5 <= pos <= b + 2.5:
			return True
	return False


def add_room_torches(stone, iron, room, step=7.0, y=2.8):
	"""部屋の壁の内側に、松明を同じ間隔で並べる（入口のまわりは避ける）"""
	x0, x1, z0, z1, _ = ROOMS[room]
	for side in ("south", "north", "west", "east"):
		if side in ("south", "north"):
			a0, a1 = x0, x1
		else:
			a0, a1 = z0, z1
		n = max(1, int((a1 - a0) / step))
		for i in range(n):
			p = a0 + (a1 - a0) * (i + 0.5) / n
			if is_doorway(room, side, p):
				continue
			if side == "south":
				add_torch_if_wall(stone, iron, p, y, z0, 0.0, 1.0)
			elif side == "north":
				add_torch_if_wall(stone, iron, p, y, z1, 0.0, -1.0)
			elif side == "west":
				add_torch_if_wall(stone, iron, x0, y, p, 1.0, 0.0)
			else:
				add_torch_if_wall(stone, iron, x1, y, p, -1.0, 0.0)


# 松明を置く前に、後ろに壁があるかを調べるためのマス目（main で入れる）
GRID = None
GRID_X0 = 0
GRID_Z0 = 0


def has_wall_behind(x, z, out_x, out_z):
	"""壁の面 (x, z) の、外側（out の反対の向き）0.5m の所が、歩けないマス（＝壁）か"""
	if GRID is None:
		return True
	wx, wz = x - out_x * 0.5, z - out_z * 0.5
	ix, iz = int(math.floor(wx)) - GRID_X0, int(math.floor(wz)) - GRID_Z0
	w, h = GRID.shape
	if not (0 <= ix < w and 0 <= iz < h):
		return False
	# 松明の金具の幅（約 1m）の分、左右も壁かを調べる（通路のつなぎ目の、壁の端に付かないように）
	for d in (-1.0, 0.0, 1.0):
		jx = int(math.floor(wx + out_z * d)) - GRID_X0
		jz = int(math.floor(wz + out_x * d)) - GRID_Z0
		if not (0 <= jx < w and 0 <= jz < h) or GRID[jx, jz] != "":
			return False
	return True


def add_torch_if_wall(stone, iron, x, y, z, out_x, out_z):
	"""後ろに壁がある時だけ、壁掛け松明を付ける（通路のつなぎ目など、壁の無い所に浮かないように）"""
	if has_wall_behind(x, z, out_x, out_z):
		add_wall_torch(stone, iron, x, y, z, out_x, out_z)


def add_corridor_torches(stone, iron, name, step=8.0, y=2.4):
	"""通路の片側の壁に、松明を並べる（交互に左右を変える）"""
	x0, x1, z0, z1 = CORRIDORS[name]
	along_z = (z1 - z0) > (x1 - x0)
	length = (z1 - z0) if along_z else (x1 - x0)
	n = max(1, int(length / step))
	for i in range(n):
		p = (z0 if along_z else x0) + length * (i + 0.5) / n
		side = 1 if i % 2 == 0 else -1
		if along_z:
			x = x0 if side < 0 else x1
			add_torch_if_wall(stone, iron, x, y, p, -side * 1.0, 0.0)
		else:
			z = z0 if side < 0 else z1
			add_torch_if_wall(stone, iron, p, y, z, 0.0, -side * 1.0)


def add_pillar(stone, rng, x, z, broken=False, square=False, height=None):
	"""柱（台座つき）。broken なら途中で折れている。square なら四角い柱"""
	stone.add_box((x, 0.3, z), (2.0, 0.6, 2.0), STONE_TILE_METER, shade=0.85, bottom_shade=0.45)
	h = height if height else (rng.uniform(2.0, 4.5) if broken else rng.uniform(8.0, 10.0))
	if square:
		stone.add_box((x, 0.6 + h / 2, z), (1.4, h, 1.4), STONE_TILE_METER, shade=0.9, bottom_shade=0.5,
			top_offsets=[rng.uniform(-0.6, 0.0) for _ in range(4)] if broken else None)
	else:
		stone.add_column((x, z), 0.6, h, 0.75, STONE_TILE_METER, broken=0.6 if broken else 0.0, rng=rng)
		if not broken:
			stone.add_box((x, 0.6 + h + 0.3, z), (1.9, 0.6, 1.9), STONE_TILE_METER, shade=0.9, bottom_shade=0.7)
	base.pillar_guards.append((x, z, 2.0))


def build_fountain(stone, rng):
	"""回復の泉：八角形の石の器と、真ん中の細い柱・上の小さな皿（水と光はゲーム側の HealFountain が描く）"""
	fx, fz = FOUNTAIN_POS
	r = FOUNTAIN_RADIUS
	stone.add_column((fx, fz), 0.0, 0.25, r + 0.4, STONE_TILE_METER, sides=8, shade=0.7)		# 下の段
	stone.add_column((fx, fz), 0.25, 0.45, r, STONE_TILE_METER, sides=8, shade=0.9)			# 器（上の面に水を描く）
	stone.add_column((fx, fz), 0.7, 1.0, 0.3, STONE_TILE_METER, sides=8, shade=0.95)			# 真ん中の柱
	stone.add_column((fx, fz), 1.7, 0.15, 0.7, STONE_TILE_METER, sides=8, shade=0.95)			# 上の小さな皿
	base.pillar_guards.append((fx, fz, (r + 0.4) * 2))


def decorate(road, stone, iron, rng):
	"""部屋ごとの飾り（柱・かがり火・燭台・鍵の台・玉座）と光"""
	# S スタート：上の入口の両わきにかがり火（図の ┃┃）
	for x in (-31.5, -20.5):
		add_brazier(stone, iron, x, 4.5, 1.1)
	add_room_torches(stone, iron, "S", step=8.0)

	# H 真ん中：4 本の柱（折れている）と、真ん中の丸い台とかがり火
	for x in (-6.0, 6.0):
		for z in (20.5, 31.5):
			add_pillar(stone, rng, x, z, broken=True)
	road.add_column((0.0, 26.0), ROAD_TOP_Y - 0.1, 0.3, 3.0, PAVE_TILE_METER, sides=16, shade=0.7)
	add_brazier(stone, iron, 0.0, 26.0, 1.4)
	add_room_torches(stone, iron, "H", step=8.0)

	# M 中ボス：広い広場。四隅にかがり火、壁ぎわに折れた柱
	x0, x1, z0, z1, _ = ROOMS["M"]
	for x in (x0 + 3.0, x1 - 3.0):
		for z in (z0 + 3.0, z1 - 3.5):
			add_brazier(stone, iron, x, z, 1.3)
	for (x, z) in ((x0 + 3.0, -3.0), (x1 - 3.0, -3.0)):
		add_pillar(stone, rng, x, z, broken=True)
	build_fountain(stone, rng)
	add_room_torches(stone, iron, "M", step=8.0)

	# R 右の部屋：柱 2 本（図の ○）
	for (x, z) in ((29.0, 63.0), (39.0, 53.0)):
		add_pillar(stone, rng, x, z)
	add_room_torches(stone, iron, "R", step=7.0)

	# K 鍵の部屋：四隅に四角い柱（図の □）。真ん中（図の ▲）は、鍵が出る所なので空けておく（鍵はゲーム側で出す）
	x0, x1, z0, z1, _ = ROOMS["K"]
	for x in (x0 + 3.5, x1 - 3.5):
		for z in (z0 + 3.5, z1 - 3.5):
			add_pillar(stone, rng, x, z, square=True)
	kx, kz = KEY_POS
	road.add_column((kx, kz), ROAD_TOP_Y - 0.1, 0.25, 2.2, PAVE_TILE_METER, sides=16, shade=0.6)	# 床の丸い印（乗り越えられる低さ）
	for (x, z) in ((kx - 4.0, kz), (kx + 4.0, kz)):
		add_candelabra(stone, iron, x, z)
	add_room_torches(stone, iron, "K", step=7.0)

	# B ボスの部屋：柱 2 列と燭台、奥に段と玉座、玉座の両わきに大きなかがり火
	x0, x1, z0, z1, _ = ROOMS["B"]
	for z in (z0 + 7.0, z0 + 15.0, z0 + 23.0):
		for sign in (-1, 1):
			x = sign * (x1 - 4.0)
			add_pillar(stone, rng, x, z, height=rng.uniform(10.0, 12.0))
			add_candelabra(stone, iron, x - sign * 2.4, z)
	for i, (w, d) in enumerate(((14.0, 8.0), (10.0, 6.0))):
		y = ROAD_TOP_Y + 0.18 * (i + 1)
		road.add_box((0.0, y - 0.1, z1 - 1.0 - d / 2), (w, 0.2, d), PAVE_TILE_METER, shade=0.8, bottom_shade=0.5)
	ty = ROAD_TOP_Y + 0.36
	tz = BOSS_THRONE_Z
	stone.add_box((0.0, ty + 0.5, tz), (2.4, 1.0, 1.8), STONE_TILE_METER, shade=0.9, bottom_shade=0.6)
	stone.add_box((0.0, ty + 2.4, tz + 0.75), (2.4, 3.8, 0.4), STONE_TILE_METER, shade=0.9, bottom_shade=0.6,
		top_offsets=[0.8, 0.8, 0.8, 0.8])
	for sign in (-1, 1):
		stone.add_box((sign * 1.3, ty + 0.9, tz), (0.3, 0.8, 1.8), STONE_TILE_METER, shade=0.85)
		add_brazier(stone, iron, sign * 5.5, tz, 1.6)
	base.pillar_guards.append((0.0, tz, 2.6))
	add_room_torches(stone, iron, "B", step=8.0, y=3.2)

	# 通路の松明
	for name in CORRIDORS:
		add_corridor_torches(stone, iron, name)


def write_map(path, grid, gx0, gz0, wall_h):
	"""上から見た地図（確認用。1 マス = 6 ピクセル。上が奥（+Z））"""
	s = 6
	w, h = grid.shape
	img = Image.new("RGB", (w * s, h * s), (20, 22, 30))
	d = ImageDraw.Draw(img)
	colors = {"S": (70, 110, 80), "H": (80, 80, 110), "M": (120, 70, 60), "R": (90, 90, 90),
		"K": (110, 100, 50), "B": (120, 50, 60)}
	for ix in range(w):
		for iz in range(h):
			px, py = ix * s, (h - 1 - iz) * s
			c = grid[ix, iz]
			if c:
				col = colors.get(c, (60, 60, 65))
			elif wall_h[ix, iz] > 0:
				v = int(120 + wall_h[ix, iz] * 6)
				col = (v, v, v)
			else:
				continue
			d.rectangle([px, py, px + s - 1, py + s - 1], fill=col)

	def to_px(x, z):
		return ((x - gx0) * s, (h - (z - gz0)) * s)

	for x, _y, z, *_ in base.stage_lights:
		px, py = to_px(x, z)
		d.ellipse([px - 2, py - 2, px + 2, py + 2], fill=(255, 170, 60))
	for name, (x0, x1, z0, z1, _) in ROOMS.items():
		px, py = to_px((x0 + x1) / 2, (z0 + z1) / 2)
		d.text((px - 4, py - 6), name, fill=(255, 255, 255))
	img.save(path)


def main():
	base.stage_lights.clear()
	base.pillar_guards.clear()
	base.discard = MeshBuilder()

	rng = random.Random(SEED)
	np_rng = np.random.default_rng(SEED)
	root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
	out_dir = os.path.join(root, "Asset", "Models", "GameObject", "Stage", "RuinedRoad")
	os.makedirs(out_dir, exist_ok=True)

	global GRID, GRID_X0, GRID_Z0
	grid, gx0, gz0 = build_grid()
	GRID, GRID_X0, GRID_Z0 = grid, gx0, gz0
	road = MeshBuilder()
	stone = MeshBuilder()
	iron = MeshBuilder()

	build_floor_from_grid(road, rng, grid, gx0, gz0)
	wall_h = build_walls_from_grid(stone, rng, grid, gx0, gz0)
	build_doorways(stone, rng)
	decorate(road, stone, iron, rng)

	# 扉（木の板）は使わないので書き出さない（空のメッシュは書き出せない）
	write_gltf(out_dir, "RuinedRoad", [(road, 0), (stone, 1), (iron, 3)])

	guard = MeshBuilder()
	for x, z, size in base.pillar_guards:
		gw = size + PILLAR_GUARD_MARGIN * 2
		guard.add_box((x, PILLAR_GUARD_HEIGHT / 2, z), (gw, PILLAR_GUARD_HEIGHT, gw), STONE_TILE_METER)
	write_gltf(out_dir, "RuinedRoadCollision", [(guard, 1)])

	with open(os.path.join(out_dir, "RuinedRoadLights.csv"), "w", encoding="utf-8") as f:
		f.write("# x,y,z,r,g,b,radius,flame（Tools/MakeCastleStage.py が作る）\n")
		for light in base.stage_lights:
			f.write(",".join(f"{v:.3f}" for v in light) + "\n")

	# テクスチャ（前のステージと同じ作り方・同じ種）
	old_rng = np.random.default_rng(base.SEED)
	pave = make_stone_texture(old_rng, rows=6, cols_per_row=4, base_color=(0.42, 0.42, 0.45),
		grout_color=(0.08, 0.08, 0.09), offset_rows=True, jitter=0.25)
	wall = make_stone_texture(old_rng, rows=8, cols_per_row=3, base_color=(0.5, 0.48, 0.46),
		grout_color=(0.12, 0.11, 0.1), offset_rows=True, jitter=0.12)
	write_png(os.path.join(out_dir, "Pavement.png"), pave)
	write_png(os.path.join(out_dir, "StoneWall.png"), wall)
	write_png(os.path.join(out_dir, "DoorWood.png"), make_door_texture(np.random.default_rng(base.SEED + 2)))
	write_png(os.path.join(out_dir, "Iron.png"), make_iron_texture(np.random.default_rng(base.SEED + 3)))

	write_map(os.path.join(root, "Docs", "StageMap.png"), grid, gx0, gz0, wall_h)

	tri = (len(road.idx) + len(stone.idx) + len(iron.idx)) // 3
	print("光源:", len(base.stage_lights), "個 ／ 柱のまわりの見えない壁:", len(base.pillar_guards), "個 ／ 三角形:", tri, "枚")


if __name__ == "__main__":
	main()
