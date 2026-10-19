# ステージのモデル（廃墟の参道）を作るスクリプト
# 使い方：python Tools/MakeStageModel.py
# Asset/Models/GameObject/Stage/RuinedRoad/ に glTF（.gltf / .bin / テクスチャ）を作る
#
# 中身（スタート地点 z=0 から、大聖堂の廃墟 z=60 へ向かう一本道）
#   ・石畳の道（幅 kRoadWidth）
#   ・道の両側に並ぶ、折れた柱
#   ・その外側の、崩れた低い石壁
#   ・あちこちに転がる瓦礫
#   ・大聖堂の手前の、半分崩れたアーチの門
#   ・道の先の、崩れた大聖堂（ボスと戦う広間。厚い壁・窓の穴・中の柱・奥の半円の壁）
#     ※壁や柱はすべて厚みのある箱で作る（薄い板だと、キャラが勢いよく動いた時にすり抜けるため）
#
# 座標はゲームの座標（左手系：+Z が前）で作り、書き出す時に glTF の座標（右手系）に直す
# 乱数の種を固定しているので、何回実行しても同じ形になる

import json
import math
import os
import random
import struct
import zlib

import numpy as np

SEED = 1018

# ===== 道の大きさ（メートル） =====
ROAD_START_Z = -12.0		# 道の始まり（スタート地点の少し後ろ）
ROAD_END_Z = 52.0			# 道の終わり（大聖堂の手前）
ROAD_WIDTH = 12.0			# 石畳の幅
ROAD_TOP_Y = 0.04			# 石畳の上の面の高さ
PILLAR_X = 7.5				# 柱が並ぶ位置（道の中心からの距離）
PILLAR_STEP = 7.0			# 柱の間隔
WALL_X = 11.0				# 石壁の位置
GATE_Z = 46.0				# 門の位置

# ===== 大聖堂（ボスの広間） =====
NAVE_START_Z = 52.0			# 入口の壁の位置
NAVE_END_Z = 96.0			# 奥の壁の位置（この先に半円の壁）
NAVE_HALF_WIDTH = 15.0		# 広間の幅の半分
NAVE_WALL_THICK = 1.6		# 壁の厚さ
NAVE_BAY = 6.0				# 柱と柱の間隔（窓1つ分）
NAVE_PILLAR_X = 11.5		# 中の柱の位置（ボスの出てくる x=7.5 にかからないように）
ENTRANCE_HALF_WIDTH = 5.0	# 入口の幅の半分

# ===== テクスチャ =====
TEX_SIZE = 512
PAVE_TILE_METER = 3.0		# 石畳のテクスチャ1枚が何メートル分か
STONE_TILE_METER = 2.0		# 石壁・柱のテクスチャ1枚が何メートル分か


