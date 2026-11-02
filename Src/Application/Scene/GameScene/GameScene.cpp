#include "GameScene.h"
#include "../../GameObject/Effect/EffectPlayer.h"
#include "../../Utility/SoundPlayer.h"
#include "../SceneManager.h"
#include "../../Utility/StageEnvironment.h"

// Camera関連
#include "../../GameObject/Camera/TPSCamera/TPSCamera.h"

// Chara関連
#include "../../GameObject/Character/Player/Player.h"
#include "../../GameObject/Character/Enemy/Enemy.h"

// UI関連
#include "../../GameObject/UI/ControlsGuide/ControlsGuide.h"

// Stage関連
#include "../../GameObject/Stage/Ground/Ground.h"
#include "../../GameObject/Stage/Sky/Sky.h"
#include "../../GameObject/Stage/Object/CathedralRuins/CathedralRuins.h"
#include "../../GameObject/Stage/Object/RuinedRoad/RuinedRoad.h"
#include "../../GameObject/Stage/Object/Barrier/Barrier.h"
#include "../../GameObject/Stage/Object/StageLights/StageLights.h"

// Effekseer関連
#include "../../../Framework/Effekseer/KdEffekseerManager.h"

namespace
{
	constexpr int kGameOverWaitFrame	= 60;	// 死亡モーション後、ゲームオーバー画面に移るまでのフレーム数
	constexpr int kClearWaitFrame		= 90;	// 敵が全滅してから、クリア画面に移るまでのフレーム数

	// デバッグキー（発表などで使わない時は false にする）
	constexpr bool	kEnableDebugKeys	= true;
	constexpr float	kDebugSpawnDistance	= 6.0f;		// Shift ＋ F9 で敵を出す、プレイヤーの前方の距離
	constexpr float	kDebugWarpBossDistance	= 12.0f;	// Shift ＋ F11 でボスの手前のどこへ移動するか（ボスから入口の方へ）

	// ステージの部屋の並び（ゲームの中の z。Tools/MakeStageModel.py の値 × ステージの拡大率 1.25）
	//   -15～57.5 城の中の道 ／ 57.5～70 門楼 ／ 70～115 中庭 ／ 115～175 大聖堂の広間 ／ 175～215 玉座の間

	// バリア（置く場所：下の真ん中、大きさ：幅・高さ）
	const Math::Vector3 kGateBarrierPos			= { 0.0f, 0.0f, 57.5f };	// 門楼の入口（城の中の道の敵を全滅させると開く）
	const Math::Vector2 kGateBarrierSize		= { 13.5f, 9.0f };
	const Math::Vector3 kCathedralBarrierPos	= { 0.0f, 0.0f, 115.0f };	// 大聖堂の入口（中庭の敵を全滅させると開く）
	const Math::Vector2 kCathedralBarrierSize	= { 23.0f, 10.0f };

	// ボスが待っている位置（玉座の間。玉座の台の手前）
	const Math::Vector3 kBossPos		= { 0.0f, 0.0f, 194.0f };

	// 敵の配置
	const std::vector<Math::Vector3> kRoadEnemies =		// 城の中の道
	{
		{  0.0f, 0.0f, 10.0f },
		{  0.0f, 0.0f, 25.0f },
		{ -4.0f, 0.0f, 27.0f },
		{  5.0f, 0.0f, 42.0f },
	};
	const std::vector<Math::Vector3> kCourtyardEnemies =	// 中庭
	{
		{ -7.0f, 0.0f, 86.0f },
		{  7.0f, 0.0f, 88.0f },
		{  0.0f, 0.0f, 101.0f },
	};
}

void GameScene::Event()
{
	// 影はプレイヤーのまわりに作る（カメラのまわりだと、カメラを引いた時にプレイヤーの影が範囲から外れる）
	if (auto player = m_wpPlayer.lock())
	{
		StageEnvironment::SetShadowCenter(player->GetPos());
	}

	UpdateDebugKeys();
	UpdateBarrier();
	CheckGameEnd();
}

