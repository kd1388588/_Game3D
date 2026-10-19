#include "TitleChara.h"

#include "../../Effect/EffectPlayer.h"
#include "../../../../Framework/Direct3D/Polygon/KdTrailPolygon.h"

namespace
{
	// モデル・アニメーション（Playerと同じもの）
	const std::string kPlayerAssetDir	= "Asset/Models/GameObject/Player/New/";
	const std::string kModelPath		= kPlayerAssetDir + "CharaBase/SK_Assassin.gltf";
	const std::string kAnimDir			= kPlayerAssetDir + "Sequence2/";

	// 見せる攻撃の種類（この順番で繰り返す）
	const std::vector<int> kShowAttackTypes	= { 1, 3, 5 };
	constexpr int	kComboStepNum			= 4;		// 1つの攻撃の種類の段数

	// 演技のタイミング
	constexpr int	kIdleHoldFrame			= 150;		// 攻撃の合間に待機するフレーム数
	constexpr float	kComboCancelFrame		= 25.0f;	// 連続攻撃で次の段に移るフレーム（最後の段は最後まで再生）
	constexpr float	kAnimBlendFrame			= 6.0f;		// アニメーション切り替え時のブレンドフレーム数

	// 斬撃エフェクト（PlayerStateComboAttack と同じ見た目になるように合わせる）
	constexpr float	kSlashEffectFrame		= 10.0f;	// 振り始めのフレーム
	constexpr float	kSlashEffectForward		= 1.0f;		// 前方へずらす距離
	constexpr float	kSlashEffectHeight		= 1.0f;		// 足元からの高さ
	constexpr float	kSlashEffectScale		= 1.0f;		// 大きさ

	// 剣の軌跡（ゲーム中のプレイヤーと同じ見た目）
	const std::vector<std::string> kBladeMeshNodeNamesR	= { "SM_Blade_R" };
	const std::vector<std::string> kBladeMeshNodeNamesL	= { "SM_Blade_L" };
	constexpr float	kBladeBaseOffset		= 0.1f;		// 手から刃の根元（つば）までの距離
	constexpr UINT	kTrailLength			= 15;		// 軌跡を残すフレーム数
	constexpr int	kTrailSubdivision		= 6;		// 1フレームの間を何分割して補間するか
	constexpr float	kTrailStartFrame		= 8.0f;		// 攻撃のこのフレームから軌跡を出す
	constexpr float	kTrailEndFrame			= 40.0f;	// 攻撃のこのフレームまで軌跡を出す

	// カメラが注目する高さ（足元から）
	constexpr float	kLookAtHeight			= 1.1f;
}

std::vector<AnimLoadInfo> TitleChara::CreateAnimList()
{
	std::vector<AnimLoadInfo> animList =
	{
		{ "Combat_Idle", kAnimDir + "01_Idle/02_Idle_Combat/AS_Idle_Combat_Seq/AS_Idle_Combat_Seq.gltf" },
	};

	for (int type : kShowAttackTypes)
	{
		for (int step = 1; step <= kComboStepNum; ++step)
		{
			std::string typeStr = "0" + std::to_string(type);
			std::string stepStr = "0" + std::to_string(step);

			std::string animName = "Attack_" + std::to_string(type) + "_" + std::to_string(step);
			std::string fileName = "AS_Combo_Attack_" + typeStr + "_" + stepStr + "_Seq";
			std::string path = kAnimDir + "02_Attack/"
				+ typeStr + "_Combo_Attack_" + typeStr + "/"
				+ fileName + "/" + fileName + ".gltf";

			animList.push_back({ animName, path });
		}
	}

	return animList;
}

std::vector<TitleChara::ShowcaseStep> TitleChara::CreateShowcase()
{
	std::vector<ShowcaseStep> showcase;

	for (int type : kShowAttackTypes)
	{
		// 構えて待機
		showcase.push_back({ "Combat_Idle", 0, 0, kIdleHoldFrame });

		// 連続攻撃
		for (int step = 1; step <= kComboStepNum; ++step)
		{
			std::string animName = "Attack_" + std::to_string(type) + "_" + std::to_string(step);
			showcase.push_back({ animName, type, step, 0 });
		}
	}

	return showcase;
}

bool TitleChara::IsInPlaceAttack(int attackType, int attackStep)
{
	// PlayerStateComboAttack::IsRootMotionLocked() と同じ攻撃
	return (attackType == 5 && (attackStep == 2 || attackStep == 3));
}

