#include "TitleChara.h"

#include "../../Effect/EffectPlayer.h"

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

	// 腰をキャラの位置に固定し、技がつながっても体が元の位置に戻らないようにする
	// （ルートモーションは使わないので、その場で演技する）
	SetExtractPelvisMotion(true);

	LoadAnimations(CreateAnimList());

	m_showcase = CreateShowcase();
	StartStep(0);

	m_pos = Math::Vector3::Zero;
	UpdateWorldMatrix();
}

void TitleChara::Update()
{
	UpdateShowcase();

	// アニメーション・重力（ルートモーションは使わず、その場で演技する）
	BaseChara::Update();

	UpdateWorldMatrix();
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
