#pragma once

// ============================================================
// 使用するエフェクトのファイル名（Asset/Data/Effect/ からの相対パス）
// 空文字 "" にしたものは再生されない（ファイルが用意できたら書き換える）
// ============================================================
namespace EffectName
{
	// プレイヤーのコンボ攻撃（斬撃）
	inline const std::string ComboSlash1_1		= "_Sword1-1.efkefc";	// 攻撃タイプ1の1段目
	inline const std::string ComboSlash1_2		= "_Sword1-2.efkefc";	// 攻撃タイプ1の2段目
	inline const std::string ComboSlash1_3		= "_Sword1-3.efkefc";	// 攻撃タイプ1の3段目
	inline const std::string ComboSlash1_4		= "_Sword1-4.efkefc";	// 攻撃タイプ1の4段目
	inline const std::string ComboSlash4		= "_Sword4.efkefc";		// 攻撃タイプ4
	inline const std::string ComboSlash5_4		= "_Sword5-4.efkefc";	// 攻撃タイプ5の4段目
	inline const std::string ComboSlashDefault	= "_Sword1.efkefc";		// 専用のエフェクトが無い攻撃
	inline const std::string ChargeDrill		= "_drill.efkefc";		// 4-3（溜め攻撃）のループ中

	// ヒット
	inline const std::string HitEnemy			= "_Sword1.efkefc";		// プレイヤーの攻撃が敵に当たった
	inline const std::string HitPlayer			= "_Claw2.efkefc";		// 敵の攻撃がプレイヤーに当たった

	// 撃破
	inline const std::string EnemyDeath			= "_boss_death.efkefc";	// 敵が倒れた

	// プレイヤーの動作（ファイルが用意できたら設定する）
	inline const std::string AwakeningAura		= "";					// 覚醒中のオーラ（覚醒中は繰り返し再生）
	inline const std::string Evade				= "";					// 回避
	inline const std::string Landing			= "";					// ジャンプの着地
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

	// 再生中のエフェクトを止めて、ポインタを空にする
	static void Stop(std::shared_ptr<KdEffekseerObject>& effect);

	// 全てのエフェクトを止める（シーン切り替え時など）
	static void StopAll();

private:

	// エフェクトファイルが存在するか（無い場合は1度だけログを出す）
	static bool Exists(const std::string& fileName);
};
