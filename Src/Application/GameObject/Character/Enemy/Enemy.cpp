#include "Enemy.h"
#include "../../../Utility/HitStop.h"
#include "../../../Utility/SoundPlayer.h"
#include "../../../Utility/GameTime.h"
#include "../Player/Player.h"
#include "../../../Scene/SceneManager.h"
#include "EnemyState.h"
#include "../../Effect/EffectPlayer.h"

namespace
{
	// 敵の種類ごとのパラメータ
	struct EnemyParam
	{
		int		hp;
		float	scale;
		float	searchRange;	// プレイヤーに気づく距離
		float	attackRange;	// 攻撃を開始する距離
		int		jumpAttackCT;	// 飛びかかり攻撃を使った後、次に使えるまでのフレーム数
	};

	constexpr EnemyParam kBossParam		= { 350, 2.6f,  16.0f, 3.5f, 60 * 6 };	// ボス：ザコの2倍くらい大きく、遠くから気づき、リーチが長い
	constexpr EnemyParam kNormalParam	= { 100,  1.25f, 8.0f,  1.2f, 60 * 10 };	// ザコ敵（飛びかかり攻撃はたまに）

	// 最初の飛びかかり攻撃までの時間（全員が同時に飛びかかってこないよう、ばらつかせる）
	constexpr int	kFirstJumpAttackMinFrame	= 60 * 1;
	constexpr int	kFirstJumpAttackMaxFrame	= 60 * 4;

	// 攻撃の構えの間に、プレイヤーの方へ向きを変える速さ（ラジアン/フレーム）
	constexpr float	kAttackTurnSpeed		= 0.12f;

	// 被弾後の無敵時間（0：無し）
	// ※プレイヤーの攻撃は、剣ごとに「同じ敵には少し間をあけてしか当たらない」ようにしているので、
	//   敵の側では無敵にしない（無敵があると、二刀流の2本目や多段ヒットが当たらなくなる）
	constexpr int	kDamageInvincibleFrame	= 0;
	constexpr int	kAttackDamage			= 5;
	constexpr int	kCriticalAttackDamage	= 10;
	constexpr float	kPushStrength			= 0.1f;		// 敵同士の押し出しの強さ（毎フレームの割合）
	constexpr float	kHitEffectScale			= 0.22f;	// プレイヤーに攻撃が当たった時のエフェクトの大きさ（大きいと前が見えなくなる）
	const Math::Color kHitEffectColor		= { 1.0f, 0.3f, 0.25f, 1.0f };	// その色（エフェクトの元の色にかけ算される。白なら元の色のまま）

	// プレイヤーに攻撃が当たった時のエフェクト（小さめ・赤っぽい色にして、プレイヤーの攻撃のエフェクトと見分けやすくする）
	void PlayHitPlayerEffect(const Math::Vector3& pos)
	{
		std::shared_ptr<KdEffekseerObject> effect = EffectPlayer::Play(EffectName::HitPlayer, pos, kHitEffectScale);
		if (effect) effect->SetColor(kHitEffectColor);
	}

	const std::string kModelDir = "Asset/Models/GameObject/Enemy/Monster2/";

	// 飛びかかり攻撃（ボスだけ）
	const std::string kJumpAttackStartAnim	= "JumpAttackStart";
	constexpr float	kLeapJumpPower			= 0.3f;		// 飛び上がる強さ（上がるのに30フレーム、高さ約4.5）
	constexpr float	kGravityPerFrame		= 0.01f;	// 重力（BaseChara と同じ値）
	constexpr float	kLeapFallGravityRate	= 1.6f;		// 落ちる時の重力の倍率（勢いよく落ちて叩きつける）
	constexpr float	kMaxLeapDistance		= 14.0f;	// 飛べる距離の上限
	constexpr float	kLeapLandOffset			= 1.5f;		// プレイヤーの少し手前に着地する（真上に落ちてめり込まないように）
	constexpr float	kFallingGravity			= 0.03f;	// これを超えたら「落ち始めた」とみなす
	constexpr float	kOnGroundGravity		= 0.011f;	// これ以下なら地面にいるとみなす

