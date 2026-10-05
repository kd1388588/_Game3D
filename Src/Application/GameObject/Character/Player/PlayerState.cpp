#include "PlayerState.h"
#include "Player.h"
#include "PlayerParamManager.h"

#include "../../Effect/Effect.h"
#include "../../../Scene/SceneManager.h"

void PlayerStateIdle::ChangeState(Player* player)
{
	if (player->IsCombatMode()) { player->ChangeAnimation("Combat_Idle", true); }
	else { player->ChangeAnimation("Idle", true); }
	player->SetAnimationSpeed(1.0f);
	player->SetUseRootMotion(false);
}

void PlayerStateIdle::Update(Player* player)
{
	if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
	{
		if (player->GetWeaponState() == WeaponState::Equipped)
		{
			player->ChangeState(std::make_shared<PlayerStateEvade>());
		}
		else
		{
			player->ChangeState(std::make_shared<PlayerStateDash>());
		}
		return;
	}

	if (player->CheckJumpInput() && player->GetGravity() >= 0.0f && player->GetGravity() <= 0.011f)
	{
		player->ChangeState(std::make_shared<PlayerStateJump>());
		return;
	}

	if (player->CheckEquipInput())
	{
		if (player->IsCombatMode()) {
			player->SetCombatMode(false);
			player->ChangeState(std::make_shared<PlayerStateUnequip>());
		}
		else {
			player->SetCombatMode(true);
			player->ChangeState(std::make_shared<PlayerStateEquip>());
		}
		return;
	}

	if (player->GetWeaponState() == WeaponState::Equipped && player->CheckAttackInput())
	{
		int type = player->GetCurrentAttackType();
		player->ChangeState(std::make_shared<PlayerStateComboAttack>(type, 1));
	}

	if (player->CheckMoveInput())
	{
		player->ChangeState(std::make_shared<PlayerStateRun>());
	}
}

void PlayerStateRun::ChangeState(Player* player)
{
	if (player->IsCombatMode()) { player->ChangeAnimation("Combat_Run", true); }
	else { player->ChangeAnimation("Run", true); }
	player->SetAnimationSpeed(1.0f);
	player->SetUseRootMotion(false);
}

void PlayerStateRun::Update(Player* player)
{
	if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
	{
		if (player->GetWeaponState() == WeaponState::Equipped)
		{
			player->ChangeState(std::make_shared<PlayerStateEvade>());
		}
		else
		{
			player->ChangeState(std::make_shared<PlayerStateDash>());
		}
		return;
	}

	if (player->CheckJumpInput() && player->GetGravity() >= 0.0f && player->GetGravity() <= 0.011f)
	{
		player->ChangeState(std::make_shared<PlayerStateJump>());
		return;
	}

	if (player->CheckEquipInput())
	{
		if (player->IsCombatMode()) {
			player->SetCombatMode(false);
			player->ChangeState(std::make_shared<PlayerStateUnequip>());
		}
		else {
			player->SetCombatMode(true);
			player->ChangeState(std::make_shared<PlayerStateEquip>());
		}
		return;
	}

	if (player->GetWeaponState() == WeaponState::Equipped && player->CheckAttackInput())
	{
		int type = player->GetCurrentAttackType();
		player->ChangeState(std::make_shared<PlayerStateComboAttack>(type, 1));
		return;
	}

	bool isMove = player->MoveProcess();
	if (!isMove)
	{
		player->ChangeState(std::make_shared<PlayerStateIdle>());
	}
}
void PlayerStateEquip::ChangeState(Player* player)
{
	player->ChangeAnimation("Equip", false);
	player->SetAnimationSpeed(1.3f);
}

void PlayerStateEquip::Update(Player* player)
{
	if (player->GetAnimTime() >= player->GetEquipFrame()) {
		player->SetWeaponState(WeaponState::Equipped);
	}

	if (player->IsAnimEnd()) {
		player->ChangeState(std::make_shared<PlayerStateIdle>());
	}
}

void PlayerStateUnequip::ChangeState(Player* player)
{
	player->ChangeAnimation("Unequip", false);
	player->SetAnimationSpeed(1.0f);

}

