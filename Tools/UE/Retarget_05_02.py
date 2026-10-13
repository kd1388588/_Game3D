# -*- coding: utf-8 -*-
# =====================================================================
# AS_Combo_Attack_05_02_Seq（剣を投げる攻撃）だけを SK_Assassin にリターゲットし直し、
# glTF に書き出す Unreal Engine 5.4 用 Python スクリプト
#
# なぜ必要か
#   投げた剣は weapon_r / weapon_l の骨の「位置」の動きで飛んでいる。
#   剣の骨がリターゲットのチェーンに入っていないと、元のアニメーションの
#   「手から見た位置」がそのまま使われ、手の向きの違いの分だけ飛ぶ方向がずれる。
#   そこで剣の骨を専用のチェーンにして、「世界での位置」を元のアニメーションに合わせる。
#
# 使い方
#   1. 「出力ログ」下の入力欄の左を「Cmd」→「Python」に切り替え、
#        py "C:/パス/Retarget_05_02.py"
#      のように実行する（パスは / 区切り）
#   2. 出力ログに、元のアニメーションとリターゲット後の
#      「剣が飛んだ方向」が表示されるので、同じ向きになっているか確認する
#
# ※ 作業前にプロジェクトのバックアップを取っておくこと
# =====================================================================

import os
import math
import unreal

# =====================================================================
# 設定（必要に応じて書き換える）
# =====================================================================

ANIM_NAME = "AS_Combo_Attack_05_02_Seq"

# リターゲット元のアニメーションを探すフォルダ（サブフォルダも探す）
SOURCE_ANIM_ROOT = "/Game/Dual_Sword/Animations/Sequence2"

# リターゲット後のアニメーションの保存先（元のフォルダ構成を ReTarget 以下に再現）
DUAL_SWORD_ROOT = "/Game/Dual_Sword"
RETARGET_ROOT = "/Game/ReTarget/Dual_Sword"

# IK Rig / IK Retargeter の保存先（既存のものがあればそれを使う）
IK_ASSET_FOLDER = "/Game/ReTarget/IK"

# リターゲット先のモデル
TARGET_MESH_NAME = "SK_Assassin"
TARGET_SEARCH_ROOT = "/Game/Scifi_Assassin"

# 剣の骨のチェーン（チェーン名, 始まりの骨, 終わりの骨）
# 元（Dual_Sword の SK_Mannequin）と先（SK_Assassin）で剣の骨の作りが違うので、別々に指定する
#   元：Weapon_R / Weapon_L（1本だけ。sword_r は無い）
#   先：weapon_r → sword_r / weapon_l → sword_l
SOURCE_WEAPON_CHAINS = [
	("Weapon_R", "Weapon_R", "Weapon_R"),
	("Weapon_L", "Weapon_L", "Weapon_L"),
]
TARGET_WEAPON_CHAINS = [
	("Weapon_R", "weapon_r", "weapon_r"),
	("Weapon_L", "weapon_l", "weapon_l"),
]
WEAPON_CHAIN_NAMES = ["Weapon_R", "Weapon_L"]

# 書き出し先（ゲームが読み込むフォルダと同じ構成）
EXPORT_FOLDER = r"C:\UE_gltf\Player\New\Sequence2\02_Attack\05_Combo_Attack_05\AS_Combo_Attack_05_02_Seq"

# =====================================================================

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
SRC = unreal.RetargetSourceOrTarget.SOURCE
TGT = unreal.RetargetSourceOrTarget.TARGET


def log(msg):
	unreal.log("[Retarget_05_02] " + msg)


def warn(msg):
	unreal.log_warning("[Retarget_05_02] " + msg)


def find_asset(root, name, cls):
	"""root 以下から、名前とクラスが一致するアセットを探す（名前で絞ってから読み込むので速い）"""
	for path in eal.list_assets(root, recursive=True, include_folder=False):
		if path.split("/")[-1].split(".")[0] != name:
			continue
		asset = eal.load_asset(path)
		if isinstance(asset, cls):
			return asset
	return None


def list_assets_of_class(root, cls):
	result = []
	for path in eal.list_assets(root, recursive=True, include_folder=False):
		asset = eal.load_asset(path)
		if isinstance(asset, cls):
			result.append(asset)
	return result


def package_folder(asset):
	return asset.get_path_name().rsplit("/", 1)[0]


def get_skeleton(asset):
	try:
		return asset.get_editor_property("skeleton")
	except Exception:
		return None


