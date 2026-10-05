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

void GameScene::Event()
{
	if (GetAsyncKeyState('T') & 0x8000)
	{
		SceneManager::Instance().SetNextScene
		(
			SceneManager::SceneType::Title
		);
	}
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