void PlayerStateUnequip::Update(Player* player)
{
	if (player->GetAnimTime() >= player->GetUnequipFrame()) {
		player->SetWeaponState(WeaponState::Sheathed);
	}

	if (player->IsAnimEnd()) {
		player->ChangeState(std::make_shared<PlayerStateIdle>());
	}
}


float PlayerStateComboAttack::GetCancelFrame() const
{
	float cancel = 20.0f; // 基本は20フレーム目以降ならクリックで次へ行ける

	// 02シリーズ（空中の繋がり）
	if (m_comboType == 2 && m_comboStep == 4) cancel = 30.0f; // ★落ちる前（空中）のフレームを指定！
	if (m_comboType == 2 && m_comboStep == 5) cancel = 999.0f; // 5段目は最後まで再生

	// 03シリーズ
	if (m_comboType == 3 && m_comboStep == 3) cancel = 30.0f;

	// 04シリーズ
	if (m_comboType == 4 && m_comboStep == 3) {
		if (m_subStep == 1) return 999.0f; // Startはキャンセル不可
		if (m_subStep == 2) return 0.0f;   // LoopはいつでもEndへ行ける
		if (m_subStep == 3) return 30.0f;  // Endは落ちる前に4_4へ！
	}

	// 05シリーズ
	if (m_comboType == 5 && m_comboStep == 2) cancel = 25.0f;
	if (m_comboType == 5 && m_comboStep == 3) cancel = 30.0f;

	return cancel;
}

std::string PlayerStateComboAttack::GetAnimName() const
{
	if (m_comboType == 4 && m_comboStep == 3) {
		if (m_subStep == 1) return "Attack_4_3_Start";
		if (m_subStep == 2) return "Attack_4_3_Loop";
		if (m_subStep == 3) return "Attack_4_3_End";
	}
	return "Attack_" + std::to_string(m_comboType) + "_" + std::to_string(m_comboStep);
}

void PlayerStateComboAttack::ChangeState(Player* player)
{
	m_effect = std::make_shared<KdEffekseerObject>();
	m_isCritical = (KdRandom::GetInt(0, 99) < player->GetCritRate());

	if (m_comboType == 4 && m_comboStep == 3 && m_subStep == 0) {
		m_subStep = 1;
	}

	std::string animName = GetAnimName();
	bool isLoop = (m_comboType == 4 && m_comboStep == 3 && m_subStep == 2);

	player->ChangeAnimation(animName, isLoop, true, 5.0f);

	player->SetAnimationSpeed(1.0f);
	player->SetWeaponState(WeaponState::Equipped);
	player->SetUseRootMotion(true);
	// マネージャーの変数を倍率としてセット
	player->SetRootMotionScale(PlayerParamManager::Instance().m_attackRootScale);

	if ((m_comboType == 2 && m_comboStep == 4) ||
		(m_comboType == 2 && m_comboStep == 5) ||
		(m_comboType == 4 && m_comboStep == 3) || 
		(m_comboType == 5 && m_comboStep == 2) ||
		(m_comboType == 5 && m_comboStep == 3))
	{
		player->SetUseRootMotion(false); // XZの座標移動を完全にロック！
	}
}

