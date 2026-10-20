# Effekseer のエフェクトの元データ（.efkproj）を作るスクリプト
# 使い方：python Tools/MakeEffekseerProjects.py
#   → Asset/Data/Effect/ に .efkproj ができる
#   → Effekseer エディタで開いて「名前を付けて保存」で同じ名前の .efkefc にする（ゲームが読むのは .efkefc）
#
# .efkproj は XML。書いていない項目はエディタの初期値になる
# 色は白～明るめで作ってあり、ゲーム側の SetColor でモードの色をかけ算できる

import os

# ===== XML を組み立てる小さな道具 =====

def fmt(v):
	"""値を文字にする（整数の項目もあるので、整数になる数は小数点を付けない）"""
	if isinstance(v, bool):
		return "True" if v else "False"
	if isinstance(v, float):
		v = round(v, 4)
		if v.is_integer():
			return str(int(v))
	return str(v)


def el(name, *children):
	"""<name>children</name> を作る。子が1つで、XML でない（数値・文字）ならそのまま値にする"""
	if len(children) == 1 and not (isinstance(children[0], str) and children[0].startswith("<")):
		return f"<{name}>{fmt(children[0])}</{name}>"
	return f"<{name}>" + "".join(children) + f"</{name}>"


def rng(center, spread=0.0):
	"""ばらつきのある値（Center / Max / Min）"""
	return el("Center", center) + el("Max", center + spread) + el("Min", center - spread)


def xyz_rng(x=(0, 0), y=(0, 0), z=(0, 0)):
	"""X / Y / Z それぞれ (中心, ばらつき)"""
	return el("X", rng(*x)) + el("Y", rng(*y)) + el("Z", rng(*z))


def rgba(r=255, g=255, b=255, a=255):
	return el("R", r) + el("G", g) + el("B", b) + el("A", a)


def rgba_rng(r=255, g=255, b=255, a=255):
	return el("R", rng(r)) + el("G", rng(g)) + el("B", rng(b)) + el("A", rng(a))


def common(life, count=1, gen_time=1.0, offset=0.0, life_spread=0.0):
	"""寿命・出す数・出す間隔・出し始めるまでの時間（フレーム）"""
	return el("CommonValues",
		el("MaxGeneration", el("Value", count)),
		el("Life", rng(life, life_spread)),
		el("GenerationTime", rng(gen_time)),
		el("GenerationTimeOffset", rng(offset)),
	)


def location_fixed(x=0.0, y=0.0, z=0.0):
	return el("LocationValues", el("Type", 0), el("Fixed", el("Location", el("X", x), el("Y", y), el("Z", z))))


def location_pva(pos=((0, 0), (0, 0), (0, 0)), vel=((0, 0), (0, 0), (0, 0)), acc=((0, 0), (0, 0), (0, 0))):
	"""位置・速度・加速度（1フレームあたり）"""
	return el("LocationValues", el("Type", 1), el("PVA",
		el("Location", xyz_rng(*pos)),
		el("Velocity", xyz_rng(*vel)),
		el("Acceleration", xyz_rng(*acc)),
	))


def rotation_fixed(x=0.0, y=0.0, z=0.0):
	return el("RotationValues", el("Type", 0), el("Fixed", el("Rotation", el("X", x), el("Y", y), el("Z", z))))


def rotation_random_z():
	"""板の向きをばらばらにする（Z回転をランダム）"""
	return el("RotationValues", el("Type", 1), el("PVA", el("Rotation", xyz_rng(z=(0, 180)))))


def scale_single_easing(start, end, start_spread=0.0, start_speed=0, end_speed=0):
	"""全体の大きさを start → end に変える"""
	return el("ScalingValues", el("Type", 4), el("SingleEasing",
		el("Start", rng(start, start_spread)),
		el("End", rng(end)),
		el("StartSpeed", start_speed),
		el("EndSpeed", end_speed),
	))


