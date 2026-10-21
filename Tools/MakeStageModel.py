# ステージのモデル（廃墟の参道）を作るスクリプト
# 使い方：python Tools/MakeStageModel.py
# Asset/Models/GameObject/Stage/RuinedRoad/ に glTF（.gltf / .bin / テクスチャ）を作る
#
# 中身（スタート地点 z=0 から、大聖堂の廃墟 z=60 へ向かう一本道）
#   ・石畳の道（幅 kRoadWidth）
#   ・道の両側に並ぶ、折れた柱
#   ・その外側の、崩れた低い石壁
#   ・柱のまわりの見えない当たり判定（RuinedRoadCollision.gltf。柱に近づきすぎないように）
#   ・大聖堂の手前の、半分崩れたアーチの門
#   ・スタート地点の後ろの壁の、入口の扉（開いているが、中は真っ黒で通れない）
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
ROAD_TOP_Y = 0.04			# 石畳の上の面の高さ
PILLAR_X = 11.0				# 柱が並ぶ位置（道の中心からの距離）
PILLAR_STEP = 7.0			# 柱の間隔
WALL_X = 16.0				# 両わきの石壁の位置（石畳はこの壁まで敷きつめる＝道から落ちない）
WALL_MIN_HEIGHT = 3.6		# 石壁の一番低い所（ジャンプの高さ 約3.1m より高くして、乗り越えられないようにする）
ROAD_WIDTH = WALL_X * 2		# 石畳の幅
GATE_Z = 46.0				# 門の位置
GATE_HALF_WIDTH = 8.0		# 門の幅の半分

# ===== 大聖堂（ボスの広間） =====
NAVE_START_Z = 52.0			# 入口の壁の位置
NAVE_END_Z = 100.0			# 奥の壁の位置（この先に半円の壁）
NAVE_HALF_WIDTH = 22.0		# 広間の幅の半分（大きなボスと戦えるように広くする）
NAVE_WALL_THICK = 1.6		# 壁の厚さ
NAVE_BAY = 6.0				# 柱と柱の間隔（窓1つ分）
NAVE_PILLAR_X = 16.0		# 中の柱の位置（真ん中で待つボスにかからないように）
NAVE_WINDOW_BOTTOM = 4.0	# 窓の穴の下の高さ（ジャンプで窓に乗れないように高くする）
ENTRANCE_HALF_WIDTH = 9.0	# 入口の幅の半分

# スタート地点の後ろの、入口の扉（ここから来た、という入口。中は真っ黒で通れない）
DOOR_HALF_WIDTH = 2.4		# 扉の穴の幅の半分
DOOR_SPRING_Y = 4.2			# アーチが曲がり始める高さ
WALL_THICK = 1.4			# 両わき・後ろの石壁の厚さ

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

	def add_quad_uv(self, pts, uvs, shade, outward):
		"""UV を直接決めた四角形を追加する（扉のように、テクスチャを1枚ぴったり貼る形用）
		   outward：表の向き（頂点の並びを、この向きが表になるように直す）"""
		p = [np.array(v, dtype=np.float64) for v in pts]
		n = np.cross(p[1] - p[0], p[3] - p[0])
		length = np.linalg.norm(n)
		if length < 1e-9:
			return
		n /= length
		order = [0, 1, 2, 3]
		if np.dot(n, outward) < 0:
			order = [0, 3, 2, 1]
			n = -n
		base = len(self.pos)
		for i in order:
			self.pos.append(p[i])
			self.nor.append(n)
			self.uv.append(uvs[i])
			self.col.append(shade)
		self.idx += [base, base + 1, base + 2, base, base + 2, base + 3]

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
			sink = rng.uniform(-0.02, 0.0)
			shade = rng.uniform(0.82, 1.0)
			top = ROAD_TOP_Y + sink
			mesh.add_box((x + step / 2, top - 0.1, z + step / 2), (step, 0.2, step), PAVE_TILE_METER,
				shade=shade, bottom_shade=shade * 0.6)
			x += step
		z += step


# 柱のまわりの見えない当たり判定（柱から、これだけ外側まで近づけない）
PILLAR_GUARD_MARGIN = 0.6
PILLAR_GUARD_HEIGHT = 4.0

# 見えない当たり判定を置く柱（中心 x, z と、台座の大きさ）。柱を作る時に集める
pillar_guards = []

