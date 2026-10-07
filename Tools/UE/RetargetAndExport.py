# -*- coding: utf-8 -*-
# =====================================================================
# Dual_Sword の Sequence2 のアニメーションを SK_Assassin にリターゲットし、
# glTF に書き出す Unreal Engine 5.4 用 Python スクリプト
#
# 使い方（詳しくは Docs/UE_Retarget_Guide.txt）
#   1. 「編集」→「プラグイン」で次の2つを有効にして UE を再起動
#        ・Python Editor Script Plugin
#        ・glTF Exporter
#   2. 「出力ログ」下の入力欄の左を「Cmd」→「Python」に切り替え、
#        py "C:/パス/RetargetAndExport.py"
#      のように実行する（ファイルのパスは / 区切りで）
#
# ※ 作業前にプロジェクトのバックアップを取っておくこと
# =====================================================================

import os
import unreal

# =====================================================================
# 設定（必要に応じて書き換える）
# =====================================================================

# リターゲット元のアニメーションがあるフォルダ（中のサブフォルダも全て対象）
SOURCE_ANIM_ROOT = "/Game/Dual_Sword/Animations/Sequence2"

# 元のフォルダ構成の基準（このフォルダ以下の構成を RETARGET_ROOT にそのまま再現する）
DUAL_SWORD_ROOT = "/Game/Dual_Sword"

# リターゲット後のアニメーションの保存先
#   例：/Game/Dual_Sword/Animations/Sequence2/01_Idle/AS_xxx
#     → /Game/ReTarget/Dual_Sword/Animations/Sequence2/01_Idle/AS_xxx
RETARGET_ROOT = "/Game/ReTarget/Dual_Sword"

# IK Rig / IK Retargeter の保存先
IK_ASSET_FOLDER = "/Game/ReTarget/IK"

# リターゲット先のモデルと、その検索フォルダ
TARGET_MESH_NAME = "SK_Assassin"
TARGET_SEARCH_ROOT = "/Game/Scifi_Assassin"

# 武器（モデルと一緒に CharaBase に書き出す）
WEAPON_MESH_NAMES = ["SM_Blade", "SM_Scabbard"]

# リターゲット元のモデル（空なら、アニメーションのスケルトンを使っているモデルを自動で探す）
#   自動で見つからない場合は "/Game/Characters/Mannequins/Meshes/SKM_Manny" のように指定する
SOURCE_MESH_PATH = ""

# glTF の書き出し先
EXPORT_ROOT = r"C:\UE_gltf\Player\New"
CHARA_BASE_FOLDER = "CharaBase"

# 実行する処理
DO_RETARGET = True		# リターゲットを行う
DO_EXPORT = True		# glTF に書き出す（リターゲット済みのアニメーションを書き出す）

# =====================================================================

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary


def log(msg):
	unreal.log("[Retarget] " + msg)


def warn(msg):
	unreal.log_warning("[Retarget] " + msg)


def list_assets_of_class(root, cls):
	"""root 以下（サブフォルダ含む）にある、指定クラスのアセットを返す"""
	result = []
	for path in eal.list_assets(root, recursive=True, include_folder=False):
		asset = eal.load_asset(path)
		if isinstance(asset, cls):
			result.append(asset)
	return result


def find_asset_by_name(root, name, cls):
	for asset in list_assets_of_class(root, cls):
		if asset.get_name() == name:
			return asset
	return None


def package_folder(asset):
	"""/Game/A/B/AssetName → /Game/A/B"""
	return asset.get_path_name().rsplit("/", 1)[0]


def get_skeleton(asset):
	try:
		return asset.get_editor_property("skeleton")
	except Exception:
		return None


def find_source_mesh(skeleton):
	"""アニメーションと同じスケルトンを使っているスケルタルメッシュを探す"""
	if SOURCE_MESH_PATH:
		return eal.load_asset(SOURCE_MESH_PATH)

	for root in [DUAL_SWORD_ROOT, "/Game"]:
		log("リターゲット元のモデルを検索中: " + root)
		for mesh in list_assets_of_class(root, unreal.SkeletalMesh):
			if get_skeleton(mesh) == skeleton:
				return mesh
	return None


# ---------------------------------------------------------------------
# IK Rig / IK Retargeter の作成
# ---------------------------------------------------------------------

def set_property_safe(obj, name, value):
	try:
		obj.set_editor_property(name, value)
		return True
	except Exception as e:
		warn("設定できませんでした: {} = {} ({})".format(name, value, e))
		return False