const std::string& TitleChara::GetSlashEffectName(int attackType, int attackStep)
{
	if (attackType == 1)
	{
		switch (attackStep)
		{
		case 1:	return EffectName::ComboSlash1_1;
		case 2:	return EffectName::ComboSlash1_2;
		case 3:	return EffectName::ComboSlash1_3;
		case 4:	return EffectName::ComboSlash1_4;
		default: break;
		}
	}
	if (attackType == 5 && attackStep == 4)
	{
		return EffectName::ComboSlash5_4;
	}
	return EffectName::ComboSlashDefault;
}

void TitleChara::Init()
{
	m_model = std::make_shared<KdModelWork>();
	m_model->SetModelData(kModelPath);

	// 踏み込む攻撃では腰をキャラの位置に固定し、技がつながっても体が元の位置に戻らないようにする
	// （移動量は使わないので、その場で演技する）
	SetExtractPelvisMotion(true);

	LoadAnimations(CreateAnimList());

	m_showcase = CreateShowcase();
	StartStep(0);

	m_pos = Math::Vector3::Zero;
	UpdateWorldMatrix();

	// 剣の軌跡：モデルの剣のメッシュから刃の形を求める（剣が無いモデルなら出さない）
	if (m_model->GetData())
	{
		m_trailR.shape = Player::CalcBladeShape(*m_model->GetData(), kBladeMeshNodeNamesR);
		m_trailL.shape = Player::CalcBladeShape(*m_model->GetData(), kBladeMeshNodeNamesL);
	}
	m_trailTex = Player::LoadOperatorTrailTexture();
	for (SwordTrail* pTrail : { &m_trailR, &m_trailL })
	{
		pTrail->polygon = std::make_shared<KdTrailPolygon>();
		pTrail->polygon->SetMaterial(m_trailTex);
		pTrail->polygon->SetLength(kTrailLength * kTrailSubdivision);
	}
}

void TitleChara::Update()
{
	UpdateShowcase();

	// アニメーション・重力（ルートモーションは使わず、その場で演技する）
	BaseChara::Update();

	UpdateWorldMatrix();
	UpdateSwordTrails();
}

void TitleChara::UpdateSwordTrails()
{
	// 攻撃を振っている間だけ軌跡を足し、それ以外は古い所から消していく
	bool isAttack = !m_showcase.empty() && m_showcase[m_stepIndex].attackType != 0;
	float animTime = GetAnimTime();
	bool isTrailFrame = isAttack && animTime >= kTrailStartFrame && animTime <= kTrailEndFrame;

	for (SwordTrail* pTrail : { &m_trailR, &m_trailL })
	{
		if (isTrailFrame)	AddSwordTrailSample(*pTrail);
		else				ShrinkSwordTrail(*pTrail);
	}
}

void TitleChara::AddSwordTrailSample(SwordTrail& trail)
{
	if (!trail.polygon || !m_model || trail.shape.nodeIndex < 0) return;

	Math::Matrix swordMat = Player::CalcBladeMatrix(trail.shape, *m_model, m_mWorld);
	float baseY = std::min(kBladeBaseOffset, trail.shape.tipLength * 0.5f);
	Math::Vector3 base = Player::GetSwordPoint(swordMat, baseY);
	Math::Vector3 tip = Player::GetSwordPoint(swordMat, trail.shape.tipLength);

	auto& baseHist = trail.baseHistory;
	auto& tipHist = trail.tipHistory;
	if (baseHist.empty())
	{
		trail.polygon->AddPoint(Player::CreateTrailMatrix(base, tip));
	}
	else
	{
		// 前のフレームとの間を曲線（Catmull-Rom）で補間して、なめらかな弧にする（Player と同じ）
		Math::Vector3 base0 = (baseHist.size() >= 2) ? baseHist[1] : baseHist[0];
		Math::Vector3 tip0 = (tipHist.size() >= 2) ? tipHist[1] : tipHist[0];
		Math::Vector3 base1 = baseHist[0];
		Math::Vector3 tip1 = tipHist[0];
		Math::Vector3 base3 = base + (base - base1);
		Math::Vector3 tip3 = tip + (tip - tip1);
		for (int i = 1; i <= kTrailSubdivision; ++i)
		{
			float t = static_cast<float>(i) / kTrailSubdivision;
			trail.polygon->AddPoint(Player::CreateTrailMatrix(
				Math::Vector3::CatmullRom(base0, base1, base, base3, t),
				Math::Vector3::CatmullRom(tip0, tip1, tip, tip3, t)));
		}
	}

	baseHist.push_front(base);
	tipHist.push_front(tip);
	while (baseHist.size() > 2) baseHist.pop_back();
	while (tipHist.size() > 2) tipHist.pop_back();
}

