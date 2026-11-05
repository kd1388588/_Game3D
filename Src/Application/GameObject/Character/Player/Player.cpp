// Player.cpp
// プレイヤーの中心（初期化・毎フレームの更新・ステートの切り替え・被弾）
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

// Scene
#include "../../../Scene/SceneManager.h"
#include "../../../Scene/BaseScene/BaseScene.h"

#include "PlayerConstants.h"
#include "../../../Utility/EffectSettings.h"
#include "../../../Utility/StageSettings.h"

using InputHelper::IsKeyDown;
using namespace PlayerConst;

Math::Vector3 Player::s_weaponRotR = { 0.0f, 0.0f, 0.0f };
Math::Vector3 Player::s_weaponPosR = { 0.0f, 0.0f, 0.0f };
Math::Vector3 Player::s_weaponRotL = { 0.0f, 0.0f, 0.0f };
Math::Vector3 Player::s_weaponPosL = { 0.0f, 0.0f, 0.0f };

int Player::s_currentAttackType = 1;

bool Player::s_useRootMotion = true;
bool Player::s_bladeShapeHitbox = true;
float Player::s_throwAngleOffset = 0.0f;
float Player::s_runSpeedRate = 1.5f;

// 読み込むアニメーションのリストを作成
// ※プレイヤーは基本的に剣を抜いているので、剣を納めている時のアニメーションは、ダッシュに使うもの
//   （走りながら納刀・剣を納めて全力で走る・急ブレーキ・立ったまま抜刀）だけを読み込む（他のファイルは Asset フォルダに残してある）
std::vector<AnimLoadInfo> Player::CreateAnimList()
{
	std::vector<AnimLoadInfo> animList =
	{
		{ "Combat_Idle",		kAnimDir + "01_Idle/02_Idle_Combat/AS_Idle_Combat_Seq/AS_Idle_Combat_Seq.gltf" },
		{ "Evade",				kAnimDir + "07_Roll/02_Roll_Combat/AS_Roll_Combat_F_0_Seq/AS_Roll_Combat_F_0_Seq.gltf" },
		{ "Combat_Run",			kAnimDir + "04_Run/02_Run_Combat/01_Run_Combat_F_0/AS_Run_Combat_F_0_Loop_Seq/AS_Run_Combat_F_0_Loop_Seq.gltf" },
		{ "Run_Sheathe",		kAnimDir + "04_Run/04_Run_Combat_to_Run/AS_Run_Combat_to_Run_Seq/AS_Run_Combat_to_Run_Seq.gltf" },	// 走りながら納刀
		{ "Dash_Run",			kAnimDir + "04_Run/01_Run/11_Run_Fast/AS_Run_Fast_Loop_Seq/AS_Run_Fast_Loop_Seq.gltf" },				// 剣を納めて全力で走る
		{ "Dash_Brake",			kAnimDir + "04_Run/01_Run/11_Run_Fast/AS_Run_Fast_End_Seq/AS_Run_Fast_End_Seq.gltf" },				// 全力疾走から急ブレーキ
		{ "Idle_Draw",			kAnimDir + "01_Idle/01_Idle/AS_Idle_to_Idle_Combat_Seq/AS_Idle_to_Idle_Combat_Seq.gltf" },			// 立ったまま抜刀
		{ "Combat_JumpStart",	kAnimDir + "05_Jump/02_Jump_Combat/AS_Jump_Combat_Start_0_Seq/AS_Jump_Combat_Start_0_Seq.gltf" },
		{ "Combat_JumpLoop",	kAnimDir + "05_Jump/02_Jump_Combat/AS_Jump_Combat_Loop_0_Seq/AS_Jump_Combat_Loop_0_Seq.gltf" },
		{ "Combat_JumpEnd",		kAnimDir + "05_Jump/02_Jump_Combat/AS_Jump_Combat_End_0_Seq/AS_Jump_Combat_End_0_Seq.gltf" },
		{ "Damage",				kAnimDir + "08_Hit/01_Hit/AS_Hit_F_Seq/AS_Hit_F_Seq.gltf" },
		{ "GuardStart",			kAnimDir + "08_Hit/14_Block/AS_Block_Start_Seq/AS_Block_Start_Seq.gltf" },
		{ "GuardLoop",			kAnimDir + "08_Hit/14_Block/AS_Block_Loop_Seq/AS_Block_Loop_Seq.gltf" },
		{ "GuardHit",			kAnimDir + "08_Hit/14_Block/AS_Block_Hit_Seq/AS_Block_Hit_Seq.gltf" },
		{ "GuardEnd",			kAnimDir + "08_Hit/14_Block/AS_Block_End_Seq/AS_Block_End_Seq.gltf" },
		{ "GuardBreak",			kAnimDir + "08_Hit/14_Block/AS_Block_Hit_Break_Seq/AS_Block_Hit_Break_Seq.gltf" },
		{ "Death",				kAnimDir + "08_Hit/01_Hit/AS_Hit_Death_Seq/AS_Hit_Death_Seq.gltf" },
	};

	// ガードしながら歩くアニメーション（8方向）
	for (const auto& def : kGuardWalkAnimDefs)
	{
		std::string fileName = std::string("AS_Walk_Block_Loop_") + def.dir + "_Seq";
		animList.push_back({ def.name, kAnimDir + "03_Walk/03_Walk_Block/" + def.folder + "/" + fileName + "/" + fileName + ".gltf" });
	}

	// 攻撃アニメーション（Attack_タイプ_段数）
	for (int type = 1; type <= kAttackTypeNum; ++type)
	{
		for (int step = 1; step <= 4; ++step)
		{
			std::string typeStr = "0" + std::to_string(type);
			std::string stepStr = "0" + std::to_string(step);

			// 4-3 だけは Start / Loop / End の3分割
			if (type == 4 && step == 3)
			{
				std::string basePath = kAnimDir + "02_Attack/04_Combo_Attack_04/AS_Combo_Attack_04_03_";
				animList.push_back({ "Attack_4_3_Start", basePath + "Start_Seq/AS_Combo_Attack_04_03_Start_Seq.gltf" });
				animList.push_back({ "Attack_4_3_Loop",  basePath + "Loop_Seq/AS_Combo_Attack_04_03_Loop_Seq.gltf" });
				animList.push_back({ "Attack_4_3_End",   basePath + "End_Seq/AS_Combo_Attack_04_03_End_Seq.gltf" });
				continue;
			}

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

void Player::Init()
{
	m_model = std::make_shared<KdModelWork>();
	m_swordModel = std::make_shared<KdModelData>();
	m_swordTrailR.polygon = std::make_shared<KdTrailPolygon>();
	m_swordTrailL.polygon = std::make_shared<KdTrailPolygon>();

	// 当たり判定（敵の攻撃を受ける所）：足元～腰と、胸～頭の2つの球で体全体を覆う
	// （前は腰から下だけだったので、高い所を振る敵の腕が当たりにくかった）
	m_pCollider = std::make_unique<KdCollider>();
	m_pCollider->RegisterCollisionShape
	(
		"Player",
		{ 0.0f, 0.5f, 0.0f },
		0.6f,
		KdCollider::Type::TypeDamage
	);
	m_pCollider->RegisterCollisionShape
	(
		"PlayerUpper",
		{ 0.0f, 1.35f, 0.0f },
		0.5f,
		KdCollider::Type::TypeDamage
	);

	m_pDebugWire = std::make_unique<KdDebugWireFrame>();

	// モデル・武器セットの読み込み
	m_model->SetModelData(kPlayerModelPath);

	// アニメーションで腰が水平に動いた分は、プレイヤー自体の移動に置き換える
	// （体が元の位置に戻らず、アニメーションが自然につながるようにする）
	SetExtractPelvisMotion(true);
	if (!kSwordModelPath.empty())		m_swordModel->Load(kSwordModelPath);

	// 武器をどのノードに付けたかを出力ウィンドウに表示（モデルを差し替えた時の確認用）
	for (const auto* pNames : { &kSwordNodeNamesR, &kSwordNodeNamesL })
	{
		const KdModelWork::Node* pNode = FindFirstNode(*pNames);
		OutputDebugStringA(("武器の取り付け先: " + (pNode ? pNode->m_name : std::string("見つからない")) + "\n").c_str());
	}

	PlayerParamManager::Instance().Load("Asset/Data/PlayerParams.txt");
	PlayerParamManager::Instance().LoadWeaponParams("Asset/Data/WeaponParams.json");

	// モデルに剣が含まれていれば、剣のメッシュから刃の向き・長さを求める
	// （攻撃判定の箱と軌跡を、実際の刃に合わせるため）
	m_swordBladeBaseY = kSwordBladeBaseY;
	m_swordBladeTipY = kSwordBladeTipY;
	if (m_model->GetData())
	{
		m_bladeShapeR = CalcBladeShape(*m_model->GetData(), kBladeMeshNodeNamesR);
		m_bladeShapeL = CalcBladeShape(*m_model->GetData(), kBladeMeshNodeNamesL);
	}
	if (m_bladeShapeR.nodeIndex >= 0)
	{
		m_swordBladeBaseY = std::min(kBladeBaseOffset, m_bladeShapeR.tipLength * 0.5f);
		m_swordBladeTipY = m_bladeShapeR.tipLength;
		OutputDebugStringA(("剣のメッシュから刃の長さを計算: " + std::to_string(m_bladeShapeR.tipLength) + "\n").c_str());
	}

	// アニメーションの読み込み
	LoadAnimations(CreateAnimList());

	// 剣を投げる攻撃で、剣をまっすぐ前へ向けるための角度を事前に計算
	m_weaponNodeR = FindNodeIndex("weapon_r");
	m_weaponNodeL = FindNodeIndex("weapon_l");
	m_handNodeR = FindNodeIndex("hand_r");
	m_handNodeL = FindNodeIndex("hand_l");
	m_pelvisNode = FindNodeIndex("pelvis");

	// 足音を鳴らすための足の骨（つま先の付け根が無いモデルなら足首）と、立っている時の高さ
	for (int f = 0; f < 2; ++f)
	{
		m_footstepNodes[f] = FindNodeIndex(kToeNodeNames[f]);
		if (m_footstepNodes[f] < 0) m_footstepNodes[f] = FindNodeIndex(kFootNodeNames[f]);
		if (m_footstepNodes[f] >= 0)
		{
			m_footGroundY[f] = m_model->GetData()->GetOriginalNodes()[m_footstepNodes[f]].m_worldTransform.Translation().y;
		}
	}

	m_throwAngleR = CalcThrowCorrectionAngle(kThrowAnimName, m_weaponNodeR);
	m_throwAngleL = CalcThrowCorrectionAngle(kThrowAnimName, m_weaponNodeL);
	OutputDebugStringA(("剣を投げる向きの補正: 右 " + std::to_string(m_throwAngleR)
		+ " 度 / 左 " + std::to_string(m_throwAngleL) + " 度\n").c_str());

	// 足の接地を調べる骨（つま先の付け根が無いモデルなら足首）
	const std::vector<std::string>& groundNodeNames =
		(FindNodeIndex(kToeNodeNames[0]) >= 0 && FindNodeIndex(kToeNodeNames[1]) >= 0) ? kToeNodeNames : kFootNodeNames;

	// ジャンプ開始アニメーションで、足が地面から離れる（踏み切る）フレームを事前に計算
	for (const auto& animName : kTakeoffAnimNames)
	{
		if (!HasAnimation(animName)) continue;

		float frame = CalcFootLiftFrame(animName, groundNodeNames);
		m_takeoffFrames[animName] = (frame >= 0.0f && frame <= kMaxTakeoffFrame) ? frame : 0.0f;
		OutputDebugStringA(("踏み切りフレーム: " + animName + " = " + std::to_string(frame) + "\n").c_str());
	}

	// 空中で腰が上がってよい高さ ＝ 空中ループのアニメーションの腰の一番高い位置（立ち位置より低ければ立ち位置）
	// （ジャンプ開始のアニメーションは、踏み切った後に腰が空中ループより高く上がってから下がる。
	//   プログラムでもキャラを上に飛ばしているので、その分は止めて、2重に上下しないようにする）
	m_airPelvisMaxY = m_pelvisRestPos.y;
	if (auto it = m_animMap.find(kAirLoopAnimName); it != m_animMap.end() && it->second)
	{
		for (float t = 0.0f; t <= it->second->m_maxLength; t += 1.0f)
		{
			Math::Vector3 pelvisPos;
			SampleMotionPos(*it->second, t, &pelvisPos);
			m_airPelvisMaxY = std::max(m_airPelvisMaxY, pelvisPos.y);
		}
	}
	OutputDebugStringA(("空中で腰が上がってよい高さ: " + std::to_string(m_airPelvisMaxY) + "\n").c_str());

	// ジャンプのアニメーションで、腰がどれだけ上下するかを出力ウィンドウに表示（ジャンプの調整用）
	// （上がる量が大きいほど、アニメーション自体に「跳ぶ高さ」が入っている）
	for (const char* animName : { "Combat_JumpStart", "Combat_JumpLoop", "Combat_JumpEnd" })
	{
		auto it = m_animMap.find(animName);
		if (it == m_animMap.end() || !it->second) continue;

		float minY = std::numeric_limits<float>::max();
		float maxY = -std::numeric_limits<float>::max();
		for (float t = 0.0f; t <= it->second->m_maxLength; t += 1.0f)
		{
			Math::Vector3 pelvisPos;
			SampleMotionPos(*it->second, t, &pelvisPos);
			minY = std::min(minY, pelvisPos.y - m_pelvisRestPos.y);
			maxY = std::max(maxY, pelvisPos.y - m_pelvisRestPos.y);
		}
		OutputDebugStringA(("ジャンプの腰の上下: " + std::string(animName)
			+ " 長さ " + std::to_string(it->second->m_maxLength)
			+ " / 最低 " + std::to_string(minY) + " / 最高 " + std::to_string(maxY) + "\n").c_str());
	}

	// ガード歩き：アニメーションごとに、どの方向へどれくらいの速さで進むかを調べる
	SetupGuardWalkAnims();

	// 04の4段目（跳び上がって剣を地面に叩きつける）で、叩きつけるフレームを事前に計算
	// （剣の軌跡・攻撃判定を、跳んでいる間から叩きつけるまで出すため）
	m_jumpSlamFrame = CalcJumpSlamFrame(kJumpSlamAnimName);
	OutputDebugStringA(("04の4段目で剣を叩きつけるフレーム: " + std::to_string(m_jumpSlamFrame) + "\n").c_str());

	// 着地アニメーションで両足が接地するフレームを事前に計算
	for (const auto& animName : kLandingAnimNames)
	{
		if (!HasAnimation(animName)) continue;

		float frame = CalcFootPlantFrame(animName, groundNodeNames);
		m_landingFrames[animName] = frame;
		OutputDebugStringA(("着地フレーム: " + animName + " = " + std::to_string(frame) + "\n").c_str());
	}

	ChangeState(std::make_shared<PlayerStateIdle>());
	ChangeAnimation("Combat_Idle", true);

	// 剣の軌跡（左右）
	// 画像はモードごとに用意しておき、モードを切り替えた時に差し替える
	m_operatorTrailTex = LoadSwordTrailTexture(kOperatorTrailTexPath, kOperatorTrailColor);
	m_punisherTrailTex = LoadSwordTrailTexture(kPunisherTrailTexPath, kPunisherTrailColor);
	for (SwordTrail* pTrail : { &m_swordTrailR, &m_swordTrailL })
	{
		pTrail->polygon->SetLength(kSwordTrailLength * kSwordTrailSubdivision);
	}
	ApplySwordTrailTexture();

	// スタート位置（道の始まりの壁は z = -15。最初の敵は z = 10）
	m_pos = { 0.0f, 0.0f, -8.0f };
	m_prevFramePos = m_pos;

	// カメラの注視点の高さ：立っている時の腰の高さを基準にする（腰が上下に動いても付いていかない）
	float pelvisHeight = (m_pelvisRestPos.y > 0.0f) ? m_pelvisRestPos.y : 0.9f;	// 腰の骨が無いモデルの時は標準の高さ
	m_cameraHeight = pelvisHeight + kCameraTargetOffsetY;
	m_cameraTargetY = m_pos.y + m_cameraHeight;

	m_maxHp = kMaxHp;
	m_hp = m_maxHp;

	// HPゲージ
	std::shared_ptr<HPGage> hpGage = std::make_shared<HPGage>();
	hpGage->Init();
	if (m_owner)
	{
		m_owner->AddObject(hpGage);
	}
	m_wpHpGage = hpGage;

	// ATBゲージ（HPゲージの下）
	std::shared_ptr<ATBGage> atbGage = std::make_shared<ATBGage>();
	atbGage->Init();
	if (m_owner)
	{
		m_owner->AddObject(atbGage);
	}
	m_wpAtbGage = atbGage;

	// ATBコマンドメニュー
	std::shared_ptr<ATBCommandMenu> commandMenu = std::make_shared<ATBCommandMenu>();
	commandMenu->Init();
	if (m_owner)
	{
		m_owner->AddObject(commandMenu);
	}
	m_wpCommandMenu = commandMenu;

	// 最初はオペレーターモード
	m_atbGage = 0.0f;
	SetBattleMode(BattleMode::Operator);

	// 前のシーンのヒットストップ・スローモーションが残らないようにする
	HitStop::Reset();
	GameTime::Reset();
	m_isCommandOpen = false;
}

Player::~Player()
{
	// スローモーション・ヒットストップのまま別のシーンへ行かないようにする
	GameTime::Reset();
	HitStop::Reset();
}

void Player::Update()
{
	// エフェクトの色に使う、今のモード（エフェクトエディタ。シーンをやり直した時も合うように毎フレーム教える）
	EffectSettings::Instance().SetPlayerPunisher(IsPunisherMode());

	// ボタンの入力は、ヒットストップ中も受け付ける（止まっている間に押した入力を取りこぼさないように）
	UpdateInputBuffer();

	// ヒットストップ中はアニメーションも移動も止め、座標の更新（描画）だけ行う
	// （敵も同じように止まる。時間は PostUpdate で進める）
	if (HitStop::IsStopping())
	{
		UpdateWorldMatrix();
		return;
	}

	// ATBコマンドメニュー（開いている間は、時間がゆっくりになり、技を選ぶ操作だけを受け付ける）
	if (UpdateCommandMenu())
	{
		// 止まっている扱いにする（カメラが横移動に付いていかないように）
		m_lastMoveDelta = Math::Vector3::Zero;
		m_prevFramePos = m_pos;

		BaseChara::Update();		// アニメーション・重力はゆっくり進む
		ApplyAnimationMotion();
		ApplyThrowCorrection();
		UpdateHpGage();
		UpdateAtb();
		UpdateWorldMatrix();
		return;
	}

	// 前のフレームに水平方向へ動いた量を記録（ジャンプで勢いを引き継ぐため）
	m_lastMoveDelta = m_pos - m_prevFramePos;
	m_lastMoveDelta.y = 0.0f;
	m_prevFramePos = m_pos;

	// ルートモーションの切り替え（確認用。オフにすると、腰の動きはその場でのアニメーションになる）
	if (m_extractPelvisMotion != s_useRootMotion)
	{
		SetExtractPelvisMotion(s_useRootMotion);
	}

	// アニメーション時間の進行とCalcNodeMatricesの実行はBaseで
	BaseChara::Update();

	// アニメーションで腰が動いた分だけ、プレイヤーを動かす
	ApplyAnimationMotion();

	// 剣を投げる攻撃では、投げた剣がまっすぐ前へ飛ぶように補正する
	ApplyThrowCorrection();

	UpdateInvincibleTimer();

	m_isTrailAddedThisFrame = false;
	m_attackFrame++;

	// ガード中の印は、ガードのステートが毎フレーム付け直す
	// （敵の攻撃はこの後の敵の Update で当たるので、ステートの Update より前に消しておく）
	m_isGuarding = false;

	if (m_state)
	{
		m_state->Update(this);
	}

	UpdateSwordTrails();
	UpdateFootsteps();
	UpdateHpGage();
	UpdateModeSwitch();
	UpdateAbilityTimers();
	UpdateAtb();
	UpdateLockOn();

	// リムライト：pow を少しずつ大きくする（体全体 → ふちだけ → 消える）
	if (m_isLimLight)
	{
		m_limPow += kLimPowSpeed;
		if (m_limPow > kLimPowMax)
		{
			m_isLimLight = false;
		}
	}

	// パニッシャーのオーラ：パニッシャーの間は出して、それ以外は消す
	m_auraFrame++;
	float auraTarget = (IsPunisherMode() && IsAlive()) ? 1.0f : 0.0f;
	if (m_auraRate < auraTarget)	m_auraRate = std::min(m_auraRate + kAuraFadeSpeed, auraTarget);
	else							m_auraRate = std::max(m_auraRate - kAuraFadeSpeed, auraTarget);

	UpdateWorldMatrix();
}

void Player::PostUpdate()
{
	// 敵との押し出しを先にして、その後の壁の押し出し（BumpHit）で壁にめり込まないようにする
	PushOutFromEnemies();

	BaseChara::PostUpdate();
	UpdateWeaponMatrix();
	UpdateCameraTargetY();

	// ヒットストップの時間を進める（すべてのキャラの Update が終わった後なので、敵も同じフレーム数だけ止まる）
	HitStop::Advance();
}

void Player::UpdateCameraTargetY()
{
	// 基本は足元の高さに合わせる（ジャンプ・階段・落下）
	float desiredY = m_pos.y + m_cameraHeight;

	// アニメーションで体が上に上がった分（ジャンプや飛び上がる攻撃）だけ、カメラも上げる
	// 腰が下がった分（回避・しゃがむ動き）は無視するので、回避でカメラがガクンと下がらない
	if (m_model && m_pelvisNode >= 0 && m_pelvisNode < static_cast<int>(m_model->GetNodes().size()))
	{
		float pelvisY = m_model->GetNodes()[m_pelvisNode].m_worldTransform.Translation().y;
		float rise = pelvisY - m_pelvisRestPos.y - kCameraRiseDeadZone;
		if (rise > 0.0f) desiredY += rise;
	}

	m_cameraTargetY += (desiredY - m_cameraTargetY) * kCameraFollowRateY;
}

void Player::UpdateWorldMatrix()
{
	// モデルが逆向きなので180度回して描画する
	Math::Matrix rotate = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(m_angle + 180.0f));
	Math::Matrix trans = Math::Matrix::CreateTranslation(m_pos);
	m_mWorld = rotate * trans;
}

void Player::UpdateHpGage()
{
	float ratio = std::max(static_cast<float>(m_hp) / static_cast<float>(m_maxHp), 0.0f);

	// HP が少ない時の画面：HP が「始まる HP」より少ないほど、白黒・暗く・周辺減光を強くする（HP 0 で一番強い）
	// 急に変わらないように、毎フレーム少しずつ近づける（設定はステージエディタの「画面」タブ）
	const StageSettings::LowHpScreen& low = StageSettings::Instance().GetLowHpScreen();
	float target = 0.0f;
	if (low.enable && low.startRate > 0.0f)
	{
		target = std::clamp((low.startRate - ratio) / low.startRate, 0.0f, 1.0f);
	}
	m_lowHpScreenRate += (target - m_lowHpScreenRate) * std::clamp(low.followRate, 0.0f, 1.0f);

	// 倒れた時：途中まで暗くなっていた画面を、決めた時間をかけて真っ黒にする（真っ黒になったら GameScene がゲームオーバーへ移す）
	if (IsDead())
	{
		float fadeFrames = std::max(low.deathFadeSec, 0.01f) * 60.0f;
		m_deathFadeRate = std::min(m_deathFadeRate + 1.0f / fadeFrames, 1.0f);
	}
	float fade = m_deathFadeRate * m_deathFadeRate * (3.0f - 2.0f * m_deathFadeRate);	// 始めと終わりをなめらかに
	float darken = low.darken * m_lowHpScreenRate;
	KdShaderManager::Instance().m_postProcessShader.SetScreenColor(
		low.grayscale * m_lowHpScreenRate, darken + (1.0f - darken) * fade, low.vignette * m_lowHpScreenRate, low.vignetteStart);

	auto hpGage = m_wpHpGage.lock();
	if (!hpGage) return;
	hpGage->SetHpRatio(ratio);
}

void Player::Release()
{
	m_model = nullptr;
	m_swordModel = nullptr;
}

void Player::ChangeState(const std::shared_ptr<PlayerState>& newState)
{
	// アビリティの攻撃中・ガード中の印は、次のステートに入る時に消す（そのステートが入る時にまた付ける）
	m_isAbilityAttack = false;
	m_isGuarding = false;

	// ジャンプ用の設定（空中で腰が上がらないようにする・速く落ちる）も、ステートが変わったら元に戻す
	SetAirPelvisMaxY(-1.0f);
	SetFallGravityRate(1.0f);

	m_state = newState;
	if (m_state)
	{
		m_state->ChangeState(this);
	}
}

bool Player::IsOnGround() const
{
	return m_gravity >= 0.0f && m_gravity <= kOnGroundGravityMax;
}

void Player::OnDamage(int damage, bool isCritical)
{
	if (m_invincibleTimer > 0) return;
	if (m_hp <= 0) return;
	if (m_isDebugInvincible) return;	// デバッグキー（F6）で無敵

	// パニッシャーの時は、受けるダメージが増える
	if (m_battleMode == BattleMode::Punisher)
	{
		damage = static_cast<int>(std::round(damage * kPunisherTakenDamageRate));
	}

	// 回避中はダメージを半分にする（1 より小さくはしない）
	if (std::dynamic_pointer_cast<PlayerStateEvade>(m_state))
	{
		damage = std::max(static_cast<int>(std::round(damage * kEvadeDamageRate)), 1);
	}

	// ガード中：ダメージを減らし、のけぞらない
	if (m_isGuarding)
	{
		m_hp -= std::max(static_cast<int>(std::round(damage * kGuardDamageRate)), 1);
		m_invincibleTimer = kDamageInvincibleFrame;
		EffectPlayer::Play(EffectName::Guard, CreateGuardEffectMatrix());
		SoundPlayer::PlaySE(SoundName::HeavyHit, kGuardSEVolume, kGuardSEPitch, 0.05f);	// 盾で受け止めた、軽く高い音

		// ガードの耐久値を減らす（受けすぎると崩される）
		float cost = damage * kGuardCostPerDamage * (isCritical ? kGuardCriticalCostRate : 1.0f);
		m_guardDurability = std::max(m_guardDurability - cost, 0.0f);
		m_guardRegenDelay = kGuardRegenDelay;

		if (m_hp <= 0)
		{
			ChangeState(std::make_shared<PlayerStateDead>());
		}
		else if (m_guardDurability <= 0.0f)
		{
			// ガードを崩された：しばらくガードできず、よろける
			m_guardBreakTimer = kGuardBreakFrame;
			HitStop::Request(kGuardBreakHitStopFrame, kGuardBreakShakePower);
			EffectPlayer::Play(EffectName::GuardBreak, CreateGuardEffectMatrix());
			SoundPlayer::PlaySE(SoundName::HeavyHit, 1.0f, kGuardBreakSEPitch);				// 盾が砕けた、重く低い音
			KdDebugGUI::Instance().AddLog("[System] ガードを崩された！\n");
			ChangeState(std::make_shared<PlayerStateGuardBreak>());
		}
		else
		{
			// 受け止めるアニメーションを再生する
			HitStop::Request(kGuardHitStopFrame, kGuardShakePower);
			ChangeState(std::make_shared<PlayerStateGuard>(PlayerStateGuard::Phase::Hit));
		}
		return;
	}

	m_hp -= damage;
	m_invincibleTimer = kDamageInvincibleFrame;
	HitStop::Request(kDamageHitStopFrame, kDamageShakePower);
	SoundPlayer::PlaySE(SoundName::HeavyHit, 1.0f, 0.0f, 0.05f);

	if (m_hp <= 0)
	{
		ChangeState(std::make_shared<PlayerStateDead>());
	}
	else if (isCritical || damage >= kHeavyDamage)
	{
		ChangeState(std::make_shared<PlayerStateDamage>());
	}
}

Math::Vector3 Player::GetCameraTargetPos() const
{
	// X・Z は足元の位置、Y は回避やジャンプで上下しない高さを使う
	// （前は腰の骨の高さを使っていたため、回避で腰が下がるとカメラがガクンと下がっていた）
	Math::Vector3 targetPos = m_pos;
	targetPos.y = m_cameraTargetY;
	return targetPos;
}
