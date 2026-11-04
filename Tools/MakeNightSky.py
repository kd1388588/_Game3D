# -*- coding: utf-8 -*-
"""
夜空・月・地面の霧のテクスチャを作る

  使い方：python Tools/MakeNightSky.py   （numpy と Pillow が必要）

  作るもの（Asset/Models/GameObject/Stage/Sky/）
    NightSky.png  … 暗く晴れた夜空（球に貼る。横：360度、縦：真上→真下）
                    上は黒に近い紺、地平線に向かって霧の色に近づく。星をまばらに散らす（雲は無し）
    Moon.png      … 少し赤い月（球に貼る。クレーターと海（暗い所））
    MoonGlow.png  … 月のまわりの、ごく薄い光のにじみ（加算で重ねる）
    NightSky.gltf・Moon.gltf … Sky.gltf（半径 10 の球）の形をそのまま使い、貼る画像だけを変えたモデル
  作るもの（Asset/Textures/Stage/）
    GroundMist.png … 地面の霧の1かたまり（白黒。白いほど濃い、もやもやした形。加算で重ねる）
"""
import math
import os

import numpy as np
from PIL import Image

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
SKY_DIR = os.path.join(ROOT, "Asset", "Models", "GameObject", "Stage", "Sky")
TEX_DIR = os.path.join(ROOT, "Asset", "Textures", "Stage")
SEED = 2024

# 地平線の色（ゲームの距離フォグの色 StageEnvironment::kDistanceFogColor に合わせ、壁の上端となじませる）
HORIZON_COLOR = np.array((0.10, 0.11, 0.16))
ZENITH_COLOR = np.array((0.008, 0.01, 0.025))	# 真上の色（黒に近い紺）
MOON_COLOR = np.array((0.95, 0.70, 0.58))		# 月の色（少し赤い）
MOON_GLOW_COLOR = np.array((0.55, 0.22, 0.16))	# 月のまわりのにじみの色


def value_noise(size_x, size_y, cells_x, cells_y, rng):
	"""なめらかなノイズ（横方向はつながるようにする＝球に貼ってもつなぎ目が出ない）"""
	grid = rng.random((cells_y + 1, cells_x))
	ys, xs = np.mgrid[0:size_y, 0:size_x]
	gx = xs / size_x * cells_x
	gy = ys / size_y * cells_y
	x0 = np.floor(gx).astype(int)
	y0 = np.floor(gy).astype(int)
	tx = gx - x0
	ty = gy - y0
	tx = tx * tx * (3 - 2 * tx)
	ty = ty * ty * (3 - 2 * ty)
	x1 = (x0 + 1) % cells_x
	y1 = np.minimum(y0 + 1, cells_y)
	a = grid[y0, x0 % cells_x] * (1 - tx) + grid[y0, x1] * tx
	b = grid[y1, x0 % cells_x] * (1 - tx) + grid[y1, x1] * tx
	return a * (1 - ty) + b * ty


