# ゲームクリア・ゲームオーバー画面の背景画像を作るスクリプト
# 使い方：python Tools/MakeResultBackground.py
# Asset/Textures/UI/Result/ に GameClear.png と GameOver.png（1280x720）を作る
#
# どちらも「崩れた大聖堂のシルエット」と空のグラデーションで、ゲームのステージと雰囲気を合わせている
#   GameClear：夜明けの金色の空、差し込む光
#   GameOver ：赤黒い空、赤い月、立ちこめる霧

import math
import os
import struct
import zlib

import numpy as np

W, H = 1280, 720
SEED = 1019


def write_png(path, rgb):
	data = (np.clip(rgb, 0, 1) * 255).astype(np.uint8)
	raw = b"".join(b"\x00" + data[y].tobytes() for y in range(H))

	def chunk(kind, body):
		return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body) & 0xffffffff)

	with open(path, "wb") as f:
		f.write(b"\x89PNG\r\n\x1a\n")
		f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 2, 0, 0, 0)))
		f.write(chunk(b"IDAT", zlib.compress(raw, 9)))
		f.write(chunk(b"IEND", b""))


def value_noise(w, h, cells_x, cells_y, rng):
	grid = rng.random((cells_y + 2, cells_x + 2))
	ty = np.linspace(0, cells_y, h, endpoint=False)
	tx = np.linspace(0, cells_x, w, endpoint=False)
	iy, ix = ty.astype(int), tx.astype(int)
	fy, fx = ty - iy, tx - ix
	fy = (fy * fy * (3 - 2 * fy))[:, None]
	fx = (fx * fx * (3 - 2 * fx))[None, :]
	a = grid[iy][:, ix]
	b = grid[iy][:, ix + 1]
	c = grid[iy + 1][:, ix]
	d = grid[iy + 1][:, ix + 1]
	return (a * (1 - fx) + b * fx) * (1 - fy) + (c * (1 - fx) + d * fx) * fy


def clouds(rng, scale=1.0):
	total = np.zeros((H, W))
	amp, norm = 1.0, 0.0
	for cells in (4, 8, 16, 32, 64):
		total += value_noise(W, H, int(cells * 1.8 * scale), int(cells * scale), rng) * amp
		norm += amp
		amp *= 0.5
	return total / norm


def ruins_mask(rng, ground_y, height_scale, x_offset=0.0):
	"""崩れた大聖堂のシルエット（1：建物、0：空）"""
	mask = np.zeros((H, W))
	ys = np.arange(H)[:, None]
	xs = np.arange(W)[None, :]

	def rect(x0, x1, top):
		x0, x1 = int(max(0, x0 + x_offset)), int(min(W, x1 + x_offset))
		if x1 <= x0:
			return
		mask[int(max(0, top)):, x0:x1] = 1.0

	# 地面
	mask[int(ground_y):, :] = 1.0

	# 両わきの高い壁（窓の穴つき・上が崩れている）
	for side in (-1, 1):
		base_x = W / 2 + side * 330
		for i in range(5):
			x = base_x + side * i * 70
			top = ground_y - height_scale * rng.uniform(0.55, 0.95)
			# 柱
			rect(x - 18, x + 18, top)
			# 柱の間の壁（ギザギザの上端）
			wall_top = ground_y - height_scale * rng.uniform(0.25, 0.55)
			seg_x0 = x + (18 if side > 0 else -52)
			for k in range(6):
				rect(seg_x0 + k * 6, seg_x0 + (k + 1) * 6, wall_top + rng.uniform(-14, 14))
			# 窓の穴（とがったアーチ）
			wx = x + side * 35
			win_top = ground_y - height_scale * 0.48
			win_bot = ground_y - height_scale * 0.25
			inside = (np.abs(xs - (wx + x_offset)) < 10) & (ys > win_top) & (ys < win_bot)
			arch = (ys <= win_top) & (ys > win_top - 14) & (np.abs(xs - (wx + x_offset)) < 10 * (1 - (win_top - ys) / 14))
			mask[inside | arch] = 0.0

	# 真ん中の、半分崩れた大きなアーチ
	cx = W / 2
	r_out, r_in = 210, 160
	arch_y = ground_y - height_scale * 0.6
	dist = np.sqrt((xs - (cx + x_offset)) ** 2 + ((ys - arch_y) * 1.15) ** 2)
	ang = np.arctan2(-(ys - arch_y), xs - (cx + x_offset))
	ring = (dist < r_out) & (dist > r_in) & (ys <= arch_y) & (ang > math.pi * 0.42)	# 左半分だけ残っている
	mask[ring] = 1.0
	rect(cx - r_out, cx - r_in, arch_y)	# 左の柱
	rect(cx + r_in, cx + r_out, ground_y - height_scale * 0.32)	# 右は途中で折れている
	# 折れた柱と瓦礫
	for _ in range(25):
		x = rng.uniform(0, W)
		w = rng.uniform(8, 30)
		h = rng.uniform(6, 40)
		rect(x, x + w, ground_y - h)
	return mask


def blur(img, radius):
	"""簡単なぼかし（横と縦に平均を取る）"""
	if radius <= 0:
		return img
	k = np.ones(radius * 2 + 1) / (radius * 2 + 1)
	out = np.apply_along_axis(lambda r: np.convolve(r, k, mode="same"), 1, img)
	out = np.apply_along_axis(lambda c: np.convolve(c, k, mode="same"), 0, out)
	return out