void TitleChara::ShrinkSwordTrail(SwordTrail& trail)
{
	// 振り終わったら履歴を消す（次の振りの軌跡とつながらないように）
	trail.baseHistory.clear();
	trail.tipHistory.clear();

	for (int i = 0; i < kTrailSubdivision; ++i)
	{
		if (!trail.polygon || trail.polygon->GetNumPoints() <= 0) break;
		trail.polygon->DelPointBack();
	}
}

void TitleChara::DrawEffect()
{
	// 軌跡は加算合成・深度書き込みなしで描く（Player と同じ）
	auto& shaderManager = KdShaderManager::Instance();
	shaderManager.ChangeRasterizerState(KdRasterizerState::CullNone);
	shaderManager.ChangeBlendState(KdBlendState::Add);
	shaderManager.ChangeDepthStencilState(KdDepthStencilState::ZWriteDisable);

	for (const SwordTrail* pTrail : { &m_trailR, &m_trailL })
	{
		if (pTrail->polygon) shaderManager.m_StandardShader.DrawPolygon(*pTrail->polygon);
	}

	shaderManager.UndoDepthStencilState();
	shaderManager.UndoBlendState();
	shaderManager.UndoRasterizerState();
}

void TitleChara::StartStep(size_t index)
{
	if (m_showcase.empty()) return;

	m_stepIndex = index % m_showcase.size();
	m_stepTimer = 0;
	m_isEffectPlayed = false;

	const ShowcaseStep& step = m_showcase[m_stepIndex];
	bool isLoop = (step.holdFrame > 0);
	ChangeAnimation(step.animName, isLoop, true, kAnimBlendFrame);

	// 腰を固定するのは、踏み込む攻撃の時だけ（ゲーム中と同じ見た目にする）
	bool isAttack = (step.attackType != 0);
	SetUseRootMotion(isAttack && !IsInPlaceAttack(step.attackType, step.attackStep));
}

void TitleChara::UpdateShowcase()
{
	if (m_showcase.empty()) return;

	const ShowcaseStep& step = m_showcase[m_stepIndex];
	m_stepTimer++;

	// 待機：決まった時間だけ続ける
	if (step.holdFrame > 0)
	{
		if (m_stepTimer >= step.holdFrame)
		{
			StartStep(m_stepIndex + 1);
		}
		return;
	}

	// 攻撃：振り始めに斬撃エフェクト
	if (!m_isEffectPlayed && GetAnimTime() >= kSlashEffectFrame)
	{
		PlaySlashEffect(step);
		m_isEffectPlayed = true;
	}

	// 途中の段は少し早めに次の段へつなぎ、最後の段は最後まで見せる
	bool isLastStep = (step.attackStep >= kComboStepNum);
	bool canCancel = (!isLastStep && GetAnimTime() >= kComboCancelFrame);
	if (canCancel || IsAnimEnd() || !HasAnimation(step.animName))
	{
		StartStep(m_stepIndex + 1);
	}
}

void TitleChara::PlaySlashEffect(const ShowcaseStep& step)
{
	// キャラクターの前方にエフェクトを出す（m_angle の向きがローカル+Z）
	Math::Matrix rot = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(m_angle));
	Math::Vector3 forward = Math::Vector3::TransformNormal(Math::Vector3(0.0f, 0.0f, 1.0f), rot);
	Math::Vector3 pos = m_pos + forward * kSlashEffectForward + Math::Vector3(0.0f, kSlashEffectHeight, 0.0f);

	Math::Matrix mWorld = Math::Matrix::CreateScale(kSlashEffectScale) * rot * Math::Matrix::CreateTranslation(pos);
	EffectPlayer::Play(GetSlashEffectName(step.attackType, step.attackStep), mWorld);
}

void TitleChara::UpdateWorldMatrix()
{
	// モデルが逆向きなので180度回して描画する（Playerと同じ）
	Math::Matrix rotate = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(m_angle + 180.0f));
	m_mWorld = rotate * Math::Matrix::CreateTranslation(m_pos);
}

void TitleChara::DrawLit()
{
	if (!m_model) return;

	auto& shaderManager = KdShaderManager::Instance();

	shaderManager.ChangeRasterizerState(KdRasterizerState::CullNone);
	shaderManager.ChangeBlendState(KdBlendState::Alpha);

	shaderManager.m_StandardShader.DrawModel(*m_model, m_mWorld);

	shaderManager.UndoBlendState();
	shaderManager.UndoRasterizerState();
}

Math::Vector3 TitleChara::GetLookAtPos() const
{
	return m_pos + Math::Vector3(0.0f, kLookAtHeight, 0.0f);
}