# ------------------------------------------------------------
# 形を作る
# ------------------------------------------------------------
class MeshBuilder:
	"""三角形を集めるだけの入れ物（頂点：位置・法線・UV・色）"""

	def __init__(self):
		self.pos = []
		self.nor = []
		self.uv = []
		self.col = []
		self.idx = []

	def add_quad(self, p0, p1, p2, p3, tile, shade=(1.0, 1.0, 1.0, 1.0), shades=None):
		"""p0→p1→p2→p3 の順に並んだ四角形（表から見て反時計回り）を追加する"""
		p = [np.array(v, dtype=np.float64) for v in (p0, p1, p2, p3)]
		n = np.cross(p[1] - p[0], p[3] - p[0])
		length = np.linalg.norm(n)
		if length < 1e-9:
			return
		n /= length

		# 面の向きに合わせて、一番平らに見える方向からUVを貼る（世界の大きさに合わせて繰り返す）
		axis = int(np.argmax(np.abs(n)))
		u_axis, v_axis = {0: (2, 1), 1: (0, 2), 2: (0, 1)}[axis]

		base = len(self.pos)
		for i, v in enumerate(p):
			self.pos.append(v)
			self.nor.append(n)
			self.uv.append((v[u_axis] / tile, -v[v_axis] / tile))
			self.col.append(shades[i] if shades else shade)
		self.idx += [base, base + 1, base + 2, base, base + 2, base + 3]

	def add_box(self, center, size, tile, rot_y=0.0, rot_x=0.0, shade=1.0, bottom_shade=None, top_offsets=None):
		"""箱を追加する。top_offsets で上の4つの角の高さをずらすと、崩れた形になる"""
		cx, cy, cz = center
		hx, hy, hz = size[0] / 2, size[1] / 2, size[2] / 2
		corners = []
		for y in (-hy, hy):
			for (x, z) in ((-hx, -hz), (hx, -hz), (hx, hz), (-hx, hz)):
				corners.append([x, y, z])
		if top_offsets:
			for i in range(4):
				corners[4 + i][1] += top_offsets[i]

		# 回転（X → Y の順）してから移動
		cyr, syr = math.cos(rot_y), math.sin(rot_y)
		cxr, sxr = math.cos(rot_x), math.sin(rot_x)
		out = []
		for x, y, z in corners:
			y, z = y * cxr - z * sxr, y * sxr + z * cxr
			x, z = x * cyr + z * syr, -x * syr + z * cyr
			out.append((x + cx, y + cy, z + cz))

		b = out[0:4]	# 下の4つ
		t = out[4:8]	# 上の4つ
		top = (shade, shade, shade, 1.0)
		low_v = shade if bottom_shade is None else bottom_shade
		low = (low_v, low_v, low_v, 1.0)

		# 上・下
		self.add_quad(t[0], t[3], t[2], t[1], tile, top)
		self.add_quad(b[0], b[1], b[2], b[3], tile, low)
		# 横の4面（下ほど暗くして、地面との境目を落ち着かせる）
		for i in range(4):
			j = (i + 1) % 4
			self.add_quad(b[j], b[i], t[i], t[j], tile, shades=[low, low, top, top])

	def add_column(self, center_xz, base_y, height, radius, tile, sides=8, broken=0.0, rng=None, shade=1.0):
		"""多角形の柱を追加する。broken > 0 なら、上の端をギザギザに折る"""
		cx, cz = center_xz
		ring_b = []
		ring_t = []
		for i in range(sides):
			a = 2 * math.pi * i / sides
			x, z = cx + math.cos(a) * radius, cz + math.sin(a) * radius
			ring_b.append((x, base_y, z))
			jag = rng.uniform(-broken, broken * 0.3) if (broken > 0 and rng) else 0.0
			ring_t.append((x, base_y + height + jag, z))

		low = (shade * 0.55, shade * 0.55, shade * 0.55, 1.0)
		top = (shade, shade, shade, 1.0)
		for i in range(sides):
			j = (i + 1) % sides
			# 横の面（外向き。表から見て反時計回りになる順）
			self.add_quad(ring_b[j], ring_b[i], ring_t[i], ring_t[j], tile, shades=[low, low, top, top])

		# 上のふた（折れた面は中心を少し下げて、えぐれた感じにする）
		center_top = (cx, base_y + height - (broken * 0.4 if broken > 0 else 0.0), cz)
		for i in range(sides):
			j = (i + 1) % sides
			self.add_tri(center_top, ring_t[j], ring_t[i], tile, top)

	def add_tri(self, p0, p1, p2, tile, shade):
		p = [np.array(v, dtype=np.float64) for v in (p0, p1, p2)]
		n = np.cross(p[1] - p[0], p[2] - p[0])
		length = np.linalg.norm(n)
		if length < 1e-9:
			return
		n /= length
		axis = int(np.argmax(np.abs(n)))
		u_axis, v_axis = {0: (2, 1), 1: (0, 2), 2: (0, 1)}[axis]
		base = len(self.pos)
		for v in p:
			self.pos.append(v)
			self.nor.append(n)
			self.uv.append((v[u_axis] / tile, -v[v_axis] / tile))
			self.col.append(shade)
		self.idx += [base, base + 1, base + 2]


