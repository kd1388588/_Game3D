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
	constexpr EnemyParam kMidBossParam	= { 220,  1.9f, 12.0f,  2.4f, 60 * 7 };	// 中ボス：ザコとボスの間（中ボスの部屋で、通路のバリアを守る）

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
	// リムライト（体のふちを薄く赤く光らせて、暗い背景に溶けないようにする）
	const Math::Vector3 kLimColor			= { 0.55f, 0.12f, 0.08f };
	constexpr float	kLimPow					= 2.5f;		// 鋭さ（大きいほどふちだけが光る）
	constexpr float	kBumpRadiusRate			= 0.4f;		// 壁・柱に当たる体の半径（体の大きさに掛ける。ザコ 0.5m・ボス 1.0m くらい）
	constexpr float	kLeapWallCheckHeight	= 0.9f;		// 飛びかかる前に、着地点までの間に柱が無いかを調べる高さ
	constexpr float	kPushStrength			= 0.1f;		// 敵同士の押し出しの強さ（毎フレームの割合）
	constexpr float	kHitEffectScale			= 0.22f;	// プレイヤーに攻撃が当たった時のエフェクトの大きさ（大きいと前が見えなくなる）
	// ※エフェクトの色（赤）は、エフェクトエディタ（EffectSettings）で _Claw2.efkefc に付けている

	// プレイヤーに攻撃が当たった時のエフェクト（小さめ・赤っぽい色にして、プレイヤーの攻撃のエフェクトと見分けやすくする）
	void PlayHitPlayerEffect(const Math::Vector3& pos)
	{
		EffectPlayer::Play(EffectName::HitPlayer, pos, kHitEffectScale);
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

	// 普通の攻撃で、手を地面に叩きつけた時の爪のエフェクト（体の大きさに掛ける。ザコ約0.3、ボス約0.6）
	constexpr float	kSlamEffectScaleRate	= 0.24f;
	constexpr float	kSlamEffectHeightRate	= 0.4f;		// 地面からの高さ（体の大きさに掛ける。地面や腕に埋もれて見えなくならないように）
	constexpr float	kSmashClawScaleRate		= 0.35f;	// 飛びかかりの着地で、_GroundSmash が無い時に代わりに出す爪のエフェクトの大きさ

	// 足元の近くへの攻撃判定（体の大きさに掛ける）
	constexpr float	kNearHitRadius			= 0.9f;		// 体の中心からこの距離以内（ボスなら約2.3m、ザコなら約1.1m）
	constexpr float	kNearHitHeight			= 2.0f;		// 上下にこれ以上離れていたら当たらない
	constexpr float	kNearHitBackDot			= -0.3f;	// 体の向きとの内積がこれより小さい（真後ろ）なら当たらない

	// ---- 殴られた時の反応 ----
	// ノックバック：殴られた瞬間に後ろへの速さを与え、毎フレーム kKnockbackDecay 倍にして止める
	//   進む距離の合計 ＝ 速さ ÷ (1 − 減り方)（例：0.12 ÷ 0.22 ≒ 0.55m）
	//   （遠くへ飛ばしすぎると、二刀流の連続攻撃が届かなくなるので短め）
	constexpr float	kKnockbackSpeed			= 0.12f;	// 普通の攻撃
	constexpr float	kKnockbackCriticalRate	= 1.6f;		// クリティカルはこれだけ強く飛ばす
	constexpr float	kKnockbackDecay			= 0.78f;	// 1 フレームごとに掛ける（小さいほど早く止まる）
	constexpr float	kKnockbackArmorRate		= 0.3f;		// スーパーアーマー中（攻撃中）は少しだけ押される
	constexpr float	kKnockbackStunRate		= 0.5f;		// ATB の技を受けている間（技の途中で届かなくならないよう弱め）
	constexpr float	kKnockbackMidBossRate	= 0.5f;		// 中ボスは重いので飛びにくい
	constexpr float	kKnockbackBossRate		= 0.25f;	// ボスはもっと飛びにくい
	constexpr float	kKnockbackStopSpeed		= 0.002f;	// これより遅くなったら止める
	constexpr float	kKnockbackWallHeight	= 0.9f;		// 壁があるかを調べるレイの高さ

	// 白く光る：リムライトを「体全体にかかる白い光」に一瞬だけ変える
	//   リムライトの明るさ ＝ 色 × (1 − 面の向き・カメラの向き)^鋭さ
	//   鋭さを小さくすると、ふちだけでなく体全体が光る
	constexpr int	kHitFlashFrame			= 6;		// 光っている長さ（ヒットストップ中は減らないので、止まっている間はずっと光る）
	const Math::Vector3 kHitFlashColor		= { 1.0f, 0.95f, 0.85f };	// 少し黄色っぽい白
	constexpr float	kHitFlashPow			= 0.3f;		// 光っている時の鋭さ（小さいほど体全体が光る）

	// 小さく揺れる（スーパーアーマー中でも、当たったことが分かるように）
	constexpr int	kHitShakeFrame			= 8;
	constexpr float	kHitShakePower			= 0.03f;	// 揺れる幅（体の大きさに掛ける）
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
	const EnemyParam& param = m_isBoss ? kBossParam : (m_isMidBoss ? kMidBossParam : kNormalParam);
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
	SetBumpRadius(m_scale * kBumpRadiusRate);	// 体の大きさに合わせて、柱にめり込まないようにする
	m_searchRange = param.searchRange;
	m_attackRange = param.attackRange;

	// 初期ステートをIdleに設定
	ChangeState(std::make_shared<EnemyStateIdle>());

	// 当たり判定（プレイヤーの攻撃を受ける所）：足元～腰と、胸～頭の 2 つの球で体全体を覆う
	// （球の位置と半径は、体の大きさ m_scale の分だけ大きくなる）
	// 【なぜ 2 つか】前は足元～腰の 1 つだけだったので、ザコ（体が小さい）の上半身には当たらなかった
	//   例：パニッシャーの 3 段目（回りながら跳ぶ）は剣が高い所を通るので、ボスにしか当たらなかった
	m_pCollider->RegisterCollisionShape
	(
		"Enemy",
		{ 0.0f, 0.5f, 0.0f },
		0.6f,
		KdCollider::Type::TypeDamage
	);
	m_pCollider->RegisterCollisionShape
	(
		"EnemyUpper",
		{ 0.0f, 1.35f, 0.0f },
		0.55f,
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
		UpdateHitReaction();	// 光・揺れが止まったままにならないように（ノックバックはゆっくり進む）
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

	UpdateHitReaction();

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
		KdShaderManager::Instance().m_StandardShader.DrawModel(*m_model, GetShakenWorld());
	}
}