def scale_xy_easing(start, end, start_speed=0, end_speed=0):
	"""X・Y の大きさを別々に start → end に変える（(x, y) で指定。細長い光などに使う）"""
	return el("ScalingValues", el("Type", 2), el("Easing",
		el("Start", el("X", rng(start[0])), el("Y", rng(start[1])), el("Z", rng(1.0))),
		el("End", el("X", rng(end[0])), el("Y", rng(end[1])), el("Z", rng(1.0))),
		el("StartSpeed", start_speed),
		el("EndSpeed", end_speed),
	))


def renderer(texture=None, fade_in=0, fade_out=0):
	"""加算合成。fade はフレーム数"""
	items = []
	if texture:
		items.append(el("ColorTexture", texture))
	items.append(el("AlphaBlend", 2))	# 2 = 加算
	if fade_in > 0:
		items += [el("FadeInType", 1), el("FadeIn", el("Frame", fade_in))]
	if fade_out > 0:
		items += [el("FadeOutType", 1), el("FadeOut", el("Frame", fade_out))]
	return el("RendererCommonValues", *items)


def sprite(color=(255, 255, 255, 255), color_end=None, billboard=None):
	"""板（スプライト）。color_end を指定すると色がだんだん変わる"""
	items = []
	if color_end is None:
		items.append(el("ColorAll", el("Type", 0), el("Fixed", rgba(*color))))
	else:
		items.append(el("ColorAll", el("Type", 2), el("Easing",
			el("Start", rgba_rng(*color)), el("End", rgba_rng(*color_end)))))
	sprite_items = []
	if billboard is not None:
		sprite_items.append(el("Billboard", billboard))
	if sprite_items:
		items.append(el("Sprite", *sprite_items))
	return el("DrawingValues", el("Type", 2), *items)


def ring(outer, inner, color=(255, 255, 255, 255), vertex=48):
	"""輪っか。outer / inner は (始めの半径, 終わりの半径)"""
	return el("DrawingValues", el("Type", 4), el("Ring",
		el("VertexCount", vertex),
		el("Outer", 2), el("Outer_Easing",
			el("Start", el("X", rng(outer[0]))), el("End", el("X", rng(outer[1]))),
			el("StartSpeed", 20), el("EndSpeed", -30)),
		el("Inner", 2), el("Inner_Easing",
			el("Start", el("X", rng(inner[0]))), el("End", el("X", rng(inner[1]))),
			el("StartSpeed", 20), el("EndSpeed", -30)),
		el("CenterRatio_Fixed", 0.5),
		el("OuterColor_Fixed", rgba(*color[:3], 0)),
		el("CenterColor_Fixed", rgba(*color)),
		el("InnerColor_Fixed", rgba(*color[:3], 0)),
	))


def node(name, *parts, children=()):
	return el("Node", *parts, el("Name", name), el("Children", *children) if children else el("Children", ""))


def hidden():
	"""描かないノード（子の位置・大きさの基準にだけ使う）"""
	return el("DrawingValues", el("Type", 0)) + el("IsRendered", False)


def scale_fixed(x=1.0, y=1.0, z=1.0):
	return el("ScalingValues", el("Type", 0), el("Fixed", el("Scale", el("X", x), el("Y", y), el("Z", z))))


def rotation_tumble(speed):
	"""ばらばらの向きから、ばらばらの速さで回る（破片用。speed は1フレームに回る角度）"""
	return el("RotationValues", el("Type", 1), el("PVA",
		el("Rotation", xyz_rng(x=(0, 180), y=(0, 180), z=(0, 180))),
		el("Velocity", xyz_rng(x=(0, speed), y=(0, speed), z=(0, speed))),
	))


def project(nodes, end_frame):
	root = el("Root", el("Name", "Root"), el("Children", *nodes))
	body = el("EffekseerProject",
		root,
		el("ToolVersion", "1.7.3.0"),
		el("Version", 3),
		el("StartFrame", 0),
		el("EndFrame", end_frame),
		el("IsLoop", False),
	)
	return '<?xml version="1.0" encoding="utf-8"?>\n' + body


