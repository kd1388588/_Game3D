# BGM と SE（効果音）を計算で作るスクリプト
# 使い方：pip install numpy scipy
#         python Tools/MakeSound.py
# Asset/Sound/BGM/ と Asset/Sound/SE/ に WAV（44.1kHz・16bit・モノラル）を作る
#
# BGM（ダークファンタジー。ニ短調）
#   Title.wav    ：ゆっくりした和音・低い持続音・鐘の音（ループ）
#   Battle.wav   ：140BPM のドラム・ベース・弦・アルペジオ（ループ）
#   Clear.wav    ：勝利のファンファーレ（1回）
#   GameOver.wav ：沈んだ短調のフレーズ（1回）
# SE
#   斬撃・ヒット・クリティカル・回避・ジャンプ・着地・ガード・ガード崩し・モード切り替え・
#   技の発動・敵の撃破・ボスの叩きつけ・被弾・メニュー（選択・決定・開く）

import os
import wave

import numpy as np
from scipy.signal import butter, lfilter

SR = 44100
RNG = np.random.default_rng(1020)


# ------------------------------------------------------------
# 音の部品
# ------------------------------------------------------------
def secs(n):
	return int(n * SR)


def tt(dur):
	return np.arange(secs(dur)) / SR


def midi(n):
	return 440.0 * 2 ** ((n - 69) / 12)


def sine(freq, dur, phase=0.0):
	t = tt(dur)
	if np.ndim(freq) == 0:
		return np.sin(2 * np.pi * freq * t + phase)
	return np.sin(2 * np.pi * np.cumsum(freq) / SR + phase)


def saw(freq, dur, harmonics=24):
	"""ギザギザの波（倍音を足し合わせて作る。高い音が割れないように倍音の数を制限）"""
	t = tt(dur)
	out = np.zeros_like(t)
	for k in range(1, harmonics + 1):
		if freq * k > SR / 2 - 1000:
			break
		out += np.sin(2 * np.pi * freq * k * t) / k
	return out * 0.6


def square(freq, dur, harmonics=15):
	t = tt(dur)
	out = np.zeros_like(t)
	for k in range(1, harmonics * 2, 2):
		if freq * k > SR / 2 - 1000:
			break
		out += np.sin(2 * np.pi * freq * k * t) / k
	return out * 0.8


def noise(dur):
	return RNG.uniform(-1, 1, secs(dur))


def env(dur, a=0.005, d=0.1, s=0.7, r=0.2):
	"""音の立ち上がり・減り方（ADSR）"""
	n = secs(dur)
	e = np.ones(n) * s
	na, nd, nr = secs(a), secs(d), secs(r)
	na = max(1, min(na, n))
	e[:na] = np.linspace(0, 1, na)
	nd = min(nd, n - na)
	if nd > 0:
		e[na:na + nd] = np.linspace(1, s, nd)
	nr = min(nr, n)
	if nr > 0:
		e[n - nr:] *= np.linspace(1, 0, nr)
	return e


def decay(dur, rate):
	return np.exp(-tt(dur) * rate)


def lowpass(x, cutoff, order=2):
	b, a = butter(order, min(cutoff, SR / 2 - 100) / (SR / 2), "low")
	return lfilter(b, a, x)


def highpass(x, cutoff, order=2):
	b, a = butter(order, cutoff / (SR / 2), "high")
	return lfilter(b, a, x)


def bandpass(x, lo, hi, order=2):
	b, a = butter(order, [lo / (SR / 2), min(hi, SR / 2 - 100) / (SR / 2)], "band")
	return lfilter(b, a, x)


def sweep_filter(x, start, end, block=512):
	"""ローパスのかかり具合を時間で変える（シュッという風切り音用）"""
	out = np.zeros_like(x)
	zi = None
	n = len(x)
	for i in range(0, n, block):
		f = start + (end - start) * (i / n)
		b, a = butter(2, max(60, min(f, SR / 2 - 200)) / (SR / 2), "band" if False else "low")
		seg = x[i:i + block]
		if zi is None:
			zi = np.zeros(max(len(a), len(b)) - 1)
		y, zi = lfilter(b, a, seg, zi=zi)
		out[i:i + block] = y
	return out