void GameScene::UpdateBarrier()
{
	// 部屋ごとに、手前の敵を全員倒したら、その先のバリアを開ける
	for (Gate& gate : m_gates)
	{
		auto barrier = gate.barrier.lock();
		if (!barrier || barrier->IsOpen()) continue;

		bool isAllDead = true;
		for (const auto& wpEnemy : gate.enemies)
		{
			auto enemy = wpEnemy.lock();
			if (enemy && enemy->IsAlive() && !enemy->IsExpired()) { isAllDead = false; break; }
		}
		if (!isAllDead) continue;

		barrier->Open();
		KdDebugGUI::Instance().AddLog("%s のバリアが開いた\n", gate.name.c_str());
	}
}

void GameScene::UpdateDebugKeys()
{
	// キーの状態は毎フレーム更新する（押した瞬間を正しく判定するため）
	bool isAtb = m_debugAtbKey.Update();
	bool isHp = m_debugHpKey.Update();
	bool isSpawn = m_debugSpawnKey.Update();
	bool isHpOne = m_debugHpOneKey.Update();
	bool isInvincible = m_debugInvincibleKey.Update();
	bool isWarp = m_debugWarpKey.Update();
	bool isTitle = m_debugTitleKey.Update();
	if (!kEnableDebugKeys) return;

	// デバッグ表示（F12）を出している間だけ効く（遊んでいる時にうっかり押しても何も起きないように）
	if (!KdDebugGUI::Instance().IsVisible()) return;

	// Shift ＋ F12：タイトルへ
	if (isTitle)
	{
		SceneManager::Instance().SetNextScene(SceneManager::SceneType::Title);
		return;
	}

	auto player = m_wpPlayer.lock();
	if (!player) return;

	// F9：ATBゲージ全開
	if (isAtb)
	{
		player->DebugFillAtb();
		KdDebugGUI::Instance().AddLog("[Debug] ATBゲージ全開\n");
	}

	// F10：HP全開
	if (isHp)
	{
		player->DebugFillHp();
		KdDebugGUI::Instance().AddLog("[Debug] HP全開\n");
	}

	// F11：HPを1にする（あと1発で倒れる状態。ゲームオーバーの確認用）
	if (isHpOne)
	{
		player->DebugSetHpOne();
		KdDebugGUI::Instance().AddLog("[Debug] HPを1にした\n");
	}

	// Shift ＋ F10：無敵の切り替え（もう一度押すと元に戻る）
	if (isInvincible)
	{
		bool on = player->DebugToggleInvincible();
		KdDebugGUI::Instance().AddLog(on ? "[Debug] 無敵 ON\n" : "[Debug] 無敵 OFF\n");
	}

	// Shift ＋ F11：ボス以外の敵を全員消し（バリアも開く）、ボスの手前（広間の中）へ瞬間移動して、ボスの方を向く
	if (isWarp)
	{
		Math::Vector3 pos = kBossPos;
		for (const auto& obj : m_objList)
		{
			auto enemy = std::dynamic_pointer_cast<Enemy>(obj);
			if (!enemy || !enemy->IsAlive()) continue;
			if (enemy->IsBoss())	pos = enemy->GetPos();
			else					enemy->DebugKill();
		}
		pos.z -= kDebugWarpBossDistance;
		player->DebugWarp(pos, 0.0f);
		KdDebugGUI::Instance().AddLog("[Debug] ボス以外の敵を倒して、ボスの手前へ移動\n");
	}

	// Shift ＋ F9：プレイヤーの前方にザコ敵を出す
	if (isSpawn)
	{
		Math::Matrix rot = Math::Matrix::CreateRotationY(player->GetAngleRad());
		Math::Vector3 forward = Math::Vector3::TransformNormal(Math::Vector3(0.0f, 0.0f, 1.0f), rot);
		Math::Vector3 pos = player->GetPos() + forward * kDebugSpawnDistance;
		pos.y = player->GetPos().y;
		SpawnEnemy(pos, false, player);
		KdDebugGUI::Instance().AddLog("[Debug] 敵をスポーン\n");
	}
}