# ---------------------------------------------------------------------
# IK Rig / IK Retargeter
# ---------------------------------------------------------------------

def rig_mesh(rig):
	try:
		return unreal.IKRigController.get_controller(rig).get_skeletal_mesh()
	except Exception:
		return None


def find_retargeter(target_mesh):
	"""リターゲット先が SK_Assassin になっている、既存の IK Retargeter を探す"""
	for root in [IK_ASSET_FOLDER, "/Game/ReTarget", "/Game"]:
		if not eal.does_directory_exist(root):
			continue
		for rtg in list_assets_of_class(root, unreal.IKRetargeter):
			controller = unreal.IKRetargeterController.get_controller(rtg)
			target_rig = controller.get_ik_rig(TGT)
			if target_rig and rig_mesh(target_rig) == target_mesh:
				return rtg
	return None


def create_ik_rig(name, mesh):
	path = IK_ASSET_FOLDER + "/" + name
	if eal.does_asset_exist(path):
		return eal.load_asset(path)
	rig = asset_tools.create_asset(name, IK_ASSET_FOLDER, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
	controller = unreal.IKRigController.get_controller(rig)
	controller.set_skeletal_mesh(mesh)
	controller.apply_auto_generated_retarget_definition()
	eal.save_loaded_asset(rig)
	log("IK Rig を作成: " + path)
	return rig


def create_retargeter(source_mesh, target_mesh):
	eal.make_directory(IK_ASSET_FOLDER)
	source_rig = create_ik_rig("IK_DualSword_Source", source_mesh)
	target_rig = create_ik_rig("IK_" + TARGET_MESH_NAME, target_mesh)

	name = "RTG_DualSword_to_" + TARGET_MESH_NAME
	path = IK_ASSET_FOLDER + "/" + name
	if eal.does_asset_exist(path):
		return eal.load_asset(path)

	rtg = asset_tools.create_asset(name, IK_ASSET_FOLDER, unreal.IKRetargeter, unreal.IKRetargetFactory())
	controller = unreal.IKRetargeterController.get_controller(rtg)
	controller.set_ik_rig(SRC, source_rig)
	controller.set_ik_rig(TGT, target_rig)
	try:
		controller.set_preview_mesh(SRC, source_mesh)
		controller.set_preview_mesh(TGT, target_mesh)
	except Exception:
		pass
	controller.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
	try:
		controller.auto_align_all_bones(TGT)
	except Exception:
		pass
	eal.save_loaded_asset(rtg)
	log("IK Retargeter を作成: " + path)
	return rtg


def chain_names(rig):
	controller = unreal.IKRigController.get_controller(rig)
	names = []
	for chain in controller.get_retarget_chains():
		try:
			names.append(str(chain.get_editor_property("chain_name")))
		except Exception:
			names.append(str(chain.chain_name))
	return names


def print_bone_parents(rig, rig_label, bone_names):
	"""剣の骨の親が何の骨か表示する（元と先で親が違うと、飛ぶ向きがずれる原因になる）"""
	try:
		skeleton = unreal.IKRigController.get_controller(rig).get_ik_rig_skeleton()
		names = [str(n) for n in skeleton.get_editor_property("bone_names")]
		parents = list(skeleton.get_editor_property("parent_indices"))
	except Exception as e:
		warn("{}：骨の親子関係を取得できませんでした ({})".format(rig_label, e))
		return
	lower = [n.lower() for n in names]
	for bone in bone_names:
		if bone.lower() not in lower:
			log("{}：骨 {} は無い".format(rig_label, bone))
			continue
		chain = []
		index = lower.index(bone.lower())
		while index >= 0:
			chain.append(names[index])
			index = parents[index]
		log("{}：{}".format(rig_label, " ← ".join(chain)))


def ensure_weapon_chains(rig, rig_label, weapon_chains):
	"""IK Rig に剣の骨のチェーンが無ければ追加する"""
	controller = unreal.IKRigController.get_controller(rig)
	existing = chain_names(rig)
	for chain_name, start_bone, end_bone in weapon_chains:
		if chain_name in existing:
			# 前回のスクリプトで違う骨のチェーンが作られている場合があるので、作り直す
			try:
				controller.remove_retarget_chain(chain_name)
				log("{}：チェーン {} を作り直します".format(rig_label, chain_name))
			except Exception:
				log("{}：チェーン {} は作成済み".format(rig_label, chain_name))
				continue
		result = None
		try:
			result = controller.add_retarget_chain(chain_name, start_bone, end_bone, "")
		except Exception as e:
			warn("{}：チェーン {} を追加できませんでした ({})".format(rig_label, chain_name, e))
			continue
		if result is None or str(result) in ("", "None"):
			warn("{}：チェーン {} を追加できませんでした（骨 {} / {} が無い可能性）".format(rig_label, chain_name, start_bone, end_bone))
		else:
			log("{}：チェーン {} を追加（{} → {}）".format(rig_label, chain_name, start_bone, end_bone))
	eal.save_loaded_asset(rig)


def setup_weapon_chain_settings(rtg):
	"""剣のチェーンを対応づけ、「世界での位置」を元のアニメーションに合わせる設定にする"""
	controller = unreal.IKRetargeterController.get_controller(rtg)

	for chain_name in WEAPON_CHAIN_NAMES:
		# 元と先のチェーンを対応づける
		try:
			controller.set_source_chain(chain_name, chain_name)
		except Exception as e:
			warn("チェーン {} の対応づけに失敗 ({})".format(chain_name, e))
			continue

		# FK：位置は「世界での位置 × 体格比」、回転は元の動きを補間してそのまま使う
		try:
			settings = controller.get_retarget_chain_settings(chain_name)
			fk = settings.get_editor_property("fk")
			fk.set_editor_property("enable_fk", True)
			fk.set_editor_property("translation_mode", unreal.RetargetTranslationMode.GLOBALLY_SCALED)
			fk.set_editor_property("rotation_mode", unreal.RetargetRotationMode.INTERPOLATED)
			settings.set_editor_property("fk", fk)

			# 剣は手足ではないので IK は使わない
			ik = settings.get_editor_property("ik")
			ik.set_editor_property("enable_ik", False)
			settings.set_editor_property("ik", ik)

			try:
				controller.set_retarget_chain_settings(chain_name, settings)
			except TypeError:
				controller.set_retarget_chain_settings(settings, chain_name)
			log("チェーン {}：位置を GloballyScaled に設定".format(chain_name))
		except Exception as e:
			warn("チェーン {} の設定に失敗 ({})。IK Retargeter を開いて手動で設定してください".format(chain_name, e))

	eal.save_loaded_asset(rtg)


# ---------------------------------------------------------------------
# 剣が飛んだ方向の確認
# ---------------------------------------------------------------------

def weapon_flight_direction(anim, bone="weapon_r"):
	"""剣の骨が一番遠くまで飛んだ時の、腰から見た方向（度）と距離を返す"""
	try:
		frames = anim.get_editor_property("number_of_sampled_frames")
	except Exception:
		frames = 72
	options = unreal.AnimPoseEvaluationOptions()
	best = None
	for frame in range(0, max(1, frames)):
		try:
			pose = unreal.AnimPoseExtensions.get_anim_pose_at_frame(anim, frame, options)
			w = unreal.AnimPoseExtensions.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.WORLD).translation
			p = unreal.AnimPoseExtensions.get_bone_pose(pose, "pelvis", unreal.AnimPoseSpaces.WORLD).translation
		except Exception as e:
			warn("骨の位置を取得できませんでした ({})".format(e))
			return None
		dx, dy = w.x - p.x, w.y - p.y
		dist = math.hypot(dx, dy)
		if best is None or dist > best[1]:
			# UE のキャラは +Y が前。前を 0 度、右を + とする
			best = (math.degrees(math.atan2(dx, dy)), dist, frame)
	return best