def reverb(x, mix=0.3, room=0.84, tail=2.0):
	"""簡単な残響（くし形フィルタ 4つ ＋ オールパス 2つ）"""
	x = np.concatenate([x, np.zeros(secs(tail))])
	wet = np.zeros_like(x)
	for delay_ms, g in ((29.7, room), (37.1, room - 0.02), (41.1, room - 0.04), (43.7, room - 0.05)):
		d = int(SR * delay_ms / 1000)
		a = np.zeros(d + 1)
		a[0] = 1.0
		a[d] = -g
		wet += lfilter([1.0], a, x)
	wet /= 4
	for delay_ms, g in ((5.0, 0.7), (1.7, 0.7)):
		d = int(SR * delay_ms / 1000)
		b = np.zeros(d + 1)
		b[0] = -g
		b[d] = 1.0
		a = np.zeros(d + 1)
		a[0] = 1.0
		a[d] = -g
		wet = lfilter(b, a, wet)
	wet = lowpass(wet, 6000)
	return x * (1 - mix) + wet * mix


def place(buf, sound, at, gain=1.0):
	"""buf の at 秒の位置に sound を足す（はみ出した分は切る）"""
	i = secs(at)
	if i >= len(buf):
		return
	n = min(len(sound), len(buf) - i)
	buf[i:i + n] += sound[:n] * gain


def soft_clip(x, drive=1.0):
	return np.tanh(x * drive) / np.tanh(drive)


def finish(x, peak=0.89):
	x = x - np.mean(x)
	m = np.max(np.abs(x))
	if m > 0:
		x = x / m * peak
	return x


def fade_edges(x, fade_in=0.002, fade_out=0.01):
	x = x.copy()
	ni, no = secs(fade_in), secs(fade_out)
	if ni > 0:
		x[:ni] *= np.linspace(0, 1, ni)
	if no > 0:
		x[-no:] *= np.linspace(1, 0, no)
	return x


def write_wav(path, x):
	data = (np.clip(x, -1, 1) * 32767).astype(np.int16)
	with wave.open(path, "wb") as w:
		w.setnchannels(1)
		w.setsampwidth(2)
		w.setframerate(SR)
		w.writeframes(data.tobytes())


def make_loop(render, length, tail):
	"""ループ用：最後にはみ出した残響を先頭に重ねて、つなぎ目が分からないようにする"""
	full = render(length + tail)
	n = secs(length)
	out = full[:n].copy()
	rest = full[n:]
	out[:len(rest)] += rest
	return out


# ------------------------------------------------------------
# 楽器
# ------------------------------------------------------------
def kick():
	d = 0.35
	f = 45 + 110 * np.exp(-tt(d) * 30)
	return sine(f, d) * decay(d, 9) + lowpass(noise(d), 800) * decay(d, 80) * 0.3


def snare():
	d = 0.25
	tone = sine(185, d) * decay(d, 25) * 0.6
	n = bandpass(noise(d), 1200, 8000) * decay(d, 18)
	return tone + n


def hat(open_=False):
	d = 0.25 if open_ else 0.06
	return highpass(noise(d), 7000) * decay(d, 14 if open_ else 70) * 0.5


def bass_note(note, dur):
	x = saw(midi(note), dur, 30) * 0.7 + sine(midi(note) / 2, dur) * 0.5
	x = lowpass(x, 700)
	return x * env(dur, 0.004, 0.08, 0.6, 0.03)


def pad(notes, dur, bright=1800, attack=0.6):
	x = np.zeros(secs(dur))
	for n in notes:
		for detune in (-0.08, 0.0, 0.08):
			x += saw(midi(n + detune), dur, 18)
	x = lowpass(x / (len(notes) * 3), bright)
	return x * env(dur, attack, 0.3, 0.8, min(1.2, dur * 0.4))


def pluck(note, dur=0.25, bright=3500):
	x = square(midi(note), dur, 9) * 0.5 + saw(midi(note), dur, 12) * 0.5
	x = lowpass(x, bright)
	return x * decay(dur, 14) * env(dur, 0.002, 0.05, 1.0, 0.02)


def bell(note, dur=3.0):
	f = midi(note)
	x = np.zeros(secs(dur))
	for ratio, amp, rate in ((1.0, 1.0, 1.2), (2.76, 0.5, 2.0), (5.4, 0.3, 3.5), (8.93, 0.15, 5.0)):
		x += sine(f * ratio, dur) * amp * decay(dur, rate)
	return x * env(dur, 0.002, 0.05, 1.0, 0.3)


def brass(note, dur):
	x = saw(midi(note), dur, 30)
	x = lowpass(x, 2200)
	return x * env(dur, 0.05, 0.2, 0.75, 0.25)


def timpani(note, dur=1.2):
	f = midi(note)
	x = sine(f, dur) * decay(dur, 3) + sine(f * 1.5, dur) * decay(dur, 5) * 0.4
	x += lowpass(noise(dur), 500) * decay(dur, 25) * 0.6
	return x


# ------------------------------------------------------------
# BGM
# ------------------------------------------------------------
def bgm_battle():
	bpm = 140
	beat = 60 / bpm
	bars = 16
	length = bars * 4 * beat
	# 和音：Dm → B♭ → C → A（2小節ずつ）
	prog = [(38, [50, 53, 57]), (34, [50, 53, 58]), (36, [48, 52, 55]), (33, [49, 52, 57])]

	def render(total):
		buf = np.zeros(secs(total))
		for bar in range(bars):
			root, chord = prog[(bar // 2) % 4]
			t0 = bar * 4 * beat
			# ドラム
			for b in range(4):
				place(buf, kick(), t0 + b * beat, 0.9 if b in (0, 2) else 0.0)
				if b in (1, 3):
					place(buf, snare(), t0 + b * beat, 0.55)
				for h in range(2):
					place(buf, hat(open_=(h == 1 and b == 3)), t0 + b * beat + h * beat / 2, 0.25)
			# 駆け足のキック（後半）
			if bar >= 8:
				place(buf, kick(), t0 + 3.5 * beat, 0.6)
			# ベース（8分音符の刻み）
			pattern = [0, 0, 12, 0, 0, 12, 0, 10]
			for i, off in enumerate(pattern):
				place(buf, bass_note(root + off, beat / 2 * 0.95), t0 + i * beat / 2, 0.55)
			# 弦（和音）
			if bar % 2 == 0:
				place(buf, pad(chord, 8 * beat, bright=1500, attack=0.3), t0, 0.35)
			# アルペジオ（後半で盛り上げる）
			if bar >= 4:
				arp = chord + [chord[0] + 12]
				for i in range(16):
					note = arp[(i if bar < 8 else (i * 3)) % len(arp)] + 12
					place(buf, pluck(note, 0.22), t0 + i * beat / 4, 0.18 if bar < 8 else 0.24)
			# 後半の旋律（鐘の音で強調）
			if bar >= 8 and bar % 2 == 0:
				place(buf, bell(chord[2] + 12, 2.0), t0, 0.18)
		buf = reverb(buf, mix=0.18, room=0.8, tail=0.0)
		return buf

	x = make_loop(render, length, tail=2.5)
	return finish(soft_clip(x * 1.2, 1.2))


def bgm_title():
	length = 32.0
	chords = [(26, [50, 53, 57, 62]), (22, [50, 53, 58, 62]), (29, [48, 53, 57, 60]), (21, [49, 52, 57, 61])]

	def render(total):
		buf = np.zeros(secs(total))
		for i, (root, chord) in enumerate(chords):
			t0 = i * 8.0
			place(buf, pad(chord, 9.0, bright=1100, attack=2.5), t0, 0.5)
			drone = lowpass(saw(midi(root), 9.0, 12), 300) * env(9.0, 2.0, 0.5, 0.9, 2.0)
			place(buf, drone, t0, 0.35)
		# まばらな鐘のメロディ
		melody = [(2.0, 74), (5.0, 72), (6.5, 69), (10.0, 70), (13.0, 69), (18.0, 72), (21.0, 74), (22.5, 77), (26.0, 76), (29.0, 73)]
		for at, note in melody:
			place(buf, bell(note, 3.5), at, 0.22)
		buf = reverb(buf, mix=0.45, room=0.88, tail=0.0)
		return buf

	x = make_loop(render, length, tail=4.0)
	return finish(x, 0.8)


def jingle_clear():
	beat = 0.16
	buf = np.zeros(secs(6.0))
	# ニ長調の駆け上がり → 和音を伸ばす
	for i, n in enumerate([62, 66, 69, 74]):
		place(buf, brass(n, beat * 1.1), i * beat, 0.5)
		place(buf, brass(n - 12, beat * 1.1), i * beat, 0.3)
	t = 4 * beat
	for n in (62, 66, 69, 74, 78):
		place(buf, brass(n, 3.2), t, 0.28)
	place(buf, brass(50, 3.2), t, 0.35)
	place(buf, timpani(38), t, 0.8)
	place(buf, timpani(38, 1.5), t + 0.3, 0.5)
	place(buf, bell(86, 3.0), t, 0.15)
	buf = reverb(buf, mix=0.3, room=0.85, tail=1.0)
	return finish(fade_edges(buf, 0.001, 0.8))


def jingle_gameover():
	buf = np.zeros(secs(7.0))
	notes = [(0.0, 69), (0.9, 65), (1.8, 62), (2.7, 61)]
	for at, n in notes:
		place(buf, pad([n, n - 12], 1.6, bright=1200, attack=0.05), at, 0.6)
	place(buf, pad([50, 53, 57, 38], 4.0, bright=900, attack=0.3), 3.4, 0.6)
	place(buf, bell(62, 4.0), 3.4, 0.25)
	place(buf, timpani(26, 2.0), 3.4, 0.6)
	buf = reverb(buf, mix=0.4, room=0.88, tail=1.5)
	return finish(fade_edges(buf, 0.001, 1.5))


# ------------------------------------------------------------
# SE
# ------------------------------------------------------------
def se_slash():
	d = 0.22
	x = sweep_filter(noise(d), 9000, 1500) * env(d, 0.01, 0.05, 0.6, 0.12)
	x += bandpass(noise(d), 3000, 9000) * decay(d, 30) * 0.3
	return finish(fade_edges(x), 0.7)


def se_hit():
	d = 0.3
	body = sine(40 + 90 * np.exp(-tt(d) * 40), d) * decay(d, 14)
	crack = bandpass(noise(d), 800, 5000) * decay(d, 45)
	flesh = lowpass(noise(d), 1200) * decay(d, 20) * 0.5
	return finish(fade_edges(soft_clip(body + crack * 0.8 + flesh, 1.5)), 0.85)


def se_critical():
	d = 0.6
	x = np.zeros(secs(d))
	x[:secs(0.3)] += se_hit() * 0.9
	for f, a in ((1250, 1.0), (1870, 0.6), (2630, 0.4), (3510, 0.25)):
		x += sine(f, d) * decay(d, 7) * a * 0.25
	return finish(fade_edges(x), 0.9)


def se_evade():
	d = 0.32
	x = sweep_filter(noise(d), 1200, 4500) * env(d, 0.06, 0.08, 0.5, 0.15)
	return finish(fade_edges(x), 0.55)


def se_jump():
	d = 0.2
	x = sweep_filter(noise(d), 800, 3500) * env(d, 0.02, 0.05, 0.4, 0.1)
	x += lowpass(noise(d), 300) * decay(d, 30) * 0.5
	return finish(fade_edges(x), 0.5)


def se_land():
	d = 0.25
	x = sine(35 + 50 * np.exp(-tt(d) * 30), d) * decay(d, 18)
	x += lowpass(noise(d), 900) * decay(d, 30) * 0.5
	return finish(fade_edges(x), 0.6)


def se_guard():
	d = 0.7
	x = np.zeros(secs(d))
	for f, a, r in ((523, 1.0, 6), (1347, 0.7, 8), (2213, 0.5, 10), (3170, 0.35, 12), (4410, 0.2, 16)):
		x += sine(f, d) * decay(d, r) * a
	x += highpass(noise(d), 2000) * decay(d, 60) * 1.2
	return finish(fade_edges(x * 0.4), 0.85)


def se_guard_break():
	d = 1.0
	x = np.zeros(secs(d))
	for i in range(6):
		at = i * 0.035
		place(x, highpass(noise(0.3), 1500) * decay(0.3, 15), at, 0.7 - i * 0.08)
	for f, r in ((900, 5), (1330, 6), (2080, 8)):
		x += sine(f * np.linspace(1.0, 0.7, secs(d)), d) * decay(d, r) * 0.3
	x += sine(30 + 60 * np.exp(-tt(d) * 10), d) * decay(d, 5) * 0.9
	return finish(fade_edges(soft_clip(x, 1.3)), 0.9)


def se_mode_change():
	d = 0.7
	f = 400 * (4 ** (tt(d) / d))
	x = sine(f, d) * env(d, 0.01, 0.2, 0.6, 0.3) * 0.5
	for ratio in (1.5, 2.0):
		x += sine(f * ratio, d) * env(d, 0.05, 0.2, 0.3, 0.3) * 0.2
	x += highpass(noise(d), 6000) * env(d, 0.1, 0.2, 0.3, 0.3) * 0.15
	x = reverb(x, mix=0.3, room=0.8, tail=0.5)
	return finish(fade_edges(x, 0.002, 0.3), 0.7)


def se_ability():
	d = 1.0
	f = 60 * (5 ** (tt(d) / d))
	x = lowpass(saw(80, d, 30) * 0.0 + sine(f, d), 2000) * env(d, 0.3, 0.2, 0.8, 0.1) * 0.5
	x += sweep_filter(noise(d), 300, 6000) * env(d, 0.4, 0.2, 0.6, 0.1) * 0.5
	place(x, sine(35 + 80 * np.exp(-tt(0.5) * 20), 0.5) * decay(0.5, 8), 0.75, 1.0)
	x = reverb(x, mix=0.25, room=0.8, tail=0.5)
	return finish(fade_edges(soft_clip(x, 1.2), 0.002, 0.3), 0.9)


def se_enemy_death():
	d = 1.6
	x = sine(28 + 70 * np.exp(-tt(d) * 6), d) * decay(d, 3.5)
	x += lowpass(noise(d), 600) * decay(d, 3.0) * 0.8
	x += bandpass(noise(d), 1500, 6000) * decay(d, 12) * 0.4
	x = reverb(x, mix=0.25, room=0.82, tail=0.8)
	return finish(fade_edges(soft_clip(x, 1.6), 0.002, 0.5), 0.95)


def se_boss_smash():
	d = 1.4
	x = sine(25 + 60 * np.exp(-tt(d) * 8), d) * decay(d, 4)
	x += lowpass(noise(d), 400) * decay(d, 4) * 1.0
	for i in range(10):
		place(x, bandpass(noise(0.15), 800, 4000) * decay(0.15, 30), 0.1 + RNG.uniform(0, 0.6), 0.25)
	return finish(fade_edges(soft_clip(x, 1.8), 0.002, 0.4), 0.95)


def se_player_damage():
	d = 0.35
	x = sine(60 + 60 * np.exp(-tt(d) * 25), d) * decay(d, 10)
	x += bandpass(noise(d), 400, 2500) * decay(d, 25) * 0.7
	return finish(fade_edges(soft_clip(x, 2.0)), 0.85)


def se_menu_select():
	d = 0.06
	x = sine(1500, d) * decay(d, 60) + sine(3000, d) * decay(d, 90) * 0.3
	return finish(fade_edges(x, 0.001, 0.01), 0.5)


def se_menu_decide():
	d = 0.5
	x = np.zeros(secs(d))
	for i, n in enumerate((81, 88)):
		place(x, (sine(midi(n), 0.4) + sine(midi(n) * 2, 0.4) * 0.3) * decay(0.4, 8), i * 0.06, 0.6)
	x = reverb(x, mix=0.2, room=0.7, tail=0.2)
	return finish(fade_edges(x, 0.001, 0.1), 0.6)


def se_menu_open():
	d = 0.5
	f = 1400 * (0.3 ** (tt(d) / d))
	x = sine(f, d) * env(d, 0.01, 0.1, 0.5, 0.2) * 0.5
	x += sweep_filter(noise(d), 5000, 800) * env(d, 0.01, 0.1, 0.3, 0.2) * 0.3
	return finish(fade_edges(x), 0.55)


def main():
	root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
	bgm_dir = os.path.join(root, "Asset", "Sound", "BGM")
	se_dir = os.path.join(root, "Asset", "Sound", "SE")
	os.makedirs(bgm_dir, exist_ok=True)
	os.makedirs(se_dir, exist_ok=True)

	bgms = {
		"Title.wav": bgm_title,
		"Battle.wav": bgm_battle,
		"Clear.wav": jingle_clear,
		"GameOver.wav": jingle_gameover,
	}
	ses = {
		"Slash.wav": se_slash,
		"Hit.wav": se_hit,
		"Critical.wav": se_critical,
		"Evade.wav": se_evade,
		"Jump.wav": se_jump,
		"Land.wav": se_land,
		"Guard.wav": se_guard,
		"GuardBreak.wav": se_guard_break,
		"ModeChange.wav": se_mode_change,
		"Ability.wav": se_ability,
		"EnemyDeath.wav": se_enemy_death,
		"BossSmash.wav": se_boss_smash,
		"PlayerDamage.wav": se_player_damage,
		"MenuSelect.wav": se_menu_select,
		"MenuDecide.wav": se_menu_decide,
		"MenuOpen.wav": se_menu_open,
	}
	for name, func in bgms.items():
		write_wav(os.path.join(bgm_dir, name), func())
		print("作成:", "BGM/" + name)
	for name, func in ses.items():
		write_wav(os.path.join(se_dir, name), func())
		print("作成:", "SE/" + name)


if __name__ == "__main__":
	main()