void GameScene::CheckGameEnd()
{
	// ゲームオーバー：プレイヤーが倒れて、死亡モーションが終わってから少し待つ
	auto player = m_wpPlayer.lock();
	if (player && player->IsDead())
	{
		if (player->IsAnimEnd())
		{
			m_gameOverTimer++;
		}

		if (m_gameOverTimer >= kGameOverWaitFrame)
		{
			SceneManager::Instance().SetNextScene(SceneManager::SceneType::GameOver);
		}
		return;
	}

	// ゲームクリア：全ての敵を倒してから少し待つ
	if (GetAliveEnemyCount() == 0)
	{
		m_clearTimer++;
		if (m_clearTimer >= kClearWaitFrame)
		{
			SceneManager::Instance().SetNextScene(SceneManager::SceneType::GameClear);
		}
	}
}

int GameScene::GetAliveEnemyCount() const
{
	int count = 0;
	for (const auto& obj : m_objList)
	{
		// 死亡モーションが終わった敵は IsExpired() が true になる
		if (!obj->IsExpired() && std::dynamic_pointer_cast<Enemy>(obj))
		{
			count++;
		}
	}
	return count;
}

void GameScene::Init()
{
	// エフェクトを出す（タイトル画面では止めているので、ここで戻す）
	EffectPlayer::SetEnabled(true);

	// BGM
	SoundPlayer::PlayBGM(SoundName::GameBGM);

	// 光・フォグ・影
	StageEnvironment::Apply();

	// ステージ
	CreateObject<Ground>();
	CreateObject<Sky>();
	// 廃墟の参道と崩れた大聖堂（Tools/MakeStageModel.py で作ったもの。壁や柱が厚く、すり抜けにくい）
	// ※前の大聖堂の廃墟（CathedralRuins）と重なるので、そちらは置かない
	//CreateObject<CathedralRuins>();
	CreateObject<RuinedRoad>();
	// 松明・かがり火・燭台の光（点光源と炎の光）
	CreateObject<StageLights>();

	// 部屋の入口のバリア（門楼・大聖堂）
	auto createBarrier = [this](const Math::Vector3& pos, const Math::Vector2& size)
	{
		std::shared_ptr<Barrier> barrier = std::make_shared<Barrier>();
		barrier->SetArea(pos, size);
		barrier->Init();
		m_objList.push_back(barrier);
		return barrier;
	};
	std::shared_ptr<Barrier> gateBarrier = createBarrier(kGateBarrierPos, kGateBarrierSize);
	std::shared_ptr<Barrier> cathedralBarrier = createBarrier(kCathedralBarrierPos, kCathedralBarrierSize);

	// プレイヤー（Init内でHPゲージを追加するため、先にオーナーをセットする）
	std::shared_ptr<Player> player = std::make_shared<Player>();
	player->SetOwner(this);
	player->Init();
	m_objList.push_back(player);
	m_wpPlayer = player;

	// 敵の配置（部屋ごと。その部屋の敵を全員倒すと、先のバリアが開く）
	Gate gate;
	gate.barrier = gateBarrier;
	gate.name = "門楼";
	for (const auto& pos : kRoadEnemies) gate.enemies.push_back(SpawnEnemy(pos, false, player));
	m_gates.push_back(gate);

	Gate cathedralGate;
	cathedralGate.barrier = cathedralBarrier;
	cathedralGate.name = "大聖堂";
	for (const auto& pos : kCourtyardEnemies) cathedralGate.enemies.push_back(SpawnEnemy(pos, false, player));
	m_gates.push_back(cathedralGate);

	// ボス：一番奥の玉座の間で待っている（プレイヤーが近づくと動き出す）
	SpawnEnemy(kBossPos, true, player);

	// カメラ
	std::shared_ptr<TPSCamera> camera = CreateObject<TPSCamera>();
	player->SetCamera(camera);
	camera->SetTarget(player);
	KdEffekseerManager::GetInstance().SetCamera(camera->GetCamera());

	// 右下の操作キーの一覧
	std::shared_ptr<ControlsGuide> controlsGuide = CreateObject<ControlsGuide>();
	controlsGuide->SetPlayer(player);
}

std::shared_ptr<Enemy> GameScene::SpawnEnemy(const Math::Vector3& pos, bool isBoss, const std::shared_ptr<Player>& target)
{
	std::shared_ptr<Enemy> enemy = std::make_shared<Enemy>();
	enemy->SetBoss(isBoss);
	enemy->Init();
	enemy->SetPos(pos);
	enemy->SetTarget(target);
	m_objList.push_back(enemy);
	return enemy;
}