def report_direction(label, anim):
	result = weapon_flight_direction(anim)
	if result:
		log("{}：剣が一番遠くまで飛んだ方向 = {:.1f} 度（前が0度） / 距離 {:.0f} cm / フレーム {}".format(
			label, result[0], result[1], result[2]))


# ---------------------------------------------------------------------
# 書き出し
# ---------------------------------------------------------------------

def export_animation(anim):
	os.makedirs(EXPORT_FOLDER, exist_ok=True)
	file_path = os.path.join(EXPORT_FOLDER, ANIM_NAME + ".gltf")

	options = unreal.GLTFExportOptions()
	# 今までの書き出しと同じく、モデル（メッシュ）は含めず骨とアニメーションだけにする
	try:
		options.set_editor_property("export_preview_mesh", False)
		options.set_editor_property("texture_image_format", unreal.GLTFTextureImageFormat.NONE)
	except Exception:
		pass

	try:
		result = unreal.GLTFExporter.export_to_gltf(anim, file_path, options, set())
	except TypeError:
		result = unreal.GLTFExporter.export_to_gltf(anim, file_path, options)
	ok = result[0] if isinstance(result, tuple) else bool(result)

	# フォルダには .gltf と .bin だけを残す
	for file_name in os.listdir(EXPORT_FOLDER):
		path = os.path.join(EXPORT_FOLDER, file_name)
		if os.path.isfile(path) and os.path.splitext(file_name)[1].lower() not in (".gltf", ".bin"):
			os.remove(path)

	if ok:
		log("書き出し: " + file_path)
	else:
		warn("書き出しに失敗: " + file_path)