	// 着地の衝撃
	constexpr float	kSmashRadius			= 1.8f;		// 攻撃の範囲（体の大きさに掛ける）
	constexpr float	kSmashHeight			= 2.0f;		// 上下にこれだけ離れていたら当たらない（ジャンプでかわせる）
	constexpr int	kSmashDamage			= 20;		// のけぞるダメージ
	constexpr int	kSmashHitStopFrame		= 4;		// 着地の時のヒットストップ（当たらなくても、地面が揺れる感じを出す）
	constexpr float	kSmashShakePower		= 0.2f;
	constexpr float	kSmashEffectScaleRate	= 1.0f;		// 着地のエフェクトの大きさ（体の大きさに掛ける。攻撃の範囲と同じくらいに広がる）

	// 普通の攻撃で、手を地面に叩きつけた時の爪のエフェクト（体の大きさに掛ける）
	constexpr float	kSlamEffectScaleRate	= 0.12f;

	// 足元の近くへの攻撃判定（体の大きさに掛ける）
	constexpr float	kNearHitRadius			= 0.9f;		// 体の中心からこの距離以内（ボスなら約2.3m、ザコなら約1.1m）
	constexpr float	kNearHitHeight			= 2.0f;		// 上下にこれ以上離れていたら当たらない
	constexpr float	kNearHitBackDot			= -0.3f;	// 体の向きとの内積がこれより小さい（真後ろ）なら当たらない
}

void Enemy::Init()
{
	m_model = std::make_shared<KdModelWork>();
	m_pCollider = std::make_unique<KdCollider>();
	m_pDebugWire = std::make_unique<KdDebugWireFrame>();

	// モデル・アニメーション（現在はボスもザコも同じものを使用）
	m_model->SetModelData(kModelDir + "Base/Rampage.gltf");
	LoadAnimations
	({
		{ "Idle",	kModelDir + "Animation/Idle/Idle/Idle.gltf" },
		{ "Run",	kModelDir + "Animation/Move/Sprint_Biped_Fwd/Sprint_Biped_Fwd.gltf" },
		{ "Attack",	kModelDir + "Animation/Attack/Attack_Melee_A/Attack_Melee_A.gltf" },
		{ "Damage",	kModelDir + "Animation/Hit/Hit/HitReact_Front/HitReact_Front.gltf" },
		{ "Dead",	kModelDir + "Animation/Hit/Death/Death_A.gltf" },

		// 飛びかかり攻撃（ファイルが無い時は読み込まれず、この攻撃は使われない）
		{ "JumpAttackStart",	kModelDir + "Animation/Special/Ability_GroundSmash_Start/Ability_GroundSmash_Start.gltf" },
		{ "JumpAttackLoop",		kModelDir + "Animation/Special/Ability_GroundSmash_Loop/Ability_GroundSmash_Loop.gltf" },
		{ "JumpAttackEnd",		kModelDir + "Animation/Special/Ability_GroundSmash_End/Ability_GroundSmash_End.gltf" },
	});

	// 種類ごとのパラメータ
	const EnemyParam& param = m_isBoss ? kBossParam : kNormalParam;
	SetHp(param.hp);
	m_jumpAttackCoolTime = param.jumpAttackCT;
	m_jumpAttackCT = KdRandom::GetInt(kFirstJumpAttackMinFrame, kFirstJumpAttackMaxFrame);

	// 飛びかかり攻撃のアニメーションが読み込めたか（読み込めていないと、この攻撃は使われない）
	if (!HasAnimation(kJumpAttackStartAnim))
	{
		OutputDebugStringA("【飛びかかり攻撃のアニメーションが読み込めていません】Monster2/Animation/Special/Ability_GroundSmash_Start を確認してください\n");
		KdDebugGUI::Instance().AddLog("[Enemy] 飛びかかり攻撃のアニメーションが読み込めていません\n");
	}
	m_scale = param.scale;
	m_searchRange = param.searchRange;
	m_attackRange = param.attackRange;

	// 初期ステートをIdleに設定
	ChangeState(std::make_shared<EnemyStateIdle>());

	m_pCollider->RegisterCollisionShape
	(
		"Enemy",
		{ 0.0f, 0.5f, 0.0f },
		0.6f,
		KdCollider::Type::TypeDamage
	);

	m_pos = { 0.0f, 0.0f, 5.0f };
	UpdateWorldMatrix();
}

