// PlayerAttack.cpp
// 攻撃の当たり判定とヒットエフェクト
// ※Player クラスの関数を、役割ごとにファイルを分けて書いている（クラスの宣言は Player.h）

// Player
#include "Player.h"
#include "PlayerState.h"
#include "PlayerParamManager.h"

// Effect
#include "../../../../Framework/Direct3D/Polygon/KdTrailPolygon.h"

// UI
#include "../UI/Gage/HP/HPGage.h"
#include "../UI/Gage/ATB/ATBGage.h"
#include "../UI/Gage/ATB/ATBCommandMenu.h"

// Enemy
#include "../Enemy/Enemy.h"

// Camera
#include "../../Camera/TPSCamera/TPSCamera.h"

// Effect
#include "../../Effect/EffectPlayer.h"

// Utility
#include "../../../Utility/HitStop.h"
#include "../../../Utility/SoundPlayer.h"
#include "../../../Utility/GameTime.h"
#include "../../../Utility/StageSettings.h"

// Scene
#include "../../../Scene/SceneManager.h"
#include "../../../Scene/BaseScene/BaseScene.h"

#include "PlayerConstants.h"

using InputHelper::IsKeyDown;
using namespace PlayerConst;

float Player::CalcScreenSlashAngle(const Math::Matrix& swordMatrix, int hand) const
{
	// 剣を振った向き：今の剣先と、軌跡に残っている前のフレームの剣先の差
	Math::Vector3 slashDir = Math::Vector3::Zero;
	const SwordTrail& trail = (hand == 1) ? m_swordTrailL : m_swordTrailR;
	Math::Vector3 tip = GetSwordPoint(swordMatrix, m_swordBladeTipY);
	for (const Math::Vector3& prevTip : trail.tipHistory)
	{
		// 同じ位置（まだ動いていない）の記録は飛ばす
		if ((tip - prevTip).LengthSquared() > 0.0001f)
		{
			slashDir = tip - prevTip;
			break;
		}
	}

	// 画面の右・上の向きに分けて角度にする（分からない時は縦）
	Math::Matrix camRot = KdShaderManager::Instance().GetCameraCB().mView.Invert();
	float dx = slashDir.Dot(camRot.Right());
	float dy = slashDir.Dot(camRot.Up());
	if (dx * dx + dy * dy <= 0.000001f) return DirectX::XM_PIDIV2;
	return std::atan2(dy, dx);
}

void Player::PlayHitEffect(HitEffectType hitEffectType, const Math::Matrix& swordMatrix, int hand, const Math::Vector3& hitPos, bool isCritical) const
{
	switch (hitEffectType)
	{
	case HitEffectType::Slash:
	{
		// _Sword1-1 の斬撃は、カメラを向いた縦長の板を Z で傾けたもの（エフェクトの中で kHitSlashBaseAngle 度傾いている）
		// 板の縦（長い方）が剣を振った向きになるように、足りない分だけ Z で回して出す
		float rotZ = CalcScreenSlashAngle(swordMatrix, hand) - DirectX::XMConvertToRadians(90.0f + kHitSlashBaseAngle);
		float scale = isCritical ? kCriticalHitEffectScale : kHitEffectScale;
		Math::Matrix mWorld = Math::Matrix::CreateScale(scale)
			* Math::Matrix::CreateRotationZ(rotZ)
			* Math::Matrix::CreateTranslation(hitPos);
		EffectPlayer::Play(EffectName::HitEnemy, mWorld);
		break;
	}

	case HitEffectType::Sword5Slash:
	{
		// _Sword5 の斬撃はエフェクトの中で +Y（上）に進むので、90度引いた分だけ回すと、剣を振った向きに進む
		// （カメラの方を向けて出すので、エフェクトの XY 平面がそのまま画面になる）
		float angle = CalcScreenSlashAngle(swordMatrix, hand) - DirectX::XM_PIDIV2;
		float scale = kSword5SlashScale * (isCritical ? kCriticalHitEffectScale / kHitEffectScale : 1.0f);
		if (EffectPlayer::PlayFacingCameraRotZ(EffectName::ComboSlash5_1, hitPos, scale, angle)) break;

		// _Sword5-1.efkefc がまだ無い時は、いつもの斬撃を出す
		PlayHitEffect(HitEffectType::Slash, swordMatrix, hand, hitPos, isCritical);
		break;
	}

	case HitEffectType::Sword5Spin:
		// 回転する斬撃（向きはエフェクトの中でばらばらにしてある）
		if (EffectPlayer::PlayFacingCamera(EffectName::ComboSlash5_2, hitPos, kSword5SpinScale)) break;

		// _Sword5-2.efkefc がまだ無い時は、いつもの斬撃を出す
		PlayHitEffect(HitEffectType::Slash, swordMatrix, hand, hitPos, isCritical);
		break;

	default:
		break;
	}
}