def build_road(mesh, rng):
	"""石畳：細かい板を並べ、ところどころ高さを少し変えて、古い道に見せる"""
	half = ROAD_WIDTH / 2
	step = 2.0
	z = ROAD_START_Z
	while z < ROAD_END_Z - 1e-6:
		x = -half
		while x < half - 1e-6:
			# 端のほうは欠けている（石が抜けている）
			edge = abs(x + step / 2) / half
			if edge > 0.75 and rng.random() < 0.35:
				x += step
				continue
			sink = rng.uniform(-0.02, 0.0)
			shade = rng.uniform(0.82, 1.0)
			top = ROAD_TOP_Y + sink
			mesh.add_box((x + step / 2, top - 0.1, z + step / 2), (step, 0.2, step), PAVE_TILE_METER,
				shade=shade, bottom_shade=shade * 0.6)
			x += step
		z += step


def build_pillars(mesh, rng):
	"""道の両側に並ぶ柱（折れているものが多い）"""
	z = ROAD_START_Z + 4.0
	while z < GATE_Z - 4.0:
		for side in (-1, 1):
			x = side * PILLAR_X + rng.uniform(-0.2, 0.2)
			pz = z + rng.uniform(-0.4, 0.4)
			# 台座
			mesh.add_box((x, 0.3, pz), (1.7, 0.6, 1.7), STONE_TILE_METER, rot_y=rng.uniform(-0.05, 0.05),
				shade=0.85, bottom_shade=0.45)

			kind = rng.random()
			if kind < 0.2:
				# 根元だけ残っている
				h = rng.uniform(0.6, 1.4)
				mesh.add_column((x, pz), 0.6, h, 0.6, STONE_TILE_METER, broken=0.35, rng=rng)
				# 倒れた柱の残り
				mesh.add_box((x + side * 1.8, 0.45, pz + rng.uniform(-1, 1)), (1.0, 0.9, 3.2), STONE_TILE_METER,
					rot_y=rng.uniform(-0.6, 0.6), shade=0.8, bottom_shade=0.45)
			elif kind < 0.75:
				# 途中で折れている
				h = rng.uniform(2.5, 5.0)
				mesh.add_column((x, pz), 0.6, h, 0.6, STONE_TILE_METER, broken=0.6, rng=rng)
			else:
				# ほぼ残っている（上に飾り石）
				h = rng.uniform(6.5, 7.5)
				mesh.add_column((x, pz), 0.6, h, 0.6, STONE_TILE_METER)
				mesh.add_box((x, 0.6 + h + 0.25, pz), (1.6, 0.5, 1.6), STONE_TILE_METER, shade=0.9, bottom_shade=0.7)
		z += PILLAR_STEP


def build_walls(mesh, rng):
	"""柱の外側の、崩れた低い石壁（ところどころ途切れている）"""
	seg = 1.6
	for side in (-1, 1):
		z = ROAD_START_Z
		while z < NAVE_START_Z - 1.0:
			if rng.random() < 0.18:
				z += seg * rng.randint(1, 3)	# 壁が崩れて無くなっている所
				continue
			h = rng.uniform(0.8, 2.6)
			offsets = [rng.uniform(-0.5, 0.15) for _ in range(4)]
			mesh.add_box((side * (WALL_X + rng.uniform(-0.15, 0.15)), h / 2, z + seg / 2), (0.9, h, seg),
				STONE_TILE_METER, rot_y=rng.uniform(-0.04, 0.04), shade=rng.uniform(0.7, 0.9), bottom_shade=0.4,
				top_offsets=offsets)
			z += seg


