# 今使っているエフェクト（.efkefc）を調整して、同じ名前の .efkproj を作るスクリプト
# 使い方：python Tools/AdjustExistingEffects.py
#   → Asset/Data/Effect/ に _boss_death.efkproj などができる
#   → Effekseer エディタで開いて「名前を付けて保存」で同じ名前の .efkefc に上書きする
#
# .efkefc の中には、エディタ用のデータ（EDIT。圧縮した XML のようなもの）が入っているので、
# それを XML に戻して、数値を書き換えてから .efkproj として書き出す（元の .efkefc は書き換えない）

import os
import struct
import zlib
import xml.etree.ElementTree as ET

EFFECT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Asset", "Data", "Effect")


# ===== .efkefc → XML =====

def read_chunks(data):
	"""EFKE ファイルの中のかたまり（INFO / EDIT / BIN_）を取り出す"""
	chunks = {}
	pos = 8
	while pos < len(data):
		tag = data[pos:pos + 4]
		size = struct.unpack("<I", data[pos + 4:pos + 8])[0]
		chunks[tag] = data[pos + 8:pos + 8 + size]
		pos += 8 + size
	return chunks


def decode_edit(edit):
	"""EDIT のかたまりを XML の木にする
	中身：要素名の表、値の表、木（要素名の番号・値の数・値の番号・子があるか・子の数）"""
	x = zlib.decompress(edit)
	pos = 0

	def u16():
		nonlocal pos
		v = struct.unpack("<H", x[pos:pos + 2])[0]
		pos += 2
		return v

	def u32():
		nonlocal pos
		v = struct.unpack("<I", x[pos:pos + 4])[0]
		pos += 4
		return v

	def table():
		nonlocal pos
		result = {}
		for _ in range(u16()):
			length = u16()
			text = x[pos:pos + length].decode("utf-8")
			pos += length
			result[u16()] = text
		return result

	names = table()
	values = table()

	def node():
		elem = ET.Element(names[u16()])
		texts = [values[u16()] for _ in range(u32())]
		if texts:
			elem.text = "".join(texts)
		if u32():
			for _ in range(u16()):
				elem.append(node())
		return elem

	roots = [node() for _ in range(u16())]
	return roots[0]


def load(name):
	with open(os.path.join(EFFECT_DIR, name + ".efkefc"), "rb") as f:
		return decode_edit(read_chunks(f.read())[b"EDIT"])


def save(name, root):
	path = os.path.join(EFFECT_DIR, name + ".efkproj")
	ET.indent(root)
	with open(path, "wb") as f:
		f.write(b'<?xml version="1.0" encoding="utf-8"?>\n')
		f.write(ET.tostring(root, encoding="utf-8"))
	print("作成:", os.path.normpath(path))


# ===== 書き換えの道具 =====

def nodes(root):
	"""すべてのノード（エフェクトの部品）"""
	return root.iter("Node")


def find_node(root, name):
	for n in nodes(root):
		if n.findtext("Name") == name:
			return n
	raise KeyError(name)


def remove_node(root, name):
	"""名前の部品を消す（同じ名前が複数あれば全部）"""
	for parent in root.iter("Children"):
		for child in list(parent):
			if child.tag == "Node" and child.findtext("Name") == name:
				parent.remove(child)


def child(elem, *path):
	"""elem の下の path の要素（無ければ作る）"""
	for tag in path:
		found = elem.find(tag)
		if found is None:
			found = ET.SubElement(elem, tag)
		elem = found
	return elem


def set_range(node, path, value):
	"""Life などの「Center / Max / Min」をまとめて value にする"""
	elem = child(node, *path)
	for tag in ("Center", "Max", "Min"):
		child(elem, tag).text = str(value)


def scale_generation(root, rate, min_count, threshold):
	"""たくさん出す部品（MaxGeneration が threshold より多い）の数を rate 倍に減らす
	※リボン（3）・軌跡（6）は、出す数が帯の長さになっているので変えない"""
	for n in nodes(root):
		if n.findtext("DrawingValues/Type") in ("3", "6"):
			continue
		v = n.find("CommonValues/MaxGeneration/Value")
		if v is not None and int(v.text) > threshold:
			v.text = str(max(int(int(v.text) * rate), min_count))


# ===== 調整の中身 =====

def adjust_boss_death():
	"""敵の撃破：6秒以上続く大きな演出を、約1.5秒の小さな演出にする
	（光の柱・レーザーの帯・後半のキラキラを消し、最後の光をすぐに出す）"""
	root = load("_boss_death")
	for name in ("Light Beams", "Ribbon Guide", "Twinkle Faster"):
		remove_node(root, name)
	set_range(find_node(root, "Base"), ("CommonValues", "Life"), 90)
	set_range(find_node(root, "Ambient Glow"), ("CommonValues", "Life"), 90)
	final = find_node(root, "Final Flash")
	set_range(final, ("CommonValues", "GenerationTimeOffset"), 20)
	set_range(final, ("CommonValues", "Life"), 60)
	set_range(find_node(root, "Twinkle"), ("CommonValues", "GenerationTimeOffset"), 0)
	child(root, "EndFrame").text = "90"
	save("_boss_death", root)


def adjust_claw():
	"""敵の攻撃が当たった：粒・稲妻・放射の線・ゆがみの数を減らして、見通しを良くする"""
	root = load("_Claw2")
	scale_generation(root, 0.15, 8, threshold=5)
	save("_Claw2", root)


def main():
	adjust_boss_death()
	adjust_claw()


if __name__ == "__main__":
	main()