def create_ik_rig(name, mesh):
	path = IK_ASSET_FOLDER + "/" + name
	if eal.does_asset_exist(path):
		log("既存の IK Rig を使用: " + path)
		return eal.load_asset(path)

	rig = asset_tools.create_asset(name, IK_ASSET_FOLDER, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
	controller = unreal.IKRigController.get_controller(rig)
	controller.set_skeletal_mesh(mesh)

	# Manny/Quinn 系の骨は、リターゲット用のチェーンを自動で作れる
	controller.apply_auto_generated_retarget_definition()
	try:
		controller.apply_auto_fbik()
	except Exception:
		pass

	eal.save_loaded_asset(rig)
	log("IK Rig を作成: " + path)
	return rig


def create_retargeter(name, source_rig, target_rig, source_mesh, target_mesh):
	path = IK_ASSET_FOLDER + "/" + name
	if eal.does_asset_exist(path):
		log("既存の IK Retargeter を使用: " + path)
		return eal.load_asset(path)

	rtg = asset_tools.create_asset(name, IK_ASSET_FOLDER, unreal.IKRetargeter, unreal.IKRetargetFactory())
	controller = unreal.IKRetargeterController.get_controller(rtg)

	SRC = unreal.RetargetSourceOrTarget.SOURCE
	TGT = unreal.RetargetSourceOrTarget.TARGET
	controller.set_ik_rig(SRC, source_rig)
	controller.set_ik_rig(TGT, target_rig)
	try:
		controller.set_preview_mesh(SRC, source_mesh)
		controller.set_preview_mesh(TGT, target_mesh)
	except Exception:
		pass

	# 骨のチェーン（腕・脚・背骨など）を名前で自動対応
	controller.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)

	# リターゲット先の基本姿勢を元に合わせる（A ポーズと T ポーズの違いなどを吸収）
	try:
		controller.auto_align_all_bones(TGT)
	except Exception:
		warn("基本姿勢の自動合わせができませんでした。必要なら IK Retargeter を開いて手動で合わせてください")

	eal.save_loaded_asset(rtg)
	log("IK Retargeter を作成: " + path)
	return rtg


# ---------------------------------------------------------------------
# リターゲット
# ---------------------------------------------------------------------

def retarget_destination_folder(source_anim):
	"""元のアニメーションのフォルダ構成を RETARGET_ROOT 以下に再現したフォルダ"""
	folder = package_folder(source_anim)
	relative = folder[len(DUAL_SWORD_ROOT):] if folder.startswith(DUAL_SWORD_ROOT) else "/" + folder.split("/", 2)[-1]
	return RETARGET_ROOT + relative


def retarget_animations(source_anims, source_mesh, target_mesh, retargeter):
	TEMP_SUFFIX = "_RTG_TMP"
	result_paths = []

	for anim in source_anims:
		name = anim.get_name()
		dest_folder = retarget_destination_folder(anim)
		dest_path = dest_folder + "/" + name

		if eal.does_asset_exist(dest_path):
			log("リターゲット済みのためスキップ: " + dest_path)
			result_paths.append(dest_path)
			continue

		asset_data = eal.find_asset_data(anim.get_path_name())

		# 元と同じフォルダに「名前_RTG_TMP」で複製・リターゲットされる
		new_assets = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
			[asset_data], source_mesh, target_mesh, retargeter,
			"", "", "", TEMP_SUFFIX, False)

		if not new_assets:
			warn("リターゲットに失敗: " + anim.get_path_name())
			continue

		# 新しいアセットを ReTarget フォルダへ移動し、名前を元に戻す
		new_asset = new_assets[0].get_asset()
		eal.make_directory(dest_folder)
		if eal.rename_asset(new_asset.get_path_name(), dest_path):
			eal.save_asset(dest_path)
			result_paths.append(dest_path)
			log("リターゲット完了: " + dest_path)
		else:
			warn("移動に失敗: " + new_asset.get_path_name() + " → " + dest_path)

	return result_paths


# ---------------------------------------------------------------------
# glTF 書き出し
# ---------------------------------------------------------------------

def make_export_options(with_textures):
	options = unreal.GLTFExportOptions()
	# アニメーションのファイルにもモデル（骨の構成）を含める
	# （ゲーム側はモデルとアニメーションの骨の並びが一致している前提で読み込むため）
	set_property_safe(options, "export_preview_mesh", True)
	if not with_textures:
		# アニメーション用：テクスチャ画像を書き出さない
		set_property_safe(options, "texture_image_format", unreal.GLTFTextureImageFormat.NONE)
	return options