def build_rubble(mesh, rng):
	"""転がっている瓦礫（道の真ん中には置かない。戦闘のじゃまにならないように）"""
	for _ in range(70):
		side = rng.choice((-1, 1))
		x = side * rng.uniform(ROAD_WIDTH / 2 - 0.5, WALL_X + 2.0)
		z = rng.uniform(ROAD_START_Z, ROAD_END_Z)
		s = rng.uniform(0.25, 0.9)
		mesh.add_box((x, s * 0.35, z), (s * rng.uniform(0.8, 1.6), s * 0.7, s * rng.uniform(0.8, 1.4)), STONE_TILE_METER,
			rot_y=rng.uniform(0, math.pi), rot_x=rng.uniform(-0.3, 0.3), shade=rng.uniform(0.6, 0.85), bottom_shade=0.4)


def build_gate(mesh, rng):
	"""大聖堂の手前の、半分崩れたアーチの門"""
	gx = ROAD_WIDTH / 2 + 0.6
	# 左右の太い柱（右はアーチの途中まで残り、左は途中で崩れている）
	for side, h in ((-1, 5.5), (1, 9.0)):
		mesh.add_box((side * gx, 0.5, GATE_Z), (2.6, 1.0, 2.6), STONE_TILE_METER, shade=0.85, bottom_shade=0.4)
		offsets = [rng.uniform(-0.8, 0.0) for _ in range(4)] if side < 0 else None
		mesh.add_box((side * gx, 1.0 + h / 2, GATE_Z), (1.8, h, 1.8), STONE_TILE_METER, shade=0.9, bottom_shade=0.6,
			top_offsets=offsets)

	# アーチ：右の柱から、半円に石を並べる（左半分は崩れて無い）
	radius = gx
	spring_y = 10.0
	stones = 12
	for i in range(stones):
		a0 = math.pi * i / stones
		a1 = math.pi * (i + 1) / stones
		am = (a0 + a1) / 2
		if am > math.pi * 0.55:
			break	# ここから先は崩れている
		x = math.cos(am) * radius
		y = spring_y + math.sin(am) * radius * 0.55
		length = radius * (a1 - a0) * 1.05
		mesh.add_box((x, y, GATE_Z), (length, 1.2, 1.6), STONE_TILE_METER, rot_y=0.0, shade=0.9, bottom_shade=0.7)
		# 石の傾き：半円の接線に合わせる（X軸まわりではなくZ軸まわりなので、頂点を直接回す）
		rotate_last_box_z(mesh, (x, y, GATE_Z), am - math.pi / 2)

	# 門の下に落ちたアーチの石
	for _ in range(6):
		mesh.add_box((rng.uniform(-gx, 0.0), 0.4, GATE_Z + rng.uniform(-2.5, 2.5)), (1.4, 0.8, 1.2), STONE_TILE_METER,
			rot_y=rng.uniform(0, math.pi), rot_x=rng.uniform(-0.25, 0.25), shade=0.75, bottom_shade=0.4)


