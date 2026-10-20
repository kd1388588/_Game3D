#include "GameScene.h"
#include "../../GameObject/Effect/EffectPlayer.h"
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

// Effekseer関連
#include "../../../Framework/Effekseer/KdEffekseerManager.h"

namespace
{
	constexpr int kGameOverWaitFrame	= 60;	// 死亡モーション後、ゲームオーバー画面に移るまでのフレーム数
	constexpr int kClearWaitFrame		= 90;	// 敵が全滅してから、クリア画面に移るまでのフレーム数

	// デバッグキー（発表などで使わない時は false にする）
	constexpr bool	kEnableDebugKeys	= true;
	constexpr float	kDebugSpawnDistance	= 6.0f;		// F3 で敵を出す、プレイヤーの前方の距離

	// ボスが待っている位置（大聖堂の広間の真ん中。広間は z=52～100。Tools/MakeStageModel.py の NAVE_START_Z / NAVE_END_Z）
	const Math::Vector3 kBossPos		= { 0.0f, 0.0f, 76.0f };
}

void GameScene::Event()
{
	// デバッグ用：Tキーでタイトルへ
	if (GetAsyncKeyState('T') & 0x8000)
	{
		SceneManager::Instance().SetNextScene
		(
			SceneManager::SceneType::Title
		);
	}

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
	auto barrier = m_wpBarrier.lock();
	if (!barrier || barrier->IsOpen()) return;

	// ザコ敵を全員倒したら、ボスの広間のバリアを開ける
	for (const auto& obj : m_objList)
	{
		auto enemy = std::dynamic_pointer_cast<Enemy>(obj);
		if (enemy && !enemy->IsBoss() && enemy->IsAlive() && !enemy->IsExpired()) return;
	}
	barrier->Open();
	KdDebugGUI::Instance().AddLog("ボスの広間のバリアが開いた\n");
}

void GameScene::UpdateDebugKeys()
{
	// キーの状態は毎フレーム更新する（押した瞬間を正しく判定するため）
	bool isAtb = m_debugAtbKey.Update();
	bool isHp = m_debugHpKey.Update();
	bool isSpawn = m_debugSpawnKey.Update();
	bool isHpOne = m_debugHpOneKey.Update();
	if (!kEnableDebugKeys) return;

	auto player = m_wpPlayer.lock();
	if (!player) return;

	// F1：ATBゲージ全開
	if (isAtb)
	{
		player->DebugFillAtb();
		KdDebugGUI::Instance().AddLog("[Debug] ATBゲージ全開\n");
	}

	// F2：HP全開
	if (isHp)
	{
		player->DebugFillHp();
		KdDebugGUI::Instance().AddLog("[Debug] HP全開\n");
	}

	// F5：HPを1にする（あと1発で倒れる状態。ゲームオーバーの確認用）
	if (isHpOne)
	{
		player->DebugSetHpOne();
		KdDebugGUI::Instance().AddLog("[Debug] HPを1にした\n");
	}

	// F3：プレイヤーの前方にザコ敵を出す
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

	// 光・フォグ・影
	StageEnvironment::Apply();

	// ステージ
	CreateObject<Ground>();
	CreateObject<Sky>();
	// 廃墟の参道と崩れた大聖堂（Tools/MakeStageModel.py で作ったもの。壁や柱が厚く、すり抜けにくい）
	// ※前の大聖堂の廃墟（CathedralRuins）と重なるので、そちらは置かない
	//CreateObject<CathedralRuins>();
	CreateObject<RuinedRoad>();
	// ボスの広間の入口のバリア（ザコ敵を全員倒すと消える）
	m_wpBarrier = CreateObject<Barrier>();

	// プレイヤー（Init内でHPゲージを追加するため、先にオーナーをセットする）
	std::shared_ptr<Player> player = std::make_shared<Player>();
	player->SetOwner(this);
	player->Init();
	m_objList.push_back(player);
	m_wpPlayer = player;

	// 敵グループの配置
	const std::vector<Math::Vector3> enemyPosList =
	{
		// グループ1
		{  0.0f, 0.0f, 10.0f },
		//{ -3.0f, 0.0f, 12.0f },
		//{  3.0f, 0.0f, 12.0f },

		// グループ2
		{  0.0f, 0.0f, 25.0f },
		{ -4.0f, 0.0f, 27.0f },
		//{  4.0f, 0.0f, 27.0f },
	};

	for (const auto& pos : enemyPosList)
	{
		SpawnEnemy(pos, false, player);
	}

	// ボス：大聖堂の広間の真ん中で待っている（プレイヤーが近づくと動き出す）
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

void GameScene::SpawnEnemy(const Math::Vector3& pos, bool isBoss, const std::shared_ptr<Player>& target)
{
	std::shared_ptr<Enemy> enemy = std::make_shared<Enemy>();
	enemy->SetBoss(isBoss);
	enemy->Init();
	enemy->SetPos(pos);
	enemy->SetTarget(target);
	m_objList.push_back(enemy);
}