def export_gltf(asset, file_path, with_textures):
	os.makedirs(os.path.dirname(file_path), exist_ok=True)
	options = make_export_options(with_textures)
	try:
		result = unreal.GLTFExporter.export_to_gltf(asset, file_path, options, set())
	except TypeError:
		result = unreal.GLTFExporter.export_to_gltf(asset, file_path, options)

	ok = result[0] if isinstance(result, tuple) else bool(result)
	if ok:
		log("書き出し: " + file_path)
	else:
		warn("書き出しに失敗: " + file_path)
	return ok


def keep_only_gltf_and_bin(folder):
	"""アニメーションのフォルダには .gltf と .bin だけを残す"""
	for file_name in os.listdir(folder):
		path = os.path.join(folder, file_name)
		if os.path.isfile(path) and os.path.splitext(file_name)[1].lower() not in (".gltf", ".bin"):
			os.remove(path)


def export_animation(anim_path):
	anim = eal.load_asset(anim_path)
	name = anim.get_name()

	# /Game/ReTarget/Dual_Sword/Animations/Sequence2/01_Idle/AS_xxx
	#   → C:\UE_gltf\Player\New\Sequence2\01_Idle\AS_xxx\AS_xxx.gltf
	relative_folder = package_folder(anim)[len(RETARGET_ROOT + "/Animations/"):]
	folder = os.path.join(EXPORT_ROOT, *relative_folder.split("/"), name)
	export_gltf(anim, os.path.join(folder, name + ".gltf"), with_textures=False)
	keep_only_gltf_and_bin(folder)


def export_chara_base(target_mesh):
	"""モデル本体と武器を、マテリアル・テクスチャ付きで CharaBase に書き出す"""
	folder = os.path.join(EXPORT_ROOT, CHARA_BASE_FOLDER)
	export_gltf(target_mesh, os.path.join(folder, target_mesh.get_name() + ".gltf"), with_textures=True)

	for weapon_name in WEAPON_MESH_NAMES:
		weapon = find_asset_by_name(TARGET_SEARCH_ROOT, weapon_name, unreal.StaticMesh)
		if weapon is None:
			weapon = find_asset_by_name("/Game", weapon_name, unreal.StaticMesh)
		if weapon is None:
			warn("武器が見つかりません: " + weapon_name)
			continue
		export_gltf(weapon, os.path.join(folder, weapon_name + ".gltf"), with_textures=True)


# ---------------------------------------------------------------------
# メイン
# ---------------------------------------------------------------------

def main():
	target_mesh = find_asset_by_name(TARGET_SEARCH_ROOT, TARGET_MESH_NAME, unreal.SkeletalMesh)
	if target_mesh is None:
		warn("リターゲット先のモデルが見つかりません: {} ({})".format(TARGET_MESH_NAME, TARGET_SEARCH_ROOT))
		return
	log("リターゲット先: " + target_mesh.get_path_name())

	source_anims = list_assets_of_class(SOURCE_ANIM_ROOT, unreal.AnimSequence)
	log("リターゲット元のアニメーション: {} 個".format(len(source_anims)))
	if not source_anims:
		return

	retargeted_paths = []

	if DO_RETARGET:
		source_skeleton = get_skeleton(source_anims[0])
		source_mesh = find_source_mesh(source_skeleton)
		if source_mesh is None:
			warn("リターゲット元のモデルが見つかりません。SOURCE_MESH_PATH を設定してください")
			return
		log("リターゲット元: " + source_mesh.get_path_name())

		eal.make_directory(IK_ASSET_FOLDER)
		source_rig = create_ik_rig("IK_DualSword_Source", source_mesh)
		target_rig = create_ik_rig("IK_" + TARGET_MESH_NAME, target_mesh)
		retargeter = create_retargeter("RTG_DualSword_to_" + TARGET_MESH_NAME,
			source_rig, target_rig, source_mesh, target_mesh)

		with unreal.ScopedSlowTask(len(source_anims), "リターゲット中...") as task:
			task.make_dialog(True)
			retargeted_paths = retarget_animations(source_anims, source_mesh, target_mesh, retargeter)
	else:
		retargeted_paths = [a.get_path_name().split(".")[0]
			for a in list_assets_of_class(RETARGET_ROOT, unreal.AnimSequence)]

	if DO_EXPORT:
		export_chara_base(target_mesh)
		with unreal.ScopedSlowTask(len(retargeted_paths), "glTF 書き出し中...") as task:
			task.make_dialog(True)
			for path in retargeted_paths:
				if task.should_cancel():
					break
				task.enter_progress_frame(1, path)
				export_animation(path)

	log("完了しました")


main()