def build_cathedral(road, stone, rng):
	"""道の先の、崩れた大聖堂（ボスと戦う広間）"""
	hw = NAVE_HALF_WIDTH
	t = NAVE_WALL_THICK

	# 床（石畳）
	step = 2.0
	z = NAVE_START_Z
	while z < NAVE_END_Z + 6.0 - 1e-6:
		x = -hw
		while x < hw - 1e-6:
			shade = rng.uniform(0.75, 0.95)
			top = ROAD_TOP_Y + rng.uniform(-0.02, 0.0)
			road.add_box((x + step / 2, top - 0.1, z + step / 2), (step, 0.2, step), PAVE_TILE_METER,
				shade=shade, bottom_shade=shade * 0.6)
			x += step
		z += step

	def wall_piece(x0, x1, z0, z1, y0, y1, broken=0.0):
		"""壁の一部（厚みのある箱）。broken > 0 なら上をギザギザに崩す"""
		offsets = [rng.uniform(-broken, 0.0) for _ in range(4)] if broken > 0 else None
		stone.add_box(((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), (x1 - x0, y1 - y0, z1 - z0), STONE_TILE_METER,
			shade=rng.uniform(0.8, 0.95), bottom_shade=0.45, top_offsets=offsets)

	def side_wall(x_center):
		"""左右の長い壁：柱の間ごとに、下の壁・窓の穴・上の壁。ところどころ崩れている"""
		x0, x1 = x_center - t / 2, x_center + t / 2
		z = NAVE_START_Z
		while z < NAVE_END_Z - 1e-6:
			z1 = min(z + NAVE_BAY, NAVE_END_Z)
			# 柱型（壁から少し出っ張った太い柱。高さはばらばら）
			ph = rng.uniform(8.0, 13.0)
			stone.add_box((x_center, ph / 2, z), (t + 1.0, ph, 1.6), STONE_TILE_METER, shade=0.9, bottom_shade=0.5,
				top_offsets=[rng.uniform(-1.2, 0.0) for _ in range(4)])

			kind = rng.random()
			if kind < 0.25:
				# 崩れて低くなった壁（乗り越えられない高さは残す）
				wall_piece(x0, x1, z + 0.8, z1 - 0.8, 0.0, rng.uniform(2.0, 3.0), broken=0.8)
			else:
				# 下の壁 → 窓の穴 → 上の壁（上は崩れていることがある）
				wall_piece(x0, x1, z + 0.8, z1 - 0.8, 0.0, 3.0)
				for zz in (z + 0.8, z1 - 1.6):	# 窓の両わきの細い壁
					wall_piece(x0, x1, zz, zz + 0.8, 3.0, 7.0)
				if kind > 0.5:
					wall_piece(x0, x1, z + 0.8, z1 - 0.8, 7.0, rng.uniform(8.5, 10.0), broken=1.2)
			z = z1

	side_wall(-hw)
	side_wall(hw)

	# 入口の壁（真ん中が入口。上は崩れている）
	z0, z1 = NAVE_START_Z - t / 2, NAVE_START_Z + t / 2
	for sign in (-1, 1):
		x_in = sign * ENTRANCE_HALF_WIDTH
		x_out = sign * (hw + t / 2)
		wall_piece(min(x_in, x_out), max(x_in, x_out), z0, z1, 0.0, rng.uniform(7.0, 10.0), broken=2.0)
		# 入口の両わきの太い柱
		stone.add_box((x_in + sign * 0.9, 6.0, NAVE_START_Z), (1.8, 12.0, 2.4), STONE_TILE_METER, shade=0.95, bottom_shade=0.5,
			top_offsets=[rng.uniform(-1.5, 0.0) for _ in range(4)])

	# 奥の半円の壁（後陣）
	apse_r = hw
	segments = 14
	for i in range(segments):
		a0 = math.pi * i / segments
		a1 = math.pi * (i + 1) / segments
		am = (a0 + a1) / 2
		cx = math.cos(am) * apse_r
		cz = NAVE_END_Z + math.sin(am) * apse_r * 0.6
		length = apse_r * (a1 - a0) * 1.15
		h = rng.uniform(4.0, 11.0)
		stone.add_box((cx, h / 2, cz), (length, h, t), STONE_TILE_METER, rot_y=-(am - math.pi / 2) * 0.9,
			shade=rng.uniform(0.8, 0.95), bottom_shade=0.45, top_offsets=[rng.uniform(-1.0, 0.0) for _ in range(4)])
	# 奥の壁と半円の間をふさぐ柱
	for sign in (-1, 1):
		stone.add_box((sign * hw, 5.0, NAVE_END_Z), (t + 1.0, 10.0, 2.0), STONE_TILE_METER, shade=0.9, bottom_shade=0.5)

	# 中の柱（2列。折れているものも）
	z = NAVE_START_Z + 6.0
	while z < NAVE_END_Z - 2.0:
		for sign in (-1, 1):
			x = sign * NAVE_PILLAR_X
			stone.add_box((x, 0.3, z), (2.0, 0.6, 2.0), STONE_TILE_METER, shade=0.85, bottom_shade=0.45)
			if rng.random() < 0.35:
				h = rng.uniform(1.5, 4.0)
				stone.add_column((x, z), 0.6, h, 0.75, STONE_TILE_METER, broken=0.6, rng=rng)
			else:
				h = rng.uniform(8.0, 10.0)
				stone.add_column((x, z), 0.6, h, 0.75, STONE_TILE_METER)
				stone.add_box((x, 0.6 + h + 0.3, z), (1.9, 0.6, 1.9), STONE_TILE_METER, shade=0.9, bottom_shade=0.7)
		z += NAVE_BAY

	# 奥の祭壇（一段高い台。段差は乗り越えられる高さ 0.2 以下にする）
	for i, (w, d) in enumerate(((10.0, 5.0), (8.0, 3.6))):
		y = ROAD_TOP_Y + 0.18 * (i + 1)
		road.add_box((0.0, y - 0.1, NAVE_END_Z + 1.5), (w, 0.2, d), PAVE_TILE_METER, shade=0.8, bottom_shade=0.5)

	# 広間の瓦礫（壁ぞいだけ）
	for _ in range(40):
		side = rng.choice((-1, 1))
		x = side * rng.uniform(hw - 3.5, hw - 1.2)
		zz = rng.uniform(NAVE_START_Z + 2, NAVE_END_Z)
		sz = rng.uniform(0.3, 1.0)
		stone.add_box((x, sz * 0.35, zz), (sz * 1.3, sz * 0.7, sz), STONE_TILE_METER, rot_y=rng.uniform(0, math.pi),
			rot_x=rng.uniform(-0.3, 0.3), shade=rng.uniform(0.6, 0.85), bottom_shade=0.4)


def rotate_last_box_z(mesh, center, angle):
	"""直前に追加した箱（24頂点）を、center を中心に Z 軸まわりで回す"""
	c, s = math.cos(angle), math.sin(angle)
	cx, cy, _ = center
	for i in range(len(mesh.pos) - 24, len(mesh.pos)):
		x, y, z = mesh.pos[i]
		x, y = x - cx, y - cy
		mesh.pos[i] = np.array((x * c - y * s + cx, x * s + y * c + cy, z))
		nx, ny, nz = mesh.nor[i]
		mesh.nor[i] = np.array((nx * c - ny * s, nx * s + ny * c, nz))


# ------------------------------------------------------------
# テクスチャを作る
# ------------------------------------------------------------
def value_noise(size, cells, rng):
	"""なめらかなノイズ（0～1）"""
	grid = rng.random((cells + 1, cells + 1))
	grid[-1, :] = grid[0, :]
	grid[:, -1] = grid[:, 0]	# 端と端がつながるように（繰り返して貼った時に継ぎ目が出ない）
	t = np.linspace(0, cells, size, endpoint=False)
	i = t.astype(int)
	f = t - i
	f = f * f * (3 - 2 * f)
	a = grid[i][:, i]
	b = grid[i][:, i + 1]
	c = grid[i + 1][:, i]
	d = grid[i + 1][:, i + 1]
	fx = f[None, :]
	fy = f[:, None]
	return (a * (1 - fx) + b * fx) * (1 - fy) + (c * (1 - fx) + d * fx) * fy


def fractal_noise(size, rng, octaves=(4, 8, 16, 32, 64)):
	total = np.zeros((size, size))
	amp = 1.0
	norm = 0.0
	for cells in octaves:
		total += value_noise(size, cells, rng) * amp
		norm += amp
		amp *= 0.5
	return total / norm


def make_stone_texture(rng, rows, cols_per_row, base_color, grout_color, offset_rows=True, jitter=0.0):
	"""石を並べた模様（石畳・石壁で共通）。石ごとに色を少し変え、すき間（目地）を暗くする"""
	size = TEX_SIZE
	ys, xs = np.mgrid[0:size, 0:size] / size
	noise = fractal_noise(size, rng)
	fine = fractal_noise(size, rng, octaves=(64, 128))

	# 行ごとに石の幅を決める
	row_h = 1.0 / rows
	row = np.floor(ys / row_h).astype(int)
	shift = np.where((row % 2 == 1) & offset_rows, 0.5 / cols_per_row, 0.0)
	col_w = 1.0 / cols_per_row
	u = (xs + shift) % 1.0
	col = np.floor(u / col_w).astype(int)

	# 石の中での位置（0～1）。端に近いほど目地
	lu = (u % col_w) / col_w
	lv = (ys % row_h) / row_h
	# 石の形を少しゆがませる
	lu = lu + (noise - 0.5) * jitter
	lv = lv + (fine - 0.5) * jitter
	edge = np.minimum(np.minimum(lu, 1 - lu) * col_w, np.minimum(lv, 1 - lv) * row_h)
	grout = np.clip(1.0 - edge / 0.008, 0.0, 1.0)

	# 石ごとの明るさのばらつき
	stone_id = (row * 131 + col * 17) % 997
	tone_table = np.array([rng.uniform(0.75, 1.1) for _ in range(997)])
	tone = tone_table[stone_id]

	# 汚れ（大きなノイズ）とざらつき（細かいノイズ）
	dirt = 0.75 + 0.35 * noise
	grain = 0.85 + 0.3 * fine
	base = np.array(base_color)[None, None, :] * (tone * dirt * grain)[..., None]

	# ひび（細い暗い線）
	crack_n = value_noise(size, 24, rng)
	crack = np.clip(1.0 - np.abs(crack_n - 0.5) / 0.012, 0.0, 1.0) * (noise > 0.55)

	color = base * (1 - grout[..., None]) + np.array(grout_color)[None, None, :] * grout[..., None]
	color *= (1 - 0.6 * crack[..., None])
	return np.clip(color, 0.0, 1.0)


def write_png(path, rgb):
	h, w, _ = rgb.shape
	data = (rgb * 255).astype(np.uint8)
	raw = b"".join(b"\x00" + data[y].tobytes() for y in range(h))

	def chunk(kind, body):
		return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body) & 0xffffffff)

	with open(path, "wb") as f:
		f.write(b"\x89PNG\r\n\x1a\n")
		f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)))
		f.write(chunk(b"IDAT", zlib.compress(raw, 9)))
		f.write(chunk(b"IEND", b""))


