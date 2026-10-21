# 叩きつけのエフェクト（_GroundSmash.efkproj）を、Effekseer のサンプルエフェクトを組み合わせて作るスクリプト
# 使い方：python Tools/MakeGroundSmashEffect.py <MAGICALxSPIRAL のフォルダ>
#   例）python Tools/MakeGroundSmashEffect.py C:/Users/xxx/Downloads/_Effekseer/MAGICALxSPIRAL
#   → Asset/Data/Effect/_GroundSmash.efkproj と、使うテクスチャ（Texture/）・モデル（Model/）をコピーする
#   → Effekseer エディタで開いて、同じ名前の .efkefc に保存する（ゲームが読むのは .efkefc）
#
# 組み合わせ
#   ・Gohlem1（ゴーレムの叩きつけ）：岩のかけら（立体のモデル）が飛び散り、光の粒と衝撃波の輪が広がる
#     （元は 10 フレーム目に叩きつけるので、0 フレーム目から始まるように前へずらす）
#   ・foot_smoke1：地面を走る衝撃波（地面に寝かせた輪）
#   ・地面のひび（Tools/MakeImpactTexture.py の GroundCrack.png）：しばらく残って消える
#   全体を Scale のノードの下にまとめ、大きさ 1 で 半径 約2m に広がるように小さくしている

import os
import re
import shutil
import sys
import xml.etree.ElementTree as ET

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
EFFECT_DIR = os.path.join(ROOT, "Asset", "Data", "Effect")

SCALE = 0.4		# サンプルの大きさ（半径 約5）→ 半径 約2 にする


def load_nodes(path):
	return ET.parse(path).getroot().find("Root/Children").findall("Node")


def sub(parent, tag, text=None):
	e = ET.SubElement(parent, tag)
	if text is not None:
		e.text = str(text)
	return e


def value_xyz(parent, tag, x, y, z):
	e = sub(parent, tag)
	sub(e, "X", x)
	sub(e, "Y", y)
	sub(e, "Z", z)
	return e


def set_offset_zero(node):
	"""出し始めるまでの時間を 0 にする"""
	off = node.find("CommonValues/GenerationTimeOffset")
	if off is None:
		return
	if len(off) == 0:
		off.text = "0"
	for e in off:
		e.text = "0"


def make_scale_node(children):
	"""描かないノード（全体の大きさを変えるだけ）"""
	n = ET.Element("Node")
	common = sub(n, "CommonValues")
	sub(sub(common, "MaxGeneration"), "Value", 1)
	life = sub(common, "Life")
	for k in ("Center", "Max", "Min"):
		sub(life, k, 120)
	scale = sub(n, "ScalingValues")
	sub(scale, "Type", 0)
	value_xyz(sub(scale, "Fixed"), "Scale", SCALE, SCALE, SCALE)
	sub(sub(n, "DrawingValues"), "Type", 0)
	sub(n, "IsRendered", "False")
	sub(n, "Name", "Scale")
	ch = sub(n, "Children")
	for c in children:
		ch.append(c)
	return n


def make_crack_node():
	"""地面のひび（地面に寝かせた板。半透明でしばらく残る）"""
	xml = """<Node>
<CommonValues><MaxGeneration><Value>1</Value></MaxGeneration><Life><Center>70</Center><Max>70</Max><Min>70</Min></Life></CommonValues>
<LocationValues><Type>0</Type><Fixed><Location><X>0</X><Y>0.05</Y><Z>0</Z></Location></Fixed></LocationValues>
<RotationValues><Type>0</Type><Fixed><Rotation><X>90</X><Y>0</Y><Z>0</Z></Rotation></Fixed></RotationValues>
<ScalingValues><Type>4</Type><SingleEasing><Start><Center>4</Center><Max>4</Max><Min>4</Min></Start><End><Center>4.5</Center><Max>4.5</Max><Min>4.5</Min></End><StartSpeed>40</StartSpeed><EndSpeed>-40</EndSpeed></SingleEasing></ScalingValues>
<RendererCommonValues><ColorTexture>Tex/GroundCrack.png</ColorTexture><AlphaBlend>1</AlphaBlend><FadeInType>1</FadeInType><FadeIn><Frame>1</Frame></FadeIn><FadeOutType>1</FadeOutType><FadeOut><Frame>30</Frame></FadeOut></RendererCommonValues>
<DrawingValues><Type>2</Type><ColorAll><Type>0</Type><Fixed><R>255</R><G>255</G><B>255</B><A>235</A></Fixed></ColorAll><Sprite><Billboard>2</Billboard></Sprite></DrawingValues>
<Name>Crack</Name><Children></Children>
</Node>"""
	return ET.fromstring(xml)


def main():
	if len(sys.argv) < 2:
		print("使い方：python Tools/MakeGroundSmashEffect.py <MAGICALxSPIRAL のフォルダ>")
		return
	src = sys.argv[1]

	# ゴーレムの叩きつけ（10 フレーム目の叩きつけを 0 フレーム目にする）
	# 岩のかけら（モデル）も、さらに 10 フレーム遅れて出るので、叩きつけた瞬間に出す
	golem = load_nodes(os.path.join(src, "Gohlem1.efkproj"))
	targets = list(golem)
	for n in golem:
		for c in n.iter("Node"):
			if c.findtext("DrawingValues/Type") == "5":
				targets.append(c)
	for n in targets:
		set_offset_zero(n)
	# 描かないノードに残っている、まちがったテクスチャ名（.efkproj）を消す（エディタで見つからないと言われないように）
	for n in golem:
		for tex in n.iter("ColorTexture"):
			if (tex.text or "").endswith(".efkproj"):
				tex.text = ""
	# 地面を走る衝撃波
	smoke = load_nodes(os.path.join(src, "foot_smoke1.efkproj"))

	root_node = ET.Element("Root")
	sub(root_node, "Name", "Root")
	children = sub(root_node, "Children")
	children.append(make_crack_node())
	children.append(make_scale_node(golem + smoke))

	project = ET.Element("EffekseerProject")
	project.append(root_node)
	sub(project, "ToolVersion", "1.43d")
	sub(project, "Version", 3)
	sub(project, "StartFrame", 0)
	sub(project, "EndFrame", 75)
	sub(project, "IsLoop", "False")

	out = os.path.join(EFFECT_DIR, "_GroundSmash.efkproj")
	ET.ElementTree(project).write(out, encoding="utf-8", xml_declaration=True)
	print("作成:", os.path.normpath(out))

	# 使っているテクスチャ・モデルをコピー（.efkproj から見た同じ場所に置く）
	text = open(out, encoding="utf-8").read()
	for folder in ("Texture", "Model"):
		os.makedirs(os.path.join(EFFECT_DIR, folder), exist_ok=True)
	for rel in sorted(set(re.findall(r">((?:Texture|Model)/[^<]+\.(?:png|efkmodel))<", text))):
		shutil.copyfile(os.path.join(src, rel), os.path.join(EFFECT_DIR, rel))
		print("コピー:", rel)


if __name__ == "__main__":
	main()