SOFT = "Tex/Particle_Soft.png"	# ふちがぼやけた丸
SPARK = "Tex/Particle02.png"		# 光の粒
LINE = "Tex/Line01.png"			# 細長い光
SHIELD = "Tex/ShieldHex.png"		# 丸い光の盾（Tools/MakeShieldTexture.py）
SHARD = "Tex/ShieldShard.png"		# 盾のかけら（Tools/MakeShieldTexture.py）
FIXED = 2						# 板の向き：固定（ノードの向きのまま。盾をプレイヤーの前に立てる）
SHIELD_W, SHIELD_H = 1.6, 1.6	# 盾の幅・高さ（丸い盾）

# ===== モード切り替え（E キー）：足元から光の輪が広がり、光の粒と柱が立ちのぼる =====
# 色は白っぽく作り、ゲーム側でモードの色（青・オレンジ）をかけ算する

def mode_change():
	return project([
		# 足元に広がる光の輪
		node("GroundRing",
			common(life=28),
			location_fixed(y=0.05),
			rotation_fixed(x=90),
			renderer(fade_out=14),
			ring(outer=(0.4, 2.6), inner=(0.2, 2.2), color=(255, 255, 255, 255)),
		),
		# 体を包む光
		node("BodyFlash",
			common(life=16),
			location_fixed(y=1.0),
			scale_single_easing(1.0, 3.2, start_speed=20, end_speed=-30),
			renderer(texture=SOFT, fade_out=10),
			sprite(color=(255, 255, 255, 200), color_end=(255, 255, 255, 0)),
		),
		# 立ちのぼる光の柱（細長い光を数本）
		node("Pillar",
			common(life=24, count=6, gen_time=1.0),
			location_pva(pos=((0, 0.5), (1.0, 0.2), (0, 0.5))),
			rotation_random_z(),
			scale_xy_easing((0.25, 3.0), (0.05, 4.5), start_speed=20, end_speed=-30),
			renderer(texture=LINE, fade_in=4, fade_out=12),
			sprite(color=(255, 255, 255, 220), billboard=1),	# 1 = Y軸だけ固定して縦に立てる
		),
		# 舞い上がる光の粒
		node("Sparks",
			common(life=36, count=30, gen_time=0.3, life_spread=8),
			location_pva(
				pos=((0, 0.6), (0.3, 0.3), (0, 0.6)),
				vel=((0, 0.05), (0.06, 0.03), (0, 0.05)),
				acc=((0, 0), (0.001, 0), (0, 0)),
			),
			scale_single_easing(0.18, 0.0, start_spread=0.06),
			renderer(texture=SPARK, fade_out=10),
			sprite(color=(255, 255, 255, 255)),
		),
	], end_frame=60)


# ===== アビリティ（ATBコマンドの技）の発動：モード切り替えより大きく、金色に強く光る =====