# ------------------------------------------------------------
# glTF に書き出す
# ------------------------------------------------------------
def to_gltf_arrays(mesh):
	"""ゲームの座標（左手系）→ glTF の座標（右手系）：Z を反転し、三角形の向きを法線に合わせる"""
	pos = np.array(mesh.pos, dtype=np.float32)
	nor = np.array(mesh.nor, dtype=np.float32)
	pos[:, 2] *= -1
	nor[:, 2] *= -1
	idx = np.array(mesh.idx, dtype=np.uint32).reshape(-1, 3)

	# glTF は「表から見て反時計回り」が表。法線と逆向きの三角形は並びを入れ替える
	p0, p1, p2 = pos[idx[:, 0]], pos[idx[:, 1]], pos[idx[:, 2]]
	face_n = np.cross(p1 - p0, p2 - p0)
	flip = (face_n * nor[idx[:, 0]]).sum(axis=1) < 0
	idx[flip] = idx[flip][:, [0, 2, 1]]

	uv = np.array(mesh.uv, dtype=np.float32)
	col = np.array(mesh.col, dtype=np.float32)
	return pos, nor, uv, col, idx.reshape(-1)


def write_gltf(out_dir, name, meshes):
	"""meshes：[(MeshBuilder, マテリアル番号), ...] を、1つのメッシュの複数のプリミティブとして書き出す"""
	bin_data = bytearray()
	buffer_views = []
	accessors = []
	primitives = []

	def add_view(arr, target):
		while len(bin_data) % 4:
			bin_data.append(0)
		offset = len(bin_data)
		bin_data.extend(arr.tobytes())
		buffer_views.append({"buffer": 0, "byteOffset": offset, "byteLength": arr.nbytes, "target": target})
		return len(buffer_views) - 1

	def add_accessor(arr, comp_type, type_name, target, with_bounds=False):
		view = add_view(arr, target)
		acc = {"bufferView": view, "componentType": comp_type, "count": int(arr.shape[0]), "type": type_name}
		if with_bounds:
			acc["min"] = arr.min(axis=0).tolist()
			acc["max"] = arr.max(axis=0).tolist()
		accessors.append(acc)
		return len(accessors) - 1

	for mesh, material in meshes:
		pos, nor, uv, col, idx = to_gltf_arrays(mesh)
		attrs = {
			"POSITION": add_accessor(pos, 5126, "VEC3", 34962, with_bounds=True),
			"NORMAL": add_accessor(nor, 5126, "VEC3", 34962),
			"TEXCOORD_0": add_accessor(uv, 5126, "VEC2", 34962),
			"COLOR_0": add_accessor(col, 5126, "VEC4", 34962),
		}
		indices = add_accessor(idx, 5125, "SCALAR", 34963)
		primitives.append({"attributes": attrs, "indices": indices, "material": material})

	def material(mat_name, tex_index, color):
		return {
			"name": mat_name,
			"pbrMetallicRoughness": {
				"baseColorTexture": {"index": tex_index},
				"baseColorFactor": color,
				"metallicFactor": 0.0,
				"roughnessFactor": 1.0,
			},
		}

	gltf = {
		"asset": {"version": "2.0", "generator": "Tools/MakeStageModel.py"},
		"scene": 0,
		"scenes": [{"nodes": [0]}],
		"nodes": [{"name": name, "mesh": 0}],
		"meshes": [{"name": name, "primitives": primitives}],
		"materials": [
			material("Pavement", 0, [1.0, 1.0, 1.0, 1.0]),
			material("StoneWall", 1, [1.0, 1.0, 1.0, 1.0]),
		],
		"textures": [{"source": 0, "sampler": 0}, {"source": 1, "sampler": 0}],
		"images": [{"uri": "Pavement.png"}, {"uri": "StoneWall.png"}],
		"samplers": [{"magFilter": 9729, "minFilter": 9987, "wrapS": 10497, "wrapT": 10497}],
		"buffers": [{"uri": name + ".bin", "byteLength": len(bin_data)}],
		"bufferViews": buffer_views,
		"accessors": accessors,
	}

	with open(os.path.join(out_dir, name + ".bin"), "wb") as f:
		f.write(bin_data)
	with open(os.path.join(out_dir, name + ".gltf"), "w", encoding="utf-8") as f:
		json.dump(gltf, f, indent=1)