def vignette(strength):
	ys, xs = np.mgrid[0:H, 0:W]
	d = np.sqrt(((xs - W / 2) / (W / 2)) ** 2 + ((ys - H / 2) / (H / 2)) ** 2)
	return np.clip(1.0 - strength * (d ** 2) * 0.5, 0, 1)[..., None]


def make_clear(rng):
	ys = np.linspace(0, 1, H)[:, None, None]
	# 空：上は紫がかった紺 → 地平線は金色
	top = np.array([0.10, 0.10, 0.22])
	mid = np.array([0.55, 0.32, 0.30])
	low = np.array([1.00, 0.78, 0.40])
	t = ys / 0.72
	sky = np.where(t < 0.6, top + (mid - top) * (t / 0.6), mid + (low - mid) * np.clip((t - 0.6) / 0.4, 0, 1))
	sky = np.broadcast_to(sky, (H, W, 3)).copy()

	# 太陽の光（アーチの奥から差し込む）
	yy, xx = np.mgrid[0:H, 0:W]
	sun = np.exp(-(((xx - W / 2) / 260) ** 2 + ((yy - H * 0.62) / 160) ** 2))
	sky += sun[..., None] * np.array([1.0, 0.85, 0.55]) * 0.9
	# 光の筋
	ang = np.arctan2(yy - H * 0.62, xx - W / 2)
	rays = (np.sin(ang * 18) * 0.5 + 0.5) ** 6 * np.exp(-np.abs(yy - H * 0.62) / 300) * (yy < H * 0.62)
	sky += rays[..., None] * np.array([1.0, 0.8, 0.5]) * 0.25

	# 雲
	c = clouds(rng)
	cloud = np.clip((c - 0.5) * 3, 0, 1) * (ys[..., 0] < 0.6)
	sky = sky * (1 - cloud[..., None] * 0.35) + cloud[..., None] * np.array([1.0, 0.75, 0.6]) * 0.25

	# 遠くの廃墟（かすんでいる）→ 手前の廃墟（暗いシルエット）
	far = blur(ruins_mask(rng, H * 0.74, 220, x_offset=40), 2)
	near = ruins_mask(rng, H * 0.80, 360)
	img = sky * (1 - far[..., None] * 0.55) + far[..., None] * np.array([0.35, 0.25, 0.28]) * 0.55
	edge = np.clip(blur(near, 3) - near, 0, 1)	# シルエットのふちが光る
	img = img * (1 - near[..., None]) + near[..., None] * np.array([0.05, 0.04, 0.06])
	img += edge[..., None] * np.array([1.0, 0.7, 0.35]) * 0.8

	img *= vignette(0.9)
	return img


def make_over(rng):
	ys = np.linspace(0, 1, H)[:, None, None]
	top = np.array([0.03, 0.01, 0.02])
	low = np.array([0.38, 0.04, 0.04])
	sky = top + (low - top) * np.clip(ys / 0.75, 0, 1) ** 1.6
	sky = np.broadcast_to(sky, (H, W, 3)).copy()

	# 赤い月
	yy, xx = np.mgrid[0:H, 0:W]
	mx, my, mr = W * 0.72, H * 0.26, 70
	d = np.sqrt((xx - mx) ** 2 + (yy - my) ** 2)
	moon = np.clip((mr - d) / 2, 0, 1)
	glow = np.exp(-((d / 220) ** 2))
	surface = 0.8 + 0.2 * value_noise(W, H, 40, 22, rng)
	sky += glow[..., None] * np.array([0.6, 0.06, 0.04]) * 0.6
	sky = sky * (1 - moon[..., None]) + moon[..., None] * np.array([0.85, 0.18, 0.12]) * surface[..., None]

	# 重い雲
	c = clouds(rng)
	cloud = np.clip((c - 0.45) * 2.5, 0, 1) * (ys[..., 0] < 0.7)
	sky = sky * (1 - cloud[..., None] * 0.6) + cloud[..., None] * np.array([0.12, 0.02, 0.03]) * 0.6

	far = blur(ruins_mask(rng, H * 0.74, 220, x_offset=-30), 2)
	near = ruins_mask(rng, H * 0.82, 340)
	img = sky * (1 - far[..., None] * 0.7) + far[..., None] * np.array([0.10, 0.02, 0.03]) * 0.7
	img = img * (1 - near[..., None]) + near[..., None] * np.array([0.015, 0.01, 0.012])

	# 地面をはう霧
	fog_n = clouds(rng, scale=0.6)
	fog = np.clip((yy / H - 0.62) * 2.5, 0, 1) * (0.4 + 0.6 * fog_n)
	img = img * (1 - fog[..., None] * 0.6) + fog[..., None] * np.array([0.35, 0.05, 0.06]) * 0.6

	img *= vignette(1.2)
	return img


def main():
	# ruins_mask は rng.uniform を使う（numpy の乱数にも同じ名前の関数がある）
	rng = np.random.default_rng(SEED)
	root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
	out_dir = os.path.join(root, "Asset", "Textures", "UI", "Result")
	os.makedirs(out_dir, exist_ok=True)

	write_png(os.path.join(out_dir, "GameClear.png"), make_clear(rng))
	write_png(os.path.join(out_dir, "GameOver.png"), make_over(rng))
	print("作成:", os.path.normpath(out_dir))


if __name__ == "__main__":
	main()