def fractal(size_x, size_y, rng, octaves):
	total = np.zeros((size_y, size_x))
	amp_sum = 0.0
	amp = 1.0
	for c in octaves:
		total += value_noise(size_x, size_y, c, max(1, c // 2), rng) * amp
		amp_sum += amp
		amp *= 0.55
	return total / amp_sum


def save(path, rgb, alpha=None):
	os.makedirs(os.path.dirname(path), exist_ok=True)
	img = np.clip(rgb, 0.0, 1.0)
	if alpha is not None:
		img = np.concatenate([img, np.clip(alpha, 0.0, 1.0)[..., None]], axis=2)
	Image.fromarray((img * 255 + 0.5).astype(np.uint8)).save(path)
	print("wrote", os.path.relpath(path, ROOT))


def make_night_sky(rng):
	"""暗く晴れた夜空（横 2048 × 縦 1024。v=0 が真上、v=0.5 が地平線）"""
	w, h = 2048, 1024
	ys, xs = np.mgrid[0:h, 0:w]
	elev = (0.5 - ys / h) * math.pi		# 高さの角度（+90度：真上、0：地平線、-90度：真下）

	# 地平線から上へ、霧の色 → 紺 → 黒に近い紺
	up = np.clip(elev / (math.pi / 2), 0.0, 1.0)
	t = 1.0 - (1.0 - up) ** 4			# 地平線のすぐ上だけ明るく、上はすぐ暗くなる
	color = HORIZON_COLOR[None, None, :] * (1 - t[..., None]) + ZENITH_COLOR[None, None, :] * t[..., None]

	# ごく薄いもや（真っ平らな色に見えないように、少しだけムラを付ける）
	haze = fractal(w, h, rng, (6, 12, 24))
	color *= (0.85 + 0.3 * haze)[..., None]

	# 星：上ほど多く、地平線の近くは霧に隠れて見えない
	stars = np.zeros((h, w))
	n = 2600
	sx = rng.integers(0, w, n)
	sy = (rng.random(n) ** 1.6 * h * 0.47).astype(int)	# 上の方に多く
	bright = rng.random(n) ** 6 * 0.9 + 0.08			# ほとんどは暗い星、たまに明るい星
	for x, y, b in zip(sx, sy, bright):
		fade = np.clip((0.5 - y / h) / 0.12, 0.0, 1.0)	# 地平線に近いほど薄く
		stars[y, x] = max(stars[y, x], b * fade)
		if b > 0.5:									# 明るい星は少し大きく
			for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
				stars[(y + dy) % h, (x + dx) % w] = max(stars[(y + dy) % h, (x + dx) % w], b * fade * 0.35)
	star_tint = np.array((0.9, 0.92, 1.0))
	color = color + stars[..., None] * star_tint[None, None, :]

	# 地平線より下は、霧の色（壁の向こうに見えても浮かないように）
	below = elev < 0
	color[below] = HORIZON_COLOR * 0.9
	save(os.path.join(SKY_DIR, "NightSky.png"), color)


def make_moon(rng):
	"""少し赤い月（球に貼る。横 1024 × 縦 512）。海（暗い所）とクレーター"""
	w, h = 1024, 512
	ys, xs = np.mgrid[0:h, 0:w]
	lon = xs / w * 2 * math.pi
	lat = (0.5 - ys / h) * math.pi
	# 球の上の位置（クレーターの距離を球の上で測るため）
	p = np.stack([np.cos(lat) * np.cos(lon), np.sin(lat), np.cos(lat) * np.sin(lon)], axis=-1)

	base = 0.85 + 0.15 * fractal(w, h, rng, (16, 32, 64, 128))
	maria = np.clip((fractal(w, h, rng, (2, 4, 8)) - 0.48) / 0.12, 0.0, 1.0)	# 海（暗く広い所）
	shade = base * (1.0 - 0.45 * maria)

	# クレーター（小さいものほど多い。ふちがほんの少し明るく、中が少し暗い丸）
	for _ in range(90):
		c_lon = rng.random() * 2 * math.pi
		c_lat = math.asin(rng.random() * 2 - 1)
		c = np.array([math.cos(c_lat) * math.cos(c_lon), math.sin(c_lat), math.cos(c_lat) * math.sin(c_lon)])
		r = rng.random() ** 3 * 0.12 + 0.01
		d = np.arccos(np.clip(p @ c, -1.0, 1.0)) / r
		inside = np.clip(1.0 - d, 0.0, 1.0)
		rim = np.exp(-((d - 1.0) / 0.2) ** 2)
		shade *= 1.0 - 0.15 * inside ** 0.7
		shade += 0.05 * rim
	color = MOON_COLOR[None, None, :] * shade[..., None]
	save(os.path.join(SKY_DIR, "Moon.png"), color)


def make_moon_glow():
	"""月のまわりのにじみ（真ん中から外へ、なめらかに消える丸。加算で重ねる）"""
	s = 256
	ys, xs = np.mgrid[0:s, 0:s]
	d = np.sqrt((xs - s / 2 + 0.5) ** 2 + (ys - s / 2 + 0.5) ** 2) / (s / 2)
	a = np.clip(1.0 - d, 0.0, 1.0) ** 2.5
	# ゲームのシェーダーはアルファが 0.55 より小さい所を描かないので、アルファは使わず、色の明るさで表す（加算で重ねる）
	color = MOON_GLOW_COLOR[None, None, :] * a[..., None]
	save(os.path.join(SKY_DIR, "MoonGlow.png"), color)


def make_ground_mist(rng):
	"""地面の霧の1かたまり（白っぽい、もやもやした形。ふちは透明）"""
	s = 256
	ys, xs = np.mgrid[0:s, 0:s]
	d = np.sqrt((xs - s / 2 + 0.5) ** 2 + (ys - s / 2 + 0.5) ** 2) / (s / 2)
	noise = fractal(s, s, rng, (4, 8, 16, 32))
	a = np.clip(1.0 - d * (1.1 - 0.5 * noise), 0.0, 1.0) ** 2 * (0.4 + 0.6 * noise)
	# ゲームのシェーダーはアルファが 0.55 より小さい所を描かないので、アルファは使わず、白黒の明るさで濃さを表す
	# （白：濃い、黒：何もない。加算で重ね、色はゲームの側で付ける）
	a = a / max(a.max(), 1e-6)
	save(os.path.join(TEX_DIR, "GroundMist.png"), np.repeat(a[..., None], 3, axis=2))


def make_sphere_models():
	"""Sky.gltf と同じ球（Sky.bin）に、別の画像を貼ったモデルを書き出す（形は作り直さない）"""
	import json
	with open(os.path.join(SKY_DIR, "Sky.gltf"), encoding="utf-8") as f:
		base = json.load(f)
	for name, image in (("NightSky", "NightSky.png"), ("Moon", "Moon.png")):
		g = json.loads(json.dumps(base))
		g["images"][0]["uri"] = image
		g["images"][0]["name"] = image
		with open(os.path.join(SKY_DIR, name + ".gltf"), "w", encoding="utf-8") as f:
			json.dump(g, f, ensure_ascii=False, indent=1)
		print("wrote", os.path.relpath(os.path.join(SKY_DIR, name + ".gltf"), ROOT))


def main():
	rng = np.random.default_rng(SEED)
	make_night_sky(rng)
	make_moon(rng)
	make_moon_glow()
	make_ground_mist(rng)
	make_sphere_models()


if __name__ == "__main__":
	main()