void Player::PlayFinisherEffect(const std::string& effectName, const Math::Vector3& hitPos, bool isCritical)
{
	// 1振りに1回だけ（二刀流で2回当たっても、多段ヒットしても、決めのエフェクトは1つ）
	if (effectName.empty() || m_isFinisherEffectPlayed) return;
	m_isFinisherEffectPlayed = true;

	// プレイヤーの攻撃の向きに合わせ、当たった位置に出す
	float scale = (isCritical ? kCriticalHitEffectScale : kHitEffectScale) * kFinisherEffectScaleRate;
	Math::Matrix mWorld = Math::Matrix::CreateScale(scale)
		* Math::Matrix::CreateRotationY(GetAngleRad())
		* Math::Matrix::CreateTranslation(hitPos);
	EffectPlayer::Play(effectName, mWorld);
}

bool Player::AttackOBB(const Math::Matrix& swordMatrix, bool isCritical, HitEffectType hitEffectType, const std::string& finisherEffectName, float damageRate, int hand)
{
	// 刃の根元から先端までを囲む、Y軸方向に長い箱
	float obbOffsetY = (m_swordBladeBaseY + m_swordBladeTipY) * 0.5f;
	Math::Vector3 obbExtents = { kSwordOBBHalfWidth, (m_swordBladeTipY - m_swordBladeBaseY) * 0.5f, kSwordOBBHalfWidth };
	Math::Matrix obbMatrix = Math::Matrix::CreateTranslation(0.0f, obbOffsetY, 0.0f) * swordMatrix;
	Math::Vector3 offset = Math::Vector3::Zero;

	// 剣が速く動いた時は、前のフレームの剣との間にも箱を置く（回転攻撃で、箱と箱の間をすり抜けないように）
	// 前のフレームの剣の行列と今の行列の間を、回転は球面補間・位置は直線補間で分ける
	int handIndex = std::clamp(hand, 0, 1);
	std::vector<Math::Matrix> obbMatrices = { obbMatrix };
	if (m_prevAttackFrame[handIndex] == m_attackFrame - 1)
	{
		const Math::Matrix& prevMat = m_prevAttackSwordMat[handIndex];
		Math::Vector3 prevTip = GetSwordPoint(prevMat, m_swordBladeTipY);
		Math::Vector3 tip = GetSwordPoint(swordMatrix, m_swordBladeTipY);
		int steps = std::min(static_cast<int>((tip - prevTip).Length() / kSwordSweepStep), kSwordSweepMaxStep);

		Math::Vector3 s0, s1, t0, t1;
		Math::Quaternion q0, q1;
		Math::Matrix m0 = prevMat, m1 = swordMatrix;
		if (steps > 0 && m0.Decompose(s0, q0, t0) && m1.Decompose(s1, q1, t1))
		{
			for (int i = 1; i <= steps; ++i)
			{
				float t = static_cast<float>(i) / (steps + 1);
				Math::Matrix mid = Math::Matrix::CreateScale(Math::Vector3::Lerp(s0, s1, t))
					* Math::Matrix::CreateFromQuaternion(Math::Quaternion::Slerp(q0, q1, t))
					* Math::Matrix::CreateTranslation(Math::Vector3::Lerp(t0, t1, t));
				obbMatrices.push_back(Math::Matrix::CreateTranslation(0.0f, obbOffsetY, 0.0f) * mid);
			}
		}
	}
	m_prevAttackSwordMat[handIndex] = swordMatrix;
	m_prevAttackFrame[handIndex] = m_attackFrame;

	std::vector<KdCollider::BoxInfo> obbInfos;
	for (const Math::Matrix& mat : obbMatrices)
	{
		obbInfos.emplace_back(KdCollider::TypeDamage, mat, offset, obbExtents, true);

		// デバッグ用の赤い箱を描画
		if (m_pDebugWire)
		{
			m_pDebugWire->AddDebugBox(mat, obbExtents, offset, true, { 1.0f, 0.0f, 0.0f, 1.0f });
		}
	}

	bool isHit = false;
	// ダメージ（モードで倍率を変える。ATBの技はモードに関係なく、技の倍率だけを使う）
	float modeRate = (m_battleMode == BattleMode::Punisher) ? PlayerTune().punisherDamageRate : PlayerTune().operatorDamageRate;
	if (m_isAbilityAttack) modeRate = 1.0f;
	int baseDamage = isCritical ? PlayerTune().criticalAttackDamage : PlayerTune().attackDamage;
	int damage = static_cast<int>(std::round(baseDamage * modeRate * damageRate));

	std::list<KdCollider::CollisionResult> retList;
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		if (obj.get() == this) continue;
		bool isTouched = false;
		for (const KdCollider::BoxInfo& info : obbInfos)
		{
			if (obj->Intersects(info, &retList)) { isTouched = true; break; }
		}
		if (!isTouched) continue;

		// 敵以外・無敵中・倒れている敵には何もしない
		// （毎フレームヒット扱いになって、ヒットストップやエフェクトが連続しないように）
		auto enemy = std::dynamic_pointer_cast<Enemy>(obj);
		if (!enemy || enemy->IsInvincible() || !enemy->IsAlive()) continue;

		// 同じ剣が同じ敵に当たってから、少しの間はもう当たらない（剣が触れている毎フレーム当たらないように）
		// 右と左の剣は別々に数えるので、二刀流で1振りに2回当たる
		auto& hitRecord = m_lastHitFrame[std::clamp(hand, 0, 1)];
		if (auto it = hitRecord.find(enemy.get()); it != hitRecord.end() && m_attackFrame - it->second < PlayerTune().multiHitInterval) continue;
		hitRecord[enemy.get()] = m_attackFrame;

		// ATBコマンドの技を受けている敵は、のけぞり続ける（その間は攻撃してこない）
		if (m_isAbilityAttack) enemy->AddStun(PlayerTune().abilityStunFrame);

		enemy->OnDamage(damage, isCritical);
		isHit = true;

		// ブレイクゲージ（普通の攻撃では少しずつ。数値はオブジェクトエディタの「Enemy」タブ）
		// ※回避の直後・パリィの攻撃で多くためる（bonusGain）のは、あとで足す
		{
			const StageSettings::Battle& battle = StageSettings::Instance().GetBattle();
			float breakGain = battle.hitGain;
			if (isCritical)									breakGain *= battle.criticalRate;
			if (m_isAbilityAttack)							breakGain *= battle.abilityRate;
			else if (m_battleMode == BattleMode::Punisher)	breakGain *= battle.punisherRate;
			enemy->AddBreak(breakGain);
		}

		// 攻撃を当てるとATBゲージがたまる（オペレーターは多め、パニッシャーは少なめ。アビリティの攻撃ではたまらない）
		if (!m_isAbilityAttack)
		{
			float gain = PlayerTune().atbHitGain * ((m_battleMode == BattleMode::Operator) ? PlayerTune().operatorAtbHitRate : PlayerTune().punisherAtbHitRate);
			if (isCritical) gain *= PlayerTune().atbCriticalRate;
			AddAtbGage(gain);
		}


		// 当たった位置にヒットエフェクト
		// （敵は当たると少しの間無敵になるので、同じ敵に毎フレーム出続けることはない）
		Math::Vector3 hitPos = retList.empty() ? obbMatrix.Translation() : retList.back().m_hitPos;
		PlayHitEffect(hitEffectType, swordMatrix, hand, hitPos, isCritical);
		PlayFinisherEffect(finisherEffectName, hitPos, isCritical);
	}

	// ヒットストップ（プレイヤーのすべての攻撃に、同じルールでかける）
	// 普通 < クリティカル。パニッシャー・アビリティの重い攻撃は少し長く止める
	if (isHit)
	{
		int frames = isCritical ? kCriticalHitStopFrame : kHitStopFrame;
		if (m_battleMode == BattleMode::Punisher || m_isAbilityAttack) frames += kHeavyHitStopBonus;
		HitStop::Request(frames, isCritical ? kCriticalHitShakePower : kHitShakePower);

		// 斬った音（同じフレームに何体に当たっても1回だけ。重い攻撃・クリティカルは低く大きく）
		bool isHeavy = (m_battleMode == BattleMode::Punisher || m_isAbilityAttack);
		float pitch = isCritical ? kSlashSECriticalPitch : (isHeavy ? kSlashSEHeavyPitch : 0.0f);
		SoundPlayer::PlaySE(SoundName::Slash, (isCritical || isHeavy) ? 1.0f : kSlashSEVolume, pitch, kSlashSEPitchRandom);
	}

	return isHit;
}