void Enemy::PostUpdate()
{
	BaseChara::PostUpdate();
}

void Enemy::Update()
{
	// ヒットストップ中は、アニメーションも移動も止める（プレイヤーと一緒に止まる）
	if (HitStop::IsStopping())
	{
		UpdateWorldMatrix();
		return;
	}

	BaseChara::Update();

	// スローモーション中（プレイヤーがコマンドを選んでいる間）は、アニメーションをゆっくり動かすだけにする
	// （攻撃する・近づくなどの考える処理は止める）
	if (GameTime::IsSlow())
	{
		ApplyRootMotion(m_rootScale, m_rotY);
		UpdateWorldMatrix();
		return;
	}

	UpdateInvincibleTimer();

	if (m_jumpAttackCT > 0) m_jumpAttackCT--;
	if (m_attackCT > 0) m_attackCT--;
	if (m_stunTimer > 0) m_stunTimer--;

	if (m_state)
	{
		m_state->Update(this);
	}

	ApplyRootMotion(m_rootScale, m_rotY);

	PushAwayFromOtherEnemies();

	UpdateWorldMatrix();

	if (m_pDebugWire)
	{
		Math::Vector3 hitPos = m_pos + Math::Vector3(0.0f, m_scale, 0.0f);
		m_pDebugWire->AddDebugSphere(hitPos, m_scale, { 0.0f, 1.0f, 0.0f, 1.0f });
	}
}

void Enemy::GenerateDepthMapFromLight()
{
	// 影：DrawLit と同じモデルを、光から見た深度に描く
	if (m_model)
	{
		KdShaderManager::Instance().m_StandardShader.DrawModel(*m_model, m_mWorld);
	}
}

void Enemy::DrawLit()
{
	if (m_model)
	{
		KdShaderManager::Instance().m_StandardShader.DrawModel(*m_model, m_mWorld);
	}
}

void Enemy::UpdateWorldMatrix()
{
	Math::Matrix scale = Math::Matrix::CreateScale(m_scale);
	Math::Matrix rot = Math::Matrix::CreateRotationY(m_rotY);
	Math::Matrix trans = Math::Matrix::CreateTranslation(m_pos);

	m_mWorld = scale * rot * trans;
}

void Enemy::PushAwayFromOtherEnemies()
{
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		// 相手がEnemyで、かつ自分自身ではない場合
		auto otherEnemy = std::dynamic_pointer_cast<Enemy>(obj);
		if (!otherEnemy || otherEnemy.get() == this) continue;

		Math::Vector3 pushVec = m_pos - otherEnemy->GetPos();
		pushVec.y = 0.0f;

		float dist = pushVec.Length();
		float pushRadius = m_scale + otherEnemy->m_scale;

		if (dist > 0.001f && dist < pushRadius)
		{
			pushVec.Normalize();
			// 毎フレームジワジワと押し出す（攻撃中も待機中も反発しあう）
			m_pos += pushVec * (pushRadius - dist) * kPushStrength;
		}
	}
}

void Enemy::ChangeState(const std::shared_ptr<EnemyState>& newState)
{
	// のけぞらない状態・落ちる時の重力の倍率は、その攻撃の間だけ（次のステートが必要なら付け直す）
	m_isSuperArmor = false;
	SetFallGravityRate(1.0f);

	m_state = newState;
	if (m_state)
	{
		m_state->ChangeState(this);
	}
}

void Enemy::AttackHit(const Math::Matrix& hitMatrix, const Math::Vector3& extents, bool isCritical)
{
	// 行列（hitMatrix）で既に位置をズラしているので、ローカルオフセットはZeroにする
	Math::Vector3 offset = Math::Vector3::Zero;

	KdCollider::BoxInfo box(KdCollider::TypeDamage, hitMatrix, offset, extents, true);

	if (m_pDebugWire)
	{
		m_pDebugWire->AddDebugBox(hitMatrix, extents, offset, true, { 1.0f, 0.0f, 0.0f, 1.0f });
	}

	int damage = isCritical ? kCriticalAttackDamage : kAttackDamage;

	std::list<KdCollider::CollisionResult> retList;
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		// 自分自身には当てない
		if (obj.get() == this) continue;
		if (!obj->Intersects(box, &retList)) continue;

		// プレイヤーだった場合、ダメージを与える（無敵中・倒れている時は何もしない）
		auto player = std::dynamic_pointer_cast<Player>(obj);
		if (!player || player->IsInvincible() || !player->IsAlive()) continue;

		player->OnDamage(damage, isCritical);

		// 当たった位置にヒットエフェクト
		Math::Vector3 hitPos = retList.empty() ? player->GetPos() : retList.back().m_hitPos;
		PlayHitPlayerEffect(hitPos);
	}
}