def main():
	rng = random.Random(SEED)
	np_rng = np.random.default_rng(SEED)

	root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
	out_dir = os.path.join(root, "Asset", "Models", "GameObject", "Stage", "RuinedRoad")
	os.makedirs(out_dir, exist_ok=True)

	# 形
	road = MeshBuilder()
	build_road(road, rng)

	stone = MeshBuilder()
	build_pillars(stone, rng)
	build_walls(stone, rng)
	build_rubble(stone, rng)
	build_gate(stone, rng)
	build_cathedral(road, stone, rng)

	write_gltf(out_dir, "RuinedRoad", [(road, 0), (stone, 1)])

	# テクスチャ（青みがかった灰色の石。夜の廃墟に合う暗めの色）
	pave = make_stone_texture(np_rng, rows=6, cols_per_row=4, base_color=(0.42, 0.42, 0.45),
		grout_color=(0.08, 0.08, 0.09), offset_rows=True, jitter=0.25)
	wall = make_stone_texture(np_rng, rows=8, cols_per_row=3, base_color=(0.5, 0.48, 0.46),
		grout_color=(0.12, 0.11, 0.1), offset_rows=True, jitter=0.12)
	write_png(os.path.join(out_dir, "Pavement.png"), pave)
	write_png(os.path.join(out_dir, "StoneWall.png"), wall)

	tri = (len(road.idx) + len(stone.idx)) // 3
	print("作成:", os.path.normpath(out_dir), "（三角形", tri, "枚）")


if __name__ == "__main__":
	main()