def ability_start():
	gold = (255, 225, 140, 255)
	return project([
		node("GroundRing",
			common(life=30),
			location_fixed(y=0.05),
			rotation_fixed(x=90),
			renderer(fade_out=14),
			ring(outer=(0.6, 4.0), inner=(0.3, 3.4), color=gold),
		),
		node("GroundRing2",
			common(life=30, offset=6),
			location_fixed(y=0.05),
			rotation_fixed(x=90),
			renderer(fade_out=14),
			ring(outer=(0.4, 3.0), inner=(0.2, 2.6), color=(255, 255, 255, 200)),
		),
		node("BodyFlash",
			common(life=18),
			location_fixed(y=1.0),
			scale_single_easing(1.5, 4.5, start_speed=20, end_speed=-30),
			renderer(texture=SOFT, fade_out=12),
			sprite(color=(255, 240, 200, 230), color_end=(255, 200, 120, 0)),
		),
		node("Pillar",
			common(life=26, count=10, gen_time=0.5),
			location_pva(pos=((0, 0.7), (1.2, 0.2), (0, 0.7))),
			scale_xy_easing((0.3, 3.5), (0.05, 6.0), start_speed=20, end_speed=-30),
			renderer(texture=LINE, fade_in=3, fade_out=12),
			sprite(color=gold, billboard=1),
		),
		node("Sparks",
			common(life=40, count=50, gen_time=0.2, life_spread=10),
			location_pva(
				pos=((0, 0.8), (0.4, 0.4), (0, 0.8)),
				vel=((0, 0.08), (0.08, 0.04), (0, 0.08)),
				acc=((0, 0), (0.0015, 0), (0, 0)),
			),
			scale_single_easing(0.22, 0.0, start_spread=0.08),
			renderer(texture=SPARK, fade_out=10),
			sprite(color=gold),
		),
	], end_frame=70)


# ===== ガード成功：体の前に丸い光の盾が出て、攻撃を受け止める =====
# ゲーム側で、プレイヤーの前に、プレイヤーの向きに合わせて出す（盾は XY 平面に立つ）

def guard():
	return project([
		node("ShieldRoot",
			common(life=24),
			scale_fixed(SHIELD_W, SHIELD_H),
			hidden(),
			children=[
				# 盾（ぱっと出て少し広がり、消える）
				node("Shield",
					common(life=24),
					scale_single_easing(0.8, 1.0, start_speed=30, end_speed=-30),
					renderer(texture=SHIELD, fade_in=2, fade_out=12),
					sprite(color=(170, 225, 255, 210), billboard=FIXED),
				),
				# 受け止めた所の波紋
				# （輪っかは盾と同じ向き＝XY平面に立つ）
				node("Ripple",
					common(life=12),
					location_fixed(z=0.01),
					renderer(fade_out=8),
					ring(outer=(0.05, 0.45), inner=(0.0, 0.38), color=(200, 235, 255, 255)),
				),
			],
		),
		# 受け止めた瞬間の光
		node("Flash",
			common(life=8),
			scale_single_easing(0.3, 0.9, start_speed=30, end_speed=-30),
			renderer(texture=SOFT, fade_out=6),
			sprite(color=(220, 240, 255, 255), color_end=(150, 200, 255, 0)),
		),
		# 小さな火花
		node("Sparks",
			common(life=14, count=10, gen_time=0.1, life_spread=4),
			location_pva(
				vel=((0, 0.08), (0.03, 0.05), (0, 0.08)),
				acc=((0, 0), (-0.006, 0), (0, 0)),
			),
			scale_single_easing(0.08, 0.0, start_spread=0.03),
			renderer(texture=SPARK, fade_out=5),
			sprite(color=(255, 245, 220, 255)),
		),
	], end_frame=30)


# ===== ガードブレイク：盾が赤くなって砕け、かけらが飛び散る =====

BREAK_FRAME = 8		# 盾が砕けるまでのフレーム

