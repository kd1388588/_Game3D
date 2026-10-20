# 今使っているエフェクト（.efkefc）を調整して、同じ名前の .efkproj を作るスクリプト
# 使い方：python Tools/AdjustExistingEffects.py          （_Sword5-1・_Sword5-2 を作る）
#         python Tools/AdjustExistingEffects.py claw     （_Claw2 だけ作り直す。boss_death・sword5_1・sword5_2・sword1_4 も選べる）
#   → Asset/Data/Effect/ に .efkproj ができる
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
	# 最後の光の輪：元は速さ 0.1・加速 0.01 で広がり続ける（60フレームで約24倍）ので、
	# 加速をなくし、短くして、体のまわりで小さく光って消えるようにする
	final = find_node(root, "Final Flash")
	set_range(final, ("CommonValues", "GenerationTimeOffset"), 20)
	set_range(final, ("CommonValues", "Life"), 25)
	set_range(final, ("ScalingValues", "SinglePVA", "Velocity"), 0.08)
	set_range(final, ("ScalingValues", "SinglePVA", "Acceleration"), 0)
	child(final, "RendererCommonValues", "FadeOut", "Frame").text = "15"
	set_range(find_node(root, "Twinkle"), ("CommonValues", "GenerationTimeOffset"), 0)
	child(root, "EndFrame").text = "90"
	save("_boss_death", root)


def adjust_claw():
	"""敵の攻撃が当たった：粒・稲妻・放射の線の数（元は 100）を 15 にして、見通しを良くする
	（何度流しても同じ結果になるように、減らす割合ではなく数を決めて書き換える）"""
	root = load("_Claw2")
	for n in nodes(root):
		if n.findtext("DrawingValues/Type") in ("3", "6"):
			continue
		v = n.find("CommonValues/MaxGeneration/Value")
		if v is not None and int(v.text) >= 50:
			v.text = "15"
	save("_Claw2", root)


def sword5_slash(slash_name):
	"""_Sword5 の4本の斬撃（Nluu～Nluu4）から1本だけ取り出す（傾き 0・すぐ出る。向きはゲーム側で回す）
	元は 0・8・16・24 フレーム目に、Z で -20・20・-70・70 度傾いて出る"""
	root = load("_Sword5")
	children = find_node(root, "SwordNull").find("Children")
	for c in list(children):
		if c.findtext("Name") != slash_name:
			children.remove(c)
	slash = find_node(root, slash_name)
	common = slash.find("CommonValues")
	offset = common.find("GenerationTimeOffset")
	if offset is not None:
		common.remove(offset)
	child(slash, "RotationValues", "Fixed", "Rotation", "Z").text = "0"
	child(root, "StartFrame").text = "0"
	child(root, "EndFrame").text = "45"
	return root, slash


def make_sword5_1():
	"""05_01（2連撃）のヒット：_Sword5 の斬撃1本。当たるたびに、剣を振った向きに回して出す"""
	root, _ = sword5_slash("Nluu")
	save("_Sword5-1", root)


def make_sword5_2():
	"""05_02（投げた剣が回転する多段攻撃）のヒット：_Sword5 の斬撃を、回転しながら弧を描くようにする
	（向きはばらばら、1フレームに 24度ずつ回る。何度も当たるので、飛び散る線などは少なめにする）"""
	root, slash = sword5_slash("Nluu2")
	rot = slash.find("RotationValues")
	for tag in ("Type", "PVA"):
		e = rot.find(tag)
		if e is not None:
			rot.remove(e)
	child(rot, "Type").text = "1"
	pva = child(rot, "PVA")
	set_range(pva, ("Rotation", "Z"), 0)
	child(pva, "Rotation", "Z", "Max").text = "180"
	child(pva, "Rotation", "Z", "Min").text = "-180"
	set_range(pva, ("Velocity", "Z"), 24)
	scale_generation(root, 0.3, 4, threshold=5)
	save("_Sword5-2", root)


def adjust_sword1_4():
	"""攻撃01の4段目：長い白い線（SwordMein・SwordGlow）をまっすぐ1本に見せる
	元はカメラを向く板（回転ビルボード）で、Z の傾きだけを使う。この斬撃は Y にも 27.5度傾いているので、
	2枚の板が少し違う角度になり、真ん中で折れて見えていた → 板の向きを固定にして、2枚を同じ直線にそろえる"""
	root = load("_Sword1-4")
	for name in ("SwordMein", "SwordGlow"):
		child(find_node(root, name), "DrawingValues", "Sprite", "Billboard").text = "2"
	save("_Sword1-4", root)


ADJUSTS = {
	"boss_death": adjust_boss_death,
	"claw": adjust_claw,
	"sword5_1": make_sword5_1,
	"sword5_2": make_sword5_2,
	"sword1_4": adjust_sword1_4,
}


def main():
	# 引数で作るものを選べる（例：python Tools/AdjustExistingEffects.py claw）。無ければ _Sword5 の分だけ
	# ※_boss_death・_Claw2 は、もうエディタで調整して .efkefc に保存してあるので、
	#   流すとエディタでの調整が .efkproj から消える。作り直したい時だけ引数で指定する
	import sys
	names = sys.argv[1:] or ["sword5_1", "sword5_2"]
	for name in names:
		ADJUSTS[name]()


if __name__ == "__main__":
	main()