# ---------------------------------------------------------------------
# メイン
# ---------------------------------------------------------------------

def main():
	source_anim = find_asset(SOURCE_ANIM_ROOT, ANIM_NAME, unreal.AnimSequence)
	if source_anim is None:
		warn("元のアニメーションが見つかりません: {} ({})".format(ANIM_NAME, SOURCE_ANIM_ROOT))
		return
	log("元のアニメーション: " + source_anim.get_path_name())

	target_mesh = find_asset(TARGET_SEARCH_ROOT, TARGET_MESH_NAME, unreal.SkeletalMesh)
	if target_mesh is None:
		warn("リターゲット先のモデルが見つかりません: " + TARGET_MESH_NAME)
		return

	# 元のアニメーションのモデル
	source_skeleton = get_skeleton(source_anim)
	source_mesh = None
	for root in [DUAL_SWORD_ROOT, "/Game"]:
		for mesh in list_assets_of_class(root, unreal.SkeletalMesh):
			if get_skeleton(mesh) == source_skeleton:
				source_mesh = mesh
				break
		if source_mesh:
			break
	if source_mesh is None:
		warn("元のアニメーションのモデルが見つかりません")
		return
	log("元のモデル: " + source_mesh.get_path_name())

	# IK Retargeter（既存のものがあれば使う）
	rtg = find_retargeter(target_mesh) or create_retargeter(source_mesh, target_mesh)
	log("IK Retargeter: " + rtg.get_path_name())
	controller = unreal.IKRetargeterController.get_controller(rtg)

	# 剣の骨の親子関係を表示（確認用）
	print_bone_parents(controller.get_ik_rig(SRC), "元の骨", ["Weapon_R", "Weapon_L", "Weapon_Holder_R", "Weapon_Holder_L"])
	print_bone_parents(controller.get_ik_rig(TGT), "先の骨", ["weapon_r", "weapon_l", "sword_r", "sword_l"])

	# 剣の骨のチェーンを追加・設定
	ensure_weapon_chains(controller.get_ik_rig(SRC), "元の IK Rig", SOURCE_WEAPON_CHAINS)
	ensure_weapon_chains(controller.get_ik_rig(TGT), "先の IK Rig", TARGET_WEAPON_CHAINS)
	setup_weapon_chain_settings(rtg)

	# 前回のリターゲット結果を削除してから、作り直す
	folder = package_folder(source_anim)
	relative = folder[len(DUAL_SWORD_ROOT):] if folder.startswith(DUAL_SWORD_ROOT) else ""
	dest_folder = RETARGET_ROOT + relative
	dest_path = dest_folder + "/" + ANIM_NAME
	if eal.does_asset_exist(dest_path):
		eal.delete_asset(dest_path)
		log("前回のリターゲット結果を削除: " + dest_path)

	new_assets = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
		[eal.find_asset_data(source_anim.get_path_name())], source_mesh, target_mesh, rtg,
		"", "", "", "_RTG_TMP", False)
	if not new_assets:
		warn("リターゲットに失敗しました")
		return

	new_asset = new_assets[0].get_asset()
	eal.make_directory(dest_folder)
	if not eal.rename_asset(new_asset.get_path_name(), dest_path):
		warn("移動に失敗: " + new_asset.get_path_name())
		return
	eal.save_asset(dest_path)
	retargeted = eal.load_asset(dest_path)
	log("リターゲット完了: " + dest_path)

	# 剣が飛んだ方向を比べる（同じ向きになっていれば成功）
	report_direction("元のアニメーション　", source_anim)
	report_direction("リターゲット後　　　", retargeted)

	export_animation(retargeted)
	log("完了しました")


main()