def guard_break():
	red = (255, 90, 60, 255)
	return project([
		node("ShieldRoot",
			common(life=BREAK_FRAME + 2),
			scale_fixed(SHIELD_W, SHIELD_H),
			hidden(),
			children=[
				# 盾（青から赤に変わりながら、ふくらんで割れる）
				node("Shield",
					common(life=BREAK_FRAME + 2),
					scale_single_easing(1.0, 1.1),
					renderer(texture=SHIELD, fade_out=3),
					sprite(color=(170, 225, 255, 230), color_end=(255, 80, 60, 255), billboard=FIXED),
				),
			],
		),
		# 割れた瞬間の光
		# （盾と同じ向きに、赤い輪が広がる）
		node("BreakRing",
			common(life=16, offset=BREAK_FRAME),
			renderer(fade_out=10),
			ring(outer=(0.6, 2.2), inner=(0.4, 1.9), color=red),
		),
		node("Glow",
			common(life=16, offset=BREAK_FRAME),
			scale_single_easing(1.0, 3.0, start_speed=30, end_speed=-30),
			renderer(texture=SOFT, fade_out=12),
			sprite(color=(255, 150, 120, 220), color_end=(255, 60, 40, 0)),
		),
		# 盾のかけら（盾のあった所から、回りながら飛び散って落ちる）
		node("Shards",
			common(life=34, count=28, gen_time=0.05, offset=BREAK_FRAME, life_spread=8),
			location_pva(
				pos=((0, SHIELD_W * 0.4), (0, SHIELD_H * 0.4), (0, 0.05)),
				vel=((0, 0.09), (0.03, 0.07), (0, 0.07)),
				acc=((0, 0), (-0.006, 0), (0, 0)),
			),
			rotation_tumble(12),
			scale_single_easing(0.24, 0.12, start_spread=0.08),
			renderer(texture=SHARD, fade_out=14),
			sprite(color=(190, 230, 255, 255), color_end=(255, 120, 100, 160), billboard=FIXED),
		),
		# 赤い火花
		node("Sparks",
			common(life=22, count=24, gen_time=0.1, offset=BREAK_FRAME, life_spread=6),
			location_pva(
				vel=((0, 0.16), (0.06, 0.1), (0, 0.16)),
				acc=((0, 0), (-0.009, 0), (0, 0)),
			),
			scale_single_easing(0.12, 0.0, start_spread=0.05),
			renderer(texture=SPARK, fade_out=8),
			sprite(color=(255, 170, 120, 255)),
		),
	], end_frame=50)


# ===== プレイヤーの攻撃が敵に当たった：剣を振った向きに1本だけ走る斬撃の光 =====
# ゲーム側で、カメラの方を向けて、X軸が剣先の動いた向きになるように回して出す（板は XY 平面）
# 白く作ってあり、ゲーム側でモードの色をかけ算する

def hit_slash():
	def streak(name, width, length, color, life):
		# 縦長の光（Line01）を Z で 90度回して、X 方向（剣を振った向き）に長くする
		return node(name + "Root",
			common(life=life),
			rotation_fixed(z=90),
			scale_fixed(width, length),
			hidden(),
			children=[
				node(name,
					common(life=life),
					scale_single_easing(0.5, 1.0, start_speed=30, end_speed=-30),
					renderer(texture=LINE, fade_out=life - 2),
					sprite(color=color, color_end=(color[0], color[1], color[2], 0), billboard=FIXED),
				),
			],
		)

	return project([
		streak("Slash", 0.35, 2.2, (255, 255, 255, 230), 10),
		streak("SlashCore", 0.12, 2.6, (255, 255, 255, 255), 8),
		# 当たった所の光
		node("Flash",
			common(life=6),
			scale_single_easing(0.25, 0.7, start_speed=30, end_speed=-30),
			renderer(texture=SOFT, fade_out=5),
			sprite(color=(255, 255, 255, 230), color_end=(255, 255, 255, 0)),
		),
		# 振った向き（+X）に飛ぶ火花
		node("Sparks",
			common(life=12, count=6, gen_time=0.1, life_spread=3),
			location_pva(
				vel=((0.07, 0.04), (0, 0.04), (0, 0.03)),
				acc=((0, 0), (-0.004, 0), (0, 0)),
			),
			scale_single_easing(0.07, 0.0, start_spread=0.02),
			renderer(texture=SPARK, fade_out=4),
			sprite(color=(255, 255, 255, 255)),
		),
	], end_frame=15)


# ===== 敵の攻撃がプレイヤーに当たった：小さな赤い3本の爪あとと光 =====
# ゲーム側でカメラの方を向けて出す