void PlayerStateComboAttack::Update(Player* player)
{
	player->ExecJump(0.0f);

	// マウスクリックされたら次の攻撃を「予約」する
	if (player->CheckAttackInput())
	{
		m_nextAttackReserved = true;
	}
	
	// 攻撃タイプ04のLoopの攻撃時間調整
	if (m_comboType == 4 && m_comboStep == 3 && m_subStep == 2)
	{
		m_loopTimer++; // 毎フレーム時間を進める

		if (m_loopTimer >= 90)
		{
			player->ChangeState(std::make_shared<PlayerStateComboAttack>
				(4, 3, 3, m_nextAttackReserved));
			return;
		}
	}

	float animTime = player->GetAnimTime();


	if (animTime >= 10.0f && animTime <= 45.0f)
	{
		if (!m_isHit)
		{
			// 戻り値（当たったかどうか）を受け取る
			bool hitR = player->AttackOBB(player->GetSwordMatrixR(), m_isCritical);
			bool hitL = player->AttackOBB(player->GetSwordMatrixL(), m_isCritical);

			// 右手か左手どちらかが敵に当たったら
			if (hitR || hitL)
			{
				player->SetHitStopTimer(1);		// ここで1回だけストップをかける！
				//m_isHit = true;				// 「当てた」と記録する（もう判定しない）
			}

			if (player->GetSwordTrail())
			{
				// 毎フレーム、剣の行列（根本から剣先までの情報）を軌跡に渡してポイントを追加する
				player->GetSwordTrail()->AddPoint(player->GetSwordMatrixR());
			}
		}
		else
		{
			// 攻撃時間が終わった（または振る前）なら、軌跡の線をリセットする
			// これをやらないと、次の攻撃を振った時に空間を跨いで線が繋がってしまいます
			if (player->GetSwordTrail())
			{
				// ※関数名が Clear() や DelPoints() の場合もあります
				player->GetSwordTrail()->ClearPoints();
			}
		}

		/*
		if (!m_isEffect)
		{
			Math::Vector3 effectPos = player->GetPos() + Math::Vector3(0.0f, 1.0f, 1.0f);
			std::shared_ptr<Effect> effect = std::make_shared<Effect>();
			effect->Init();

			// 今何段目の攻撃かによって、出すエフェクトファイルを変える！
			if (m_comboStep == 1)      effect->SetEffect("_Sword1-1.efkefc", effectPos, 1.0f);
			else if (m_comboStep == 2) effect->SetEffect("_Sword1-2.efkefc", effectPos, 1.0f);
			else if (m_comboStep == 3) effect->SetEffect("_Sword1-3.efkefc", effectPos, 1.0f);
			else if (m_comboStep == 4) effect->SetEffect("_Sword1-4.efkefc", effectPos, 1.0f);

			SceneManager::Instance().AddObject(effect);
			m_isEffect = true;
		}
		*/
	}

	// 指定したフレームを超えたら、空中にいる状態から次へ滑らかに移行
	float cancelFrame = GetCancelFrame();

	if (m_nextAttackReserved && animTime >= cancelFrame && !player->IsAnimEnd())
	{
		if (m_comboType == 4 && m_comboStep == 3 && m_subStep == 2) {
			player->ChangeState(std::make_shared<PlayerStateComboAttack>(4, 3, 3, true));
			return;
		}
		else if (m_comboStep < 5) {
			player->ChangeState(std::make_shared<PlayerStateComboAttack>(m_comboType, m_comboStep + 1));
			return;
		}
	}

	if (player->IsAnimEnd())
	{
		// 4_3_Start が最後まで終わったら、自動で Loop(subStep=2) に繋ぐ
		if (m_comboType == 4 && m_comboStep == 3 && m_subStep == 1) {
			player->ChangeState(std::make_shared<PlayerStateComboAttack>(4, 3, 2));
			return;
		}
		// 4_3_End が終わった時、予約があれば4_4へ、なければIdleへ
		if (m_comboType == 4 && m_comboStep == 3 && m_subStep == 3) {
			if (m_nextAttackReserved) player->ChangeState(std::make_shared<PlayerStateComboAttack>(4, 4));
			else player->ChangeState(std::make_shared<PlayerStateIdle>());
			return;
		}

		// その他の攻撃で、予約があれば次の段へ
		if (m_nextAttackReserved && m_comboStep < 5)
		{
			player->ChangeState(std::make_shared<PlayerStateComboAttack>(m_comboType, m_comboStep + 1));
		}
		else
		{
			// 予約がない、または5段目まで打ち終わった
			player->ChangeState(std::make_shared<PlayerStateIdle>());
		}
	}
}

void PlayerStateEvade::ChangeState(Player* player)
{
	player->ChangeAnimation("Evade", false);
	//player->SetInvincibleTimer(10);
	player->SetAnimationSpeed(1.55f);
}

void PlayerStateEvade::Update(Player* player)
{
	float animTime = player->GetAnimTime();

	if (animTime < 25.0f)
	{
		float speed = PlayerParamManager::Instance().m_evadeSpeed * (1.0f - (animTime / 25.0f));

		Math::Vector3 pos = player->GetPos();
		Math::Vector3 forward = player->GetMatrix().Backward() * -1.0f;

		pos += forward * speed;
		player->SetPos(pos);
		player->BumpHit();
	}

	if (player->IsAnimEnd()) {
		player->ChangeState(std::make_shared<PlayerStateIdle>());
	}
}

