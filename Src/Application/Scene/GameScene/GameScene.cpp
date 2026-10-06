#include "GameScene.h"
#include "../SceneManager.h"

// Camera関連
#include "../../GameObject/Camera/TPSCamera/TPSCamera.h"

// Chara関連
#include "../../GameObject/Character/Player/Player.h"
#include "../../GameObject/Character/Enemy/Enemy.h"

// Stage関連
#include "../../GameObject/Stage/Ground/Ground.h"
#include "../../GameObject/Stage/Sky/Sky.h"
#include "../../GameObject/Stage/Object/CathedralRuins/CathedralRuins.h"

// Effekseer関連
#include "../../../Framework/Effekseer/KdEffekseerManager.h"

namespace
{
	constexpr int kGameOverWaitFrame	= 60;	// 死亡モーション後、ゲームオーバー画面に移るまでのフレーム数
	constexpr int kClearWaitFrame		= 90;	// 敵が全滅してから、クリア画面に移るまでのフレーム数
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

	CheckGameEnd();
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
	// ステージ
	CreateObject<Ground>();
	CreateObject<Sky>();
	CreateObject<CathedralRuins>();

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

	// ボス
	SpawnEnemy({ 7.5f, 15.0f, 75.0f }, true, player);

	// カメラ
	std::shared_ptr<TPSCamera> camera = CreateObject<TPSCamera>();
	player->SetCamera(camera);
	camera->SetTarget(player);
	KdEffekseerManager::GetInstance().SetCamera(camera->GetCamera());
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