void Enemy::DrawLit()
{
	if (m_model)
	{
		// リムライトは DrawModel の後に自動で元に戻る（ほかの物には付かない）
		auto& shader = KdShaderManager::Instance().m_StandardShader;
		shader.SetLimLightEnable(true);

		// 殴られた直後は白く光らせる（光り始めが一番明るく、だんだん元の赤いふちに戻る）
		float flash = static_cast<float>(m_hitFlashTimer) / kHitFlashFrame;	// 1 → 0
		Math::Vector3 limColor = Math::Vector3::Lerp(kLimColor, kHitFlashColor, flash);
		float limPow = kLimPow + (kHitFlashPow - kLimPow) * flash;
		shader.SetLimLight(limColor, limPow);

		shader.DrawModel(*m_model, GetShakenWorld());
	}
}

void Enemy::UpdateWorldMatrix()
{
	Math::Matrix scale = Math::Matrix::CreateScale(m_scale);
	Math::Matrix rot = Math::Matrix::CreateRotationY(m_rotY);
	Math::Matrix trans = Math::Matrix::CreateTranslation(m_pos);

	m_mWorld = scale * rot * trans;
}

// ==========================================
// 殴られた時の反応
// ==========================================
void Enemy::StartHitReaction(bool isCritical, bool isFlinch)
{
	// 光る・揺れるは、どの時でも必ず出す（スーパーアーマー中でも「当たった」と分かるように）
	m_hitFlashTimer = kHitFlashFrame;
	m_hitShakeTimer = kHitShakeFrame;

	// ノックバックの向き：プレイヤーから敵へ向かう向き（横だけ。上へは飛ばさない）
	auto target = m_wpTarget.lock();
	if (!target) return;
	Math::Vector3 dir = m_pos - target->GetPos();
	dir.y = 0.0f;
	if (dir.LengthSquared() < 0.0001f) return;
	dir.Normalize();

	// 強さ：重い敵ほど、攻撃中ほど、飛びにくくする
	float speed = kKnockbackSpeed;
	if (isCritical)				speed *= kKnockbackCriticalRate;
	if (!isFlinch)				speed *= kKnockbackArmorRate;
	else if (IsStunned())		speed *= kKnockbackStunRate;
	if (m_isBoss)				speed *= kKnockbackBossRate;
	else if (m_isMidBoss)		speed *= kKnockbackMidBossRate;

	// 足すのではなく置きかえる（連続で当たっても、どんどん遠くへ飛んでいかないように）
	m_knockbackVel = dir * speed;
}