def hit_player():
	red = (255, 70, 50, 255)
	claws = []
	for i, offset in enumerate((-0.12, 0.0, 0.12)):
		claws.append(node(f"Claw{i}Root",
			common(life=10),
			location_fixed(x=offset, y=-offset * 0.3),
			rotation_fixed(z=35),
			scale_fixed(0.18, 1.0),
			hidden(),
			children=[
				node(f"Claw{i}",
					common(life=10, offset=i),
					scale_single_easing(0.4, 1.0, start_speed=30, end_speed=-30),
					renderer(texture=LINE, fade_out=7),
					sprite(color=red, color_end=(255, 40, 30, 0), billboard=FIXED),
				),
			],
		))
	return project(claws + [
		node("Flash",
			common(life=7),
			scale_single_easing(0.2, 0.6, start_speed=30, end_speed=-30),
			renderer(texture=SOFT, fade_out=6),
			sprite(color=(255, 140, 110, 220), color_end=(255, 60, 40, 0)),
		),
		node("Sparks",
			common(life=12, count=6, gen_time=0.1, life_spread=3),
			location_pva(
				vel=((0, 0.05), (0.02, 0.04), (0, 0.05)),
				acc=((0, 0), (-0.004, 0), (0, 0)),
			),
			scale_single_easing(0.06, 0.0, start_spread=0.02),
			renderer(texture=SPARK, fade_out=4),
			sprite(color=(255, 150, 110, 255)),
		),
	], end_frame=15)


# ===== 敵を倒した：小さくまとまった黒紫の光と、立ちのぼる粒（約1秒で消える）=====
# 大きさ 1 で、普通の敵の体に合う大きさ。ゲーム側でカメラの方を向けて、体の大きさに合わせて出す

def enemy_death():
	purple = (170, 90, 255, 255)
	return project([
		node("Flash",
			common(life=14),
			scale_single_easing(0.6, 1.8, start_speed=30, end_speed=-30),
			renderer(texture=SOFT, fade_out=10),
			sprite(color=(220, 180, 255, 220), color_end=(120, 60, 200, 0)),
		),
		node("Ring",
			common(life=16),
			renderer(fade_out=10),
			ring(outer=(0.2, 1.3), inner=(0.1, 1.15), color=purple),
		),
		# 立ちのぼって消える粒（体がほどけていく感じ）
		node("Dust",
			common(life=40, count=18, gen_time=0.5, life_spread=10),
			location_pva(
				pos=((0, 0.4), (0, 0.5), (0, 0.4)),
				vel=((0, 0.01), (0.025, 0.01), (0, 0.01)),
			),
			scale_single_easing(0.16, 0.0, start_spread=0.05),
			renderer(texture=SOFT, fade_in=4, fade_out=14),
			sprite(color=(150, 80, 230, 230)),
		),
		node("Sparks",
			common(life=16, count=10, gen_time=0.1, life_spread=4),
			location_pva(
				vel=((0, 0.06), (0.02, 0.05), (0, 0.06)),
				acc=((0, 0), (-0.003, 0), (0, 0)),
			),
			scale_single_easing(0.08, 0.0, start_spread=0.03),
			renderer(texture=SPARK, fade_out=6),
			sprite(color=(230, 200, 255, 255)),
		),
	], end_frame=50)


EFFECTS = {
	"ModeChange.efkproj": mode_change,
	"AbilityStart.efkproj": ability_start,
	"Guard.efkproj": guard,
	"GuardBreak.efkproj": guard_break,
	"HitSlash.efkproj": hit_slash,
	"HitPlayer.efkproj": hit_player,
	"EnemyDeath.efkproj": enemy_death,
}


def main():
	root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
	out_dir = os.path.join(root, "Asset", "Data", "Effect")
	os.makedirs(out_dir, exist_ok=True)
	for name, make in EFFECTS.items():
		path = os.path.join(out_dir, name)
		with open(path, "w", encoding="utf-8") as f:
			f.write(make())
		print("作成:", os.path.normpath(path))


if __name__ == "__main__":
	main()