void PlayerStateDash::ChangeState(Player* player)
{
	player->ChangeAnimation("Dash", false);
	player->SetAnimationSpeed(1.0f);

	player->SetUseRootMotion(false);
}

void PlayerStateDash::Update(Player* player)
{
	float animTime = player->GetAnimTime();

	if (animTime < 25.0f)
	{
		float speed = PlayerParamManager::Instance().m_dashSpeed * (1.0f - (animTime / 25.0f));

		Math::Vector3 pos = player->GetPos();
		Math::Vector3 forward = player->GetMatrix().Backward() * -1.0f;
		forward.y = 0.0f;
		forward.Normalize();

		pos += forward * speed;
		player->SetPos(pos);
		player->BumpHit();
	}
	if (player->IsAnimEnd()) {
		player->ChangeState(std::make_shared<PlayerStateIdle>());
	}
}

void PlayerStateJump::ChangeState(Player* player)
{
	m_jumpPhase = 1;
	m_jumpCount = 1;
	m_isFalling = false;

	player->ExecJump(0.25f);
	player->ChangeAnimation("JumpStart", false, true);
	player->SetAnimationSpeed(1.0f);
	player->SetUseRootMotion(false);
}

void PlayerStateJump::Update(Player* player)
{
	if (m_jumpPhase != 2)
	{
		player->MoveProcess();
	}

	// --------------------------------------------------
	if (m_jumpPhase == 1) // 【空中】
	{
		if (player->IsAnimEnd() || player->GetGravity() > 0.011f)
		{
			if (player->GetAnimTime() > 0.0f)
			{
				player->ChangeAnimation("JumpLoop", true);
			}
		}

		//// 2段ジャンプの入力検知
		//if (m_jumpCount < 2 && player->CheckJumpInput())
		//{
		//	m_jumpCount++;
		//	player->ExecJump(0.2f); 
		//	player->ChangeAnimation("JumpStart", false, true); 
		//	m_isFalling = false;
		//}

		if (player->GetGravity() > 0.011f)
		{
			m_isFalling = true;
		}

		if (m_isFalling && player->GetGravity() <= 0.011f)
		{
			player->ChangeAnimation("JumpEnd", false, true);
			m_jumpPhase = 2;
		}
	}

	// --------------------------------------------------
	else if (m_jumpPhase == 2) // 【着地硬直】
	{
		if (player->CheckMoveInput())
		{
			player->ChangeState(std::make_shared<PlayerStateRun>());
			return;
		}

		if (player->IsAnimEnd())
		{
			player->ChangeState(std::make_shared<PlayerStateIdle>());
		}
	}
}

void PlayerStateDamage::ChangeState(Player* player)
{
	// 仰け反るようなダメージモーションを再生
	player->ChangeAnimation("Damage", false, true);
	player->SetAnimationSpeed(1.0f);

}

void PlayerStateDamage::Update(Player* player)
{

	float animTime = player->GetAnimTime();
	if (animTime < 15.0f)
	{
		Math::Vector3 pos = player->GetPos();

		Math::Vector3 backward = player->GetMatrix().Backward();

		backward.y = 0.0f;
		backward.Normalize();

		pos += backward * 0.06f;
		player->SetPos(pos);
	}

	if (player->IsAnimEnd())
	{
		player->ChangeState(std::make_shared<PlayerStateIdle>());
	}
}

void PlayerStateDead::ChangeState(Player* player)
{
	player->ChangeAnimation("Death", false, true);
	player->SetAnimationSpeed(1.0f);

}

void PlayerStateDead::Update(Player* player)
{
	//if (player->IsAnimEnd())
	//{
	//	player->Expire();
	//}
	if (player->IsAnimEnd())
	{
		if (GetAsyncKeyState('Q') & 0x8000)
		{
			player->Revive();
			player->ChangeState(std::make_shared<PlayerStateIdle>());
		}
	}
}