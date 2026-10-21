# -*- coding: utf-8 -*-
"""
Asset/Music の mp3 から、ゲームで使う wav を作る

  ゲームの音（KdAudioManager / DirectXTK の SoundEffect）は wav しか読めないので、mp3 を wav に変換する。
  使い方：python Tools/MakeSoundAssets.py   （ffmpeg と numpy が必要）

  作るもの
    Asset/Sounds/BGM/Deckrail.wav        … BGM（そのまま変換）
    Asset/Sounds/SE/Slash.wav            … 剣で斬る2.mp3（前後の無音を切る）
    Asset/Sounds/SE/HeavyHit.wav         … 重いパンチ1.mp3（前後の無音を切る）
    Asset/Sounds/SE/Footstep1～4.wav     … アスファルトの上を走る1.mp3 から、足音を1歩ずつ切り出す
                                            （ゲームの中で、足が地面に着いた瞬間に1歩ずつ鳴らすため）
"""
import os
import subprocess
import wave

import numpy as np

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
MUSIC_DIR = os.path.join(ROOT, "Asset", "Music")
BGM_DIR = os.path.join(ROOT, "Asset", "Sounds", "BGM")
SE_DIR = os.path.join(ROOT, "Asset", "Sounds", "SE")

SAMPLE_RATE = 44100

# 足音として切り出す、アスファルトの上を走る1.mp3 の中の1歩の始まり（秒）
# （大きすぎる・小さすぎる歩を避けて、音の大きさがそろっている4歩を選んだ）
FOOTSTEP_STARTS = [0.545, 1.12, 1.415, 2.855]
FOOTSTEP_LENGTH = 0.22		# 1歩の長さ（秒）


def decode(mp3_name, channels):
	"""mp3 を 44.1kHz・16bit の配列にする（[サンプル数, チャンネル数]）"""
	raw = subprocess.run(
		["ffmpeg", "-v", "error", "-i", os.path.join(MUSIC_DIR, mp3_name),
		 "-ac", str(channels), "-ar", str(SAMPLE_RATE), "-f", "s16le", "-"],
		check=True, capture_output=True).stdout
	return np.frombuffer(raw, dtype=np.int16).reshape(-1, channels).astype(np.float32)


def write_wav(path, data):
	os.makedirs(os.path.dirname(path), exist_ok=True)
	pcm = np.clip(data, -32768, 32767).astype(np.int16)
	with wave.open(path, "wb") as w:
		w.setnchannels(pcm.shape[1])
		w.setsampwidth(2)
		w.setframerate(SAMPLE_RATE)
		w.writeframes(pcm.tobytes())
	print("wrote", os.path.relpath(path, ROOT), f"{len(pcm) / SAMPLE_RATE:.2f}s")


def fade(data, fade_in_sec, fade_out_sec):
	data = data.copy()
	n_in = int(SAMPLE_RATE * fade_in_sec)
	n_out = int(SAMPLE_RATE * fade_out_sec)
	if n_in > 0:
		data[:n_in] *= np.linspace(0.0, 1.0, n_in)[:, None]
	if n_out > 0:
		data[-n_out:] *= np.linspace(1.0, 0.0, n_out)[:, None]
	return data


def trim_silence(data, threshold=0.01, margin_sec=0.01):
	"""前後の無音（一番大きい音の threshold 倍より小さい所）を切る"""
	level = np.abs(data).max(axis=1)
	loud = np.where(level > level.max() * threshold)[0]
	margin = int(SAMPLE_RATE * margin_sec)
	start = max(0, loud[0] - margin)
	end = min(len(data), loud[-1] + margin)
	return data[start:end]


def normalize(data, peak=0.9):
	return data * (32767.0 * peak / max(1.0, np.abs(data).max()))


def main():
	# BGM
	write_wav(os.path.join(BGM_DIR, "Deckrail.wav"), decode("Deckrail.mp3", 2))

	# 効果音
	slash = trim_silence(decode("剣で斬る2.mp3", 2))
	write_wav(os.path.join(SE_DIR, "Slash.wav"), fade(normalize(slash), 0.0, 0.05))

	heavy = trim_silence(decode("重いパンチ1.mp3", 2))
	write_wav(os.path.join(SE_DIR, "HeavyHit.wav"), fade(normalize(heavy), 0.0, 0.05))

	# 足音（1歩ずつ）
	run = decode("アスファルトの上を走る1.mp3", 2)
	for i, start in enumerate(FOOTSTEP_STARTS):
		s = int((start - 0.01) * SAMPLE_RATE)
		step = run[s:s + int(FOOTSTEP_LENGTH * SAMPLE_RATE)]
		write_wav(os.path.join(SE_DIR, f"Footstep{i + 1}.wav"), fade(normalize(step, 0.8), 0.005, 0.06))


if __name__ == "__main__":
	main()
