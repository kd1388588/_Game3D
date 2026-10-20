#pragma once

// ============================================================
// 使用するエフェクトのファイル名（Asset/Data/Effect/ からの相対パス）
// 空文字 "" にしたもの・ファイルが無いものは再生されない（ファイルが用意できたら書き換える）
// ModeChange / AbilityStart / Guard / GuardBreak は Tools/MakeEffekseerProjects.py で作った .efkproj を
// Effekseer エディタで .efkefc に保存して使う
// _Claw2 / _boss_death は Tools/AdjustExistingEffects.py で数を減らした・短くした .efkproj を作れる
// ============================================================
namespace EffectName
{
	// プレイヤーのコンボ攻撃（斬撃）
	inline const std::string ComboSlash1_1		= "_Sword1-1.efkefc";	// 攻撃タイプ1の1段目
	inline const std::string ComboSlash1_2		= "_Sword1-2.efkefc";	// 攻撃タイプ1の2段目
	inline const std::string ComboSlash1_3		= "_Sword1-3.efkefc";	// 攻撃タイプ1の3段目
	inline const std::string ComboSlash1_4		= "_Sword1-4.efkefc";	// 攻撃タイプ1の4段目
	inline const std::string ComboSlash4		= "_Sword4.efkefc";		// 攻撃タイプ4
	inline const std::string ComboSlash5_1		= "_Sword5-1.efkefc";	// 攻撃タイプ5の1段目（斬撃1本。向きはゲーム側で回す）
	inline const std::string ComboSlash5_2		= "_Sword5-2.efkefc";	// 攻撃タイプ5の2段目（同上）
	inline const std::string ComboSlash5_3		= "_Sword5-3.efkefc";	// 攻撃タイプ5の3段目（同上）
	inline const std::string ComboSlash5_4		= "_Sword5-4.efkefc";	// 攻撃タイプ5の4段目
	inline const std::string ComboSlashDefault	= "_Sword1.efkefc";		// 専用のエフェクトが無い攻撃

	// ヒット
	inline const std::string HitEnemy			= "_Sword1-1.efkefc";	// プレイヤーの攻撃が敵に当たった（斬撃1本。剣を振った向きに回して出す）
	inline const std::string HitPlayer			= "_Claw2.efkefc";		// 敵の攻撃がプレイヤーに当たった

	// 撃破
	inline const std::string EnemyDeath			= "_boss_death.efkefc";	// 敵が倒れた
	inline const std::string BossGroundSmash	= "";					// ボスの飛びかかり攻撃の着地

	// プレイヤーの動作（ファイルが用意できたら設定する）
	inline const std::string Evade				= "";					// 回避
	inline const std::string Landing			= "";					// ジャンプの着地
	inline const std::string ModeChange			= "ModeChange.efkefc";		// バトルモードの切り替え（Eキー）（白く作ってあり、モードの色をかける）
	inline const std::string AbilityStart		= "AbilityStart.efkefc";	// アビリティ（ATBコマンドで選んだ技）の発動
	inline const std::string Guard				= "Guard.efkefc";		// 敵の攻撃をガードした
	inline const std::string GuardBreak			= "GuardBreak.efkefc";	// ガードを崩された
}

// ============================================================
// Effekseerのエフェクトを再生する補助クラス
// ============================================================
class EffectPlayer
{
public:

	// 指定した位置でエフェクトを1回再生する
	// （ファイル名が空・ファイルが無い場合は何もせず nullptr を返す）
	static std::shared_ptr<KdEffekseerObject> Play(const std::string& fileName, const Math::Vector3& pos,
		float scale = 1.0f, float speed = 1.0f);

	// ワールド行列（位置・向き・大きさ）を指定してエフェクトを1回再生する
	static std::shared_ptr<KdEffekseerObject> Play(const std::string& fileName, const Math::Matrix& mWorld,
		float speed = 1.0f);

	// カメラの方を向けて、エフェクトを1回再生する（板が XY 平面のエフェクト用）
	// screenDir：エフェクトの X 軸を、画面の上でこの向き（ワールドの向き）に合わせる。ゼロなら回さない
	static std::shared_ptr<KdEffekseerObject> PlayFacingCamera(const std::string& fileName, const Math::Vector3& pos,
		float scale = 1.0f, const Math::Vector3& screenDir = Math::Vector3::Zero);

	// 再生中のエフェクトを止めて、ポインタを空にする
	static void Stop(std::shared_ptr<KdEffekseerObject>& effect);

	// 全てのエフェクトを止める（シーン切り替え時など）
	static void StopAll();

	// エフェクトを出すかどうか（false の間は Play しても何も出ない。タイトル画面で使う）
	static void SetEnabled(bool isEnabled) { s_isEnabled = isEnabled; }

private:

	static inline bool s_isEnabled = true;

	// エフェクトファイルが存在するか（無い場合は1度だけログを出す）
	static bool Exists(const std::string& fileName);
};