# 瓦礫は置かない（ここに入れた形は書き出さない。乱数の順番を変えないように、作るだけ作って捨てる）
discard = None


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
			pillar_guards.append((x, pz, 1.7))

			kind = rng.random()
			if kind < 0.2:
				# 根元だけ残っている
				h = rng.uniform(0.6, 1.4)
				mesh.add_column((x, pz), 0.6, h, 0.6, STONE_TILE_METER, broken=0.35, rng=rng)
				# 倒れた柱の残り（瓦礫なので置かない）
				discard.add_box((x + side * 1.8, 0.45, pz + rng.uniform(-1, 1)), (1.0, 0.9, 3.2), STONE_TILE_METER,
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
	"""道の両わきと後ろの石壁（すき間なく囲んで、道から落ちないようにする。上は崩れてギザギザ）"""
	seg = 1.6
	thick = WALL_THICK
	for side in (-1, 1):
		z = ROAD_START_Z - thick
		while z < NAVE_START_Z - 0.8:
			z1 = min(z + seg, NAVE_START_Z - 0.8)
			h = rng.uniform(WALL_MIN_HEIGHT + 0.4, WALL_MIN_HEIGHT + 1.8)
			offsets = [rng.uniform(-0.4, 0.1) for _ in range(4)]
			mesh.add_box((side * (WALL_X + thick / 2), h / 2, (z + z1) / 2), (thick, h, (z1 - z) + 0.05),
				STONE_TILE_METER, shade=rng.uniform(0.7, 0.9), bottom_shade=0.4, top_offsets=offsets)
			z = z1
	# 後ろの壁（スタート地点の後ろ）
	x = -WALL_X
	while x < WALL_X - 1e-6:
		h = rng.uniform(WALL_MIN_HEIGHT + 0.4, WALL_MIN_HEIGHT + 1.8)
		offsets = [rng.uniform(-0.4, 0.1) for _ in range(4)]
		shade = rng.uniform(0.7, 0.9)
		# 真ん中は入口の扉にする（乱数は使ってから飛ばすので、ほかの形は変わらない）
		if abs(x + seg / 2) > DOOR_HALF_WIDTH + 1.0:
			mesh.add_box((x + seg / 2, h / 2, ROAD_START_Z - thick / 2), (seg + 0.05, h, thick), STONE_TILE_METER,
				shade=shade, bottom_shade=0.4, top_offsets=offsets)
		x += seg


def pointed_arch(mesh, z, depth, thick, span_half, spring_y, shade):
	"""とがったアーチ（ゴシック）：左右の柱の上から、反対側の柱を中心にした円弧を描き、真ん中でとがらせる"""
	radius = span_half * 2
	stones = 6
	top = math.radians(60)		# 2つの円弧が真ん中で出会う角度
	for sign in (-1, 1):
		for i in range(stones):
			a0 = top * i / stones
			a1 = top * (i + 1) / stones
			am = (a0 + a1) / 2
			x = sign * (-span_half + math.cos(am) * radius)
			y = spring_y + math.sin(am) * radius
			length = radius * (a1 - a0) * 1.08
			mesh.add_box((x, y, z), (length, thick, depth), STONE_TILE_METER, shade=shade, bottom_shade=shade * 0.75)
			# 石の傾き：円弧の接線に合わせる
			rotate_last_box_z(mesh, (x, y, z), (math.pi / 2 - am) if sign < 0 else (math.pi / 2 + am))


def pointed_arch_height(x, span_half, spring_y):
	"""とがったアーチの内側の高さ（横の位置 x で）"""
	radius = span_half * 2
	dx = span_half + abs(x)
	return spring_y + math.sqrt(max(radius * radius - dx * dx, 0.0))


def build_entrance_door(mesh, door, rng):
	"""スタート地点の後ろの壁の、入口の扉

	  ・とがったアーチの石の枠と、両わきの太い柱（まわりの壁より高くして目立たせる）
	  ・扉は2枚とも内側（道の方）へ開いている
	  ・扉の穴の中は真っ黒な板でふさぐ（この先は何も無い。壁と同じ当たり判定で通れない）
	"""
	dw = DOOR_HALF_WIDTH
	t = WALL_THICK
	z_wall = ROAD_START_Z - t / 2		# 後ろの壁の厚さの真ん中
	z_front = ROAD_START_Z				# 壁の、道の側の面
	top_y = DOOR_SPRING_Y + dw * 2 * math.sin(math.radians(60)) + 1.6	# 扉のまわりの壁の高さ（アーチの上まで）

	# 扉のまわりの壁（後ろの壁の切れ目をふさぐ。アーチの上まで高く）
	for sign in (-1, 1):
		x0, x1 = sorted((sign * dw, sign * (dw + 1.9)))
		mesh.add_box(((x0 + x1) / 2, top_y / 2, z_wall), (x1 - x0, top_y, t), STONE_TILE_METER,
			shade=rng.uniform(0.8, 0.9), bottom_shade=0.4, top_offsets=[rng.uniform(-0.8, 0.0) for _ in range(4)])
		# 両わきの太い柱（上は飾り石）
		px = sign * (dw + 1.7)
		mesh.add_box((px, top_y / 2 + 0.6, z_wall + 0.3), (1.2, top_y + 1.2, t + 0.8), STONE_TILE_METER,
			shade=0.92, bottom_shade=0.45)
		mesh.add_box((px, top_y + 1.5, z_wall + 0.3), (1.5, 0.5, t + 1.1), STONE_TILE_METER, shade=0.95, bottom_shade=0.7)
		pillar_guards.append((px, z_wall + 0.3, t + 0.8))
		# 扉の枠の柱（穴の両わき。手前に少し出っ張る）
		mesh.add_box((sign * (dw + 0.3), DOOR_SPRING_Y / 2, z_front + 0.15), (0.6, DOOR_SPRING_Y, 0.6), STONE_TILE_METER,
			shade=0.95, bottom_shade=0.5)

	# アーチの上の壁（アーチの形に合わせて、細い縦の石を並べてふさぐ）
	slices = 8
	for i in range(slices):
		xa = -dw + 2 * dw * i / slices
		xb = xa + 2 * dw / slices
		y0 = pointed_arch_height((xa + xb) / 2, dw, DOOR_SPRING_Y) - 0.2
		y1 = top_y - rng.uniform(0.0, 0.4)
		mesh.add_box(((xa + xb) / 2, (y0 + y1) / 2, z_wall), (xb - xa + 0.02, y1 - y0, t), STONE_TILE_METER,
			shade=rng.uniform(0.8, 0.9), bottom_shade=0.7)

	# 石の枠のアーチ（壁の中と、手前の出っ張った枠）
	pointed_arch(mesh, z_wall, t, 0.6, dw, DOOR_SPRING_Y, 0.9)
	pointed_arch(mesh, z_front + 0.15, 0.6, 0.6, dw + 0.3, DOOR_SPRING_Y, 0.95)

	# 穴の中の真っ黒な板（色を 0 にして、光を当てても黒いままにする。アーチの形に合わせて縦に並べる）
	slices = 12
	for i in range(slices):
		xa = -dw + 2 * dw * i / slices
		xb = xa + 2 * dw / slices
		h = pointed_arch_height((xa + xb) / 2, dw, DOOR_SPRING_Y)
		mesh.add_box(((xa + xb) / 2, h / 2, z_wall - 0.2), (xb - xa + 0.02, h, 0.4), STONE_TILE_METER,
			shade=0.0, bottom_shade=0.0)

	# 開いた扉（2枚。蝶番の所から、内側（道の方）へ 70 度ほど開いている）
	# 扉は木の板と鉄の帯のテクスチャ（DoorWood.png）を貼った、別のメッシュ（door）に作る
	open_angle = math.radians(70)
	for sign in (-1, 1):
		add_door_leaf(door, sign, z_front - 0.1, open_angle)

	# 扉の前の低い段（乗り越えられる高さ）
	mesh.add_box((0.0, ROAD_TOP_Y + 0.08, z_front + 0.9), (dw * 2 + 2.4, 0.16, 1.6), PAVE_TILE_METER,
		shade=0.75, bottom_shade=0.45)


# 扉の板の厚さ・テクスチャ（扉1枚の幅と、一番高い所の高さ。テクスチャはこの大きさに1枚貼る）
DOOR_THICK = 0.2
DOOR_LEAF_WIDTH = DOOR_HALF_WIDTH - 0.08
DOOR_LEAF_HEIGHT = DOOR_SPRING_Y + DOOR_HALF_WIDTH * 2 * math.sin(math.radians(60))


def add_door_leaf(mesh, sign, hinge_z, open_angle):
	"""扉1枚：上はアーチに合わせてとがった形（真ん中ほど高い）。表と裏にテクスチャを1枚ずつ貼る

	  sign：-1 なら左、+1 なら右の扉（蝶番は外側、開く側は真ん中）
	  扉の中の位置 (u, y)：u は蝶番からの距離、y は高さ
	"""
	w = DOOR_LEAF_WIDTH
	t = DOOR_THICK
	hinge = np.array((sign * (DOOR_HALF_WIDTH - 0.06), 0.0, hinge_z))
	along = np.array((-sign * math.cos(open_angle), 0.0, math.sin(open_angle)))	# 蝶番から開く側への向き
	side = np.array((along[2], 0.0, -along[0]))								# 扉の厚みの向き

	def top(u):
		# 閉じた時の、扉のその位置の真上のアーチの高さ（少し下げて、アーチとの間にすき間を作る）
		return pointed_arch_height(DOOR_HALF_WIDTH - 0.06 - u, DOOR_HALF_WIDTH, DOOR_SPRING_Y) - 0.08

	def point(u, y, n):
		return tuple(hinge + along * u + side * n + np.array((0.0, y, 0.0)))

	def uv(u, y):
		return (u / w, 1.0 - y / DOOR_LEAF_HEIGHT)

	slices = 10
	us = [w * i / slices for i in range(slices + 1)]
	shade = (1.0, 1.0, 1.0, 1.0)
	for k in range(slices):
		u0, u1 = us[k], us[k + 1]
		for n in (-t / 2, t / 2):
			pts = [point(u0, 0.0, n), point(u1, 0.0, n), point(u1, top(u1), n), point(u0, top(u0), n)]
			uvs = [uv(u0, 0.0), uv(u1, 0.0), uv(u1, top(u1)), uv(u0, top(u0))]
			mesh.add_quad_uv(pts, uvs, shade, side * (1 if n > 0 else -1))
		# 上の面（とがった形のふち）
		pts = [point(u0, top(u0), -t / 2), point(u1, top(u1), -t / 2), point(u1, top(u1), t / 2), point(u0, top(u0), t / 2)]
		mesh.add_quad_uv(pts, [uv(u0, top(u0))] * 2 + [uv(u1, top(u1))] * 2, shade, np.array((0.0, 1.0, 0.0)))
	# 蝶番の側・開く側の、ふちの面
	for u, out in ((0.0, -along), (w, along)):
		pts = [point(u, 0.0, -t / 2), point(u, 0.0, t / 2), point(u, top(u), t / 2), point(u, top(u), -t / 2)]
		mesh.add_quad_uv(pts, [uv(u, 0.0)] * 2 + [uv(u, top(u))] * 2, shade, out)


def make_door_texture(rng):
	"""扉のテクスチャ：縦に並んだ古い木の板と、黒い鉄の帯・鋲・輪の取っ手

	  扉1枚（幅 DOOR_LEAF_WIDTH × 高さ DOOR_LEAF_HEIGHT）にちょうど1枚貼るので、
	  鋲などの丸い形は、メートルに直して丸くなるように描く
	"""
	size = TEX_SIZE * 2		# 扉は縦に長く引きのばして貼るので、細かく描く
	ys, xs = np.mgrid[0:size, 0:size] / size
	mx = xs * DOOR_LEAF_WIDTH							# 横の位置（メートル。0 が蝶番の側）
	my = (1.0 - ys) * DOOR_LEAF_HEIGHT					# 高さ（メートル）
	noise = fractal_noise(size, rng)
	fine = fractal_noise(size, rng, octaves=(64, 128))

	# 木の板（縦に5枚。板ごとに色を変え、木目は縦に長くのびた模様にする）
	planks = 5
	plank = np.minimum((xs * planks).astype(int), planks - 1)
	lu = xs * planks - plank
	tones = np.array([rng.uniform(0.8, 1.1) for _ in range(planks)])
	grain = 0.5 + 0.5 * np.sin((lu * 9.0 + noise * 3.0 + plank * 1.7) * math.pi * 2)
	wood = np.array((0.32, 0.22, 0.14))[None, None, :] * (tones[plank] * (0.75 + 0.25 * grain) * (0.85 + 0.3 * fine))[..., None]
	# 板と板のすき間（暗い線）と、板のふちの少しの影
	gap = np.clip(1.0 - np.minimum(lu, 1.0 - lu) / 0.03, 0.0, 1.0)
	color = wood * (1.0 - 0.85 * gap[..., None])
	# 下の方ほど汚れて暗い（雨や泥）
	color *= (0.7 + 0.3 * np.clip(my / 1.5, 0.0, 1.0))[..., None]

	iron = np.array((0.13, 0.13, 0.14))
	iron_hi = np.array((0.34, 0.33, 0.33))

	def paint(mask, base, highlight=None, highlight_mask=None):
		nonlocal color
		c = base[None, None, :] * (0.8 + 0.4 * fine)[..., None]
		if highlight is not None:
			c = c * (1 - highlight_mask[..., None]) + highlight[None, None, :] * highlight_mask[..., None]
		color = color * (1 - mask[..., None]) + c * mask[..., None]

	# 鉄の帯（横に4本。蝶番の側は、先がとがった長い蝶番の形）
	for band_y in (0.7, 2.0, 3.3, 4.6):
		half = 0.09
		dy = np.abs(my - band_y)
		# 開く側に向かって少し細くなる
		taper = half * (1.0 - 0.35 * np.clip(mx / DOOR_LEAF_WIDTH, 0.0, 1.0))
		band = (dy < taper).astype(float)
		# 蝶番の側の、太くなった所
		knuckle = ((mx < 0.35) & (dy < half * 1.6)).astype(float)
		mask = np.maximum(band, knuckle)
		edge_hi = ((my - band_y > taper * 0.55) & (mask > 0)).astype(float)	# 上のふちを明るく（光が当たる）
		paint(mask, iron, iron_hi, edge_hi * 0.6)
		# 鋲（帯の上に等間隔）
		for rx in np.arange(0.18, DOOR_LEAF_WIDTH - 0.05, 0.32):
			d = np.sqrt((mx - rx) ** 2 + (my - band_y) ** 2)
			rivet = (d < 0.045).astype(float)
			hi = ((d < 0.025) & (my > band_y)).astype(float)
			paint(rivet, iron * 1.3, iron_hi * 1.4, hi)

	# 開く側のふちの鋲（縦に並ぶ）
	for ry in np.arange(0.35, DOOR_LEAF_HEIGHT, 0.45):
		d = np.sqrt((mx - (DOOR_LEAF_WIDTH - 0.12)) ** 2 + (my - ry) ** 2)
		paint((d < 0.035).astype(float), iron * 1.3)

	# 輪の取っ手（開く側の、胸の高さ）
	hx, hy = DOOR_LEAF_WIDTH - 0.38, 1.45
	d_plate = np.sqrt((mx - hx) ** 2 + (my - hy) ** 2)
	paint((d_plate < 0.1).astype(float), iron * 1.2, iron_hi, ((d_plate < 0.05) & (my > hy)).astype(float))
	ring_c = hy - 0.22
	d_ring = np.sqrt((mx - hx) ** 2 + (my - ring_c) ** 2)
	ring = ((np.abs(d_ring - 0.2) < 0.03) & (my < hy - 0.02)).astype(float)
	paint(ring, iron * 1.4, iron_hi * 1.2, (ring * (my > ring_c)).astype(float) * 0.7)

	return np.clip(color, 0.0, 1.0)


def build_rubble(mesh, rng):
	"""転がっている瓦礫（道の真ん中には置かない。戦闘のじゃまにならないように）"""
	for _ in range(70):
		side = rng.choice((-1, 1))
		x = side * rng.uniform(PILLAR_X + 1.2, WALL_X - 0.6)
		z = rng.uniform(ROAD_START_Z, ROAD_END_Z)
		s = rng.uniform(0.25, 0.9)
		mesh.add_box((x, s * 0.35, z), (s * rng.uniform(0.8, 1.6), s * 0.7, s * rng.uniform(0.8, 1.4)), STONE_TILE_METER,
			rot_y=rng.uniform(0, math.pi), rot_x=rng.uniform(-0.3, 0.3), shade=rng.uniform(0.6, 0.85), bottom_shade=0.4)


def build_gate(mesh, rng):
	"""大聖堂の手前の、半分崩れたアーチの門"""
	gx = GATE_HALF_WIDTH
	# 左右の太い柱（右はアーチの途中まで残り、左は途中で崩れている）
	for side, h in ((-1, 5.5), (1, 9.0)):
		mesh.add_box((side * gx, 0.5, GATE_Z), (2.6, 1.0, 2.6), STONE_TILE_METER, shade=0.85, bottom_shade=0.4)
		pillar_guards.append((side * gx, GATE_Z, 2.6))
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
	while z < NAVE_END_Z + hw * 0.6 + 1.0 - 1e-6:	# 奥の半円の壁の内側まで床を敷く
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
				wall_piece(x0, x1, z + 0.8, z1 - 0.8, 0.0, rng.uniform(WALL_MIN_HEIGHT + 0.6, WALL_MIN_HEIGHT + 1.5), broken=0.5)
			else:
				# 下の壁 → 窓の穴 → 上の壁（上は崩れていることがある）
				wall_piece(x0, x1, z + 0.8, z1 - 0.8, 0.0, NAVE_WINDOW_BOTTOM)
				for zz in (z + 0.8, z1 - 1.6):	# 窓の両わきの細い壁
					wall_piece(x0, x1, zz, zz + 0.8, NAVE_WINDOW_BOTTOM, 8.0)
				if kind > 0.5:
					wall_piece(x0, x1, z + 0.8, z1 - 0.8, 8.0, rng.uniform(9.5, 11.0), broken=1.2)
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
			pillar_guards.append((x, z, 2.0))
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

	# 広間の瓦礫（置かない）
	for _ in range(40):
		side = rng.choice((-1, 1))
		x = side * rng.uniform(hw - 3.5, hw - 1.2)
		zz = rng.uniform(NAVE_START_Z + 2, NAVE_END_Z)
		sz = rng.uniform(0.3, 1.0)
		discard.add_box((x, sz * 0.35, zz), (sz * 1.3, sz * 0.7, sz), STONE_TILE_METER, rot_y=rng.uniform(0, math.pi),
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
			material("DoorWood", 2, [1.0, 1.0, 1.0, 1.0]),
		],
		"textures": [{"source": 0, "sampler": 0}, {"source": 1, "sampler": 0}, {"source": 2, "sampler": 0}],
		"images": [{"uri": "Pavement.png"}, {"uri": "StoneWall.png"}, {"uri": "DoorWood.png"}],
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

	global discard
	discard = MeshBuilder()

	stone = MeshBuilder()
	build_pillars(stone, rng)
	build_walls(stone, rng)
	build_rubble(discard, rng)	# 瓦礫は置かない
	build_gate(stone, rng)
	build_cathedral(road, stone, rng)
	door = MeshBuilder()
	build_entrance_door(stone, door, random.Random(SEED + 2))	# 扉は別の乱数を使う（ほかの形が変わらないように）

	write_gltf(out_dir, "RuinedRoad", [(road, 0), (stone, 1), (door, 2)])

	# 柱のまわりの見えない当たり判定（描画はせず、横から近づけないようにするだけ）
	guard = MeshBuilder()
	for x, z, size in pillar_guards:
		w = size + PILLAR_GUARD_MARGIN * 2
		guard.add_box((x, PILLAR_GUARD_HEIGHT / 2, z), (w, PILLAR_GUARD_HEIGHT, w), STONE_TILE_METER)
	write_gltf(out_dir, "RuinedRoadCollision", [(guard, 1)])

	# テクスチャ（青みがかった灰色の石。夜の廃墟に合う暗めの色）
	pave = make_stone_texture(np_rng, rows=6, cols_per_row=4, base_color=(0.42, 0.42, 0.45),
		grout_color=(0.08, 0.08, 0.09), offset_rows=True, jitter=0.25)
	wall = make_stone_texture(np_rng, rows=8, cols_per_row=3, base_color=(0.5, 0.48, 0.46),
		grout_color=(0.12, 0.11, 0.1), offset_rows=True, jitter=0.12)
	write_png(os.path.join(out_dir, "Pavement.png"), pave)
	write_png(os.path.join(out_dir, "StoneWall.png"), wall)
	write_png(os.path.join(out_dir, "DoorWood.png"), make_door_texture(np.random.default_rng(SEED + 2)))

	tri = (len(road.idx) + len(stone.idx)) // 3
	print("作成:", os.path.normpath(out_dir), "（三角形", tri, "枚）")


if __name__ == "__main__":
	main()