Math::Matrix Enemy::GetRightArmMatrix() const
{
	if (m_model)
	{
		// ※モデルの骨の向きの都合で "lowerarm_l" を使用している
		if (const KdModelWork::Node* pNode = m_model->FindNode("lowerarm_l"))
		{
			return pNode->m_worldTransform * m_mWorld;
		}
	}
	return m_mWorld;
}

void Enemy::Release()
{
	m_model = nullptr;
}

void Enemy::OnDamage(int damage, bool isCritical)
{
	if (m_invincibleTimer > 0) return;
	if (m_hp <= 0) return;

	m_hp -= damage;
	m_invincibleTimer = kDamageInvincibleFrame;

	if (m_hp <= 0)
	{
		ChangeState(std::make_shared<EnemyStateDead>());
		return;
	}

	KdDebugGUI::Instance().AddLog
	(
		isCritical ? "[HIT] 敵にクリティカル！ ダメージ: %d\n" : "[HIT] 敵に通常ダメージ！ ダメージ: %d\n",
		damage
	);

	// 攻撃のアニメーション中（普通の攻撃・飛びかかり攻撃）は、のけぞらない（ダメージだけ受ける）
	// （ただし、プレイヤーのATBコマンドの技を受けている時は、のけぞる）
	if (m_isSuperArmor && m_stunTimer <= 0) return;

	ChangeState(std::make_shared<EnemyStateDamage>());
}

void Enemy::TurnToPlayer()
{
	auto target = m_wpTarget.lock();
	if (!target) return;

	// プレイヤーへの方向ベクトルを計算（上下方向は無視する）
	Math::Vector3 dir = target->GetPos() - m_pos;
	dir.y = 0.0f;

	if (dir.LengthSquared() > 0.0f)
	{
		dir.Normalize();
		// モデルが逆向きなので180度回す
		m_rotY = atan2(dir.x, dir.z) + DirectX::XM_PI;
	}
}

bool Enemy::CanJumpAttack() const
{
	return m_jumpAttackCT <= 0 && HasAnimation(kJumpAttackStartAnim);
}

void Enemy::StartJumpAttackCooldown()
{
	m_jumpAttackCT = m_jumpAttackCoolTime;
}

void Enemy::TurnToPlayerSmooth(float maxRadPerFrame)
{
	auto target = m_wpTarget.lock();
	if (!target) return;

	Math::Vector3 dir = target->GetPos() - m_pos;
	dir.y = 0.0f;
	if (dir.LengthSquared() <= 0.0f) return;

	// 向きたい角度との差（-π ～ π）を、1フレームに maxRadPerFrame までだけ回す
	float targetRot = std::atan2(dir.x, dir.z) + DirectX::XM_PI;
	float diff = std::remainder(targetRot - m_rotY, DirectX::XM_2PI);
	m_rotY += std::clamp(diff, -maxRadPerFrame, maxRadPerFrame);
}

float Enemy::GetAttackTurnSpeed()
{
	return kAttackTurnSpeed;
}

void Enemy::StartLeap(const Math::Vector3& target)
{
	// プレイヤーの少し手前を着地点にする
	Math::Vector3 toTarget = target - m_pos;
	toTarget.y = 0.0f;
	float dist = toTarget.Length();
	if (dist > 0.0f) toTarget /= dist;
	float leapDist = std::clamp(dist - kLeapLandOffset * m_scale * 0.5f, 0.0f, kMaxLeapDistance);

	// 空中にいる時間 ＝ 上がる時間（強さ ÷ 重力）＋ 落ちる時間（落ちる時は重力が強いので短い）
	float riseFrames = GetLeapRiseFrames();
	float airFrames = riseFrames * (1.0f + 1.0f / std::sqrt(kLeapFallGravityRate));
	m_leapVelocity = toTarget * (leapDist / airFrames);

	m_gravity = -kLeapJumpPower;
	m_isLeapFalling = false;
	SetFallGravityRate(kLeapFallGravityRate);	// 着地の予測（PredictLandingFrames）もこの重力で計算される
}