void Enemy::UpdateHitReaction()
{
	if (m_hitFlashTimer > 0) m_hitFlashTimer--;
	if (m_hitShakeTimer > 0) m_hitShakeTimer--;

	float speed = m_knockbackVel.Length();
	if (speed < kKnockbackStopSpeed)
	{
		m_knockbackVel = Math::Vector3::Zero;
		return;
	}

	// スローモーション中は、動く量も減り方もゆっくりにする
	float timeScale = GameTime::GetScale();
	float step = speed * timeScale;

	// 後ろに壁・柱があれば、その手前で止める（壁の中へ押しこまないように）
	Math::Vector3 dir = m_knockbackVel / speed;
	Math::Vector3 from = m_pos + Math::Vector3(0.0f, kKnockbackWallHeight, 0.0f);
	float wallDist = FindWallDistance(from, dir, step + m_bumpRadius);
	step = std::min(step, std::max(wallDist - m_bumpRadius, 0.0f));

	m_pos += dir * step;
	m_knockbackVel *= std::pow(kKnockbackDecay, timeScale);
}

Math::Matrix Enemy::GetShakenWorld() const
{
	if (m_hitShakeTimer <= 0) return m_mWorld;

	// 揺れ始めが一番大きく、だんだん小さくする。毎回ランダムな向きにずらす
	float power = kHitShakePower * m_scale * (static_cast<float>(m_hitShakeTimer) / kHitShakeFrame);
	Math::Vector3 offset =
	{
		KdRandom::GetFloat(-power, power),
		0.0f,
		KdRandom::GetFloat(-power, power),
	};
	return m_mWorld * Math::Matrix::CreateTranslation(offset);
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

void Enemy::DebugKill()
{
	m_hp = 0;
	m_isExpired = true;		// シーンから取り除く（撃破エフェクトを出さない）
}

void Enemy::OnDamage(int damage, bool isCritical)
{
	if (m_invincibleTimer > 0) return;
	if (m_hp <= 0) return;

	m_hp -= damage;
	m_invincibleTimer = kDamageInvincibleFrame;

	// のけぞるか（攻撃中のスーパーアーマーは、ATB の技を受けている時だけ破れる）
	bool isFlinch = !m_isSuperArmor || m_stunTimer > 0;

	// 光る・揺れる・ノックバック（倒した一撃でも出す。倒れながら少し後ろへすべる）
	StartHitReaction(isCritical, isFlinch || m_hp <= 0);

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
	if (!isFlinch) return;

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

	// 着地点までの間に柱や壁があれば、その手前に着地する
	// （空中では低い柱の上を越えてしまい、柱の中に着地して出られなくなっていたため）
	if (leapDist > 0.0f)
	{
		Math::Vector3 from = m_pos + Math::Vector3(0.0f, kLeapWallCheckHeight, 0.0f);
		float wallDist = FindWallDistance(from, toTarget, leapDist + m_bumpRadius);
		leapDist = std::min(leapDist, std::max(wallDist - m_bumpRadius, 0.0f));
	}

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
	pos.y += kSlamEffectHeightRate * m_scale;

	EffectPlayer::Play(EffectName::HitPlayer, pos, kSlamEffectScaleRate * m_scale);
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
	if (!EffectPlayer::Play(EffectName::BossGroundSmash, m_pos, kSmashEffectScaleRate * m_scale))
	{
		// _GroundSmash.efkefc がまだ無い時は、爪のエフェクトで代わりにする
		EffectPlayer::Play(EffectName::HitPlayer,
			m_pos + Math::Vector3(0.0f, kSlamEffectHeightRate * m_scale, 0.0f), kSmashClawScaleRate * m_scale);
	}
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