float Enemy::GetLeapRiseFrames() const
{
	return kLeapJumpPower / kGravityPerFrame;
}

void Enemy::StopLeap()
{
	m_leapVelocity = Math::Vector3::Zero;
}

void Enemy::UpdateLeapMove()
{
	m_pos += m_leapVelocity * GameTime::GetScale();
	if (m_gravity > kFallingGravity) m_isLeapFalling = true;
}

bool Enemy::IsLeapLanded() const
{
	return m_isLeapFalling && m_gravity >= 0.0f && m_gravity <= kOnGroundGravity;
}

void Enemy::PlaySlamEffect()
{
	// 叩きつけた手（モデルの骨の向きの都合で、攻撃に使う腕は "_l"）の真下の地面
	Math::Vector3 pos = m_pos;
	if (m_model)
	{
		if (const KdModelWork::Node* pNode = m_model->FindNode("hand_l"))
		{
			Math::Vector3 hand = (pNode->m_worldTransform * m_mWorld).Translation();
			pos.x = hand.x;
			pos.z = hand.z;
		}
	}
	pos.y += 0.3f;

	std::shared_ptr<KdEffekseerObject> effect = EffectPlayer::Play(EffectName::HitPlayer, pos, kSlamEffectScaleRate * m_scale);
	if (effect) effect->SetColor(kHitEffectColor);
}

void Enemy::AttackHitNear(bool isCritical)
{
	auto player = m_wpTarget.lock();
	if (!player || player->IsInvincible() || !player->IsAlive()) return;

	float radius = kNearHitRadius * m_scale;
	if (m_pDebugWire)
	{
		m_pDebugWire->AddDebugSphere(m_pos, radius, { 1.0f, 0.0f, 0.0f, 1.0f });
	}

	Math::Vector3 diff = player->GetPos() - m_pos;
	float height = std::abs(diff.y);
	diff.y = 0.0f;
	if (diff.Length() > radius || height > kNearHitHeight) return;

	// 体の向き（モデルが逆向きなので、回転の 180 度反対）。真後ろにいる時は当たらない
	Math::Vector3 forward = { -std::sin(m_rotY), 0.0f, -std::cos(m_rotY) };
	if (diff.LengthSquared() > 0.0001f)
	{
		Math::Vector3 dir = diff;
		dir.Normalize();
		if (dir.Dot(forward) < kNearHitBackDot) return;
	}

	player->OnDamage(isCritical ? kCriticalAttackDamage : kAttackDamage, isCritical);
	PlayHitPlayerEffect(player->GetPos() + Math::Vector3(0.0f, 1.0f, 0.0f));
}

void Enemy::GroundSmash()
{
	float radius = kSmashRadius * m_scale;

	// 地面が揺れる感じを出す（当たらなくても）
	HitStop::Request(kSmashHitStopFrame, kSmashShakePower);
	EffectPlayer::Play(EffectName::BossGroundSmash, m_pos, kSmashEffectScaleRate * m_scale);
	SoundPlayer::PlaySE(SoundName::HeavyHit, 1.0f, -0.5f);	// 地面を叩きつけた、低い音

	if (m_pDebugWire)
	{
		m_pDebugWire->AddDebugSphere(m_pos, radius, { 1.0f, 0.5f, 0.0f, 1.0f });
	}

	// 円の中にいるプレイヤーにダメージ（高く跳んでいればかわせる）
	auto player = m_wpTarget.lock();
	if (!player || player->IsInvincible() || !player->IsAlive()) return;

	Math::Vector3 diff = player->GetPos() - m_pos;
	float height = std::abs(diff.y);
	diff.y = 0.0f;
	if (diff.Length() > radius || height > kSmashHeight) return;

	player->OnDamage(kSmashDamage, false);
	PlayHitPlayerEffect(player->GetPos() + Math::Vector3(0.0f, 1.0f, 0.0f));
}
