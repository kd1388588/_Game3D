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

// テスト
#include <Application/GameObject/Character/TestModel/TestModel.h>

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
	// テスト
	//std::shared_ptr<TestModel> test = std::make_shared<TestModel>();
	//test->Init();
	//m_objList.push_back(test);
	//// ↑消していいよ

	std::shared_ptr<Ground> ground = std::make_shared<Ground>();
	ground->Init();
	m_objList.push_back(ground);

	std::shared_ptr<Sky> sky = std::make_shared<Sky>();
	sky->Init();
	m_objList.push_back(sky);

	std::shared_ptr<CathedralRuins> cathedralRuins = std::make_shared<CathedralRuins>();
	cathedralRuins->Init();
	m_objList.push_back(cathedralRuins);

	std::shared_ptr<Player> player = std::make_shared<Player>();
	player->SetOwner(this);
	player->Init();
	m_objList.push_back(player);

	// 敵グループの配置
	std::vector<Math::Vector3> group1Pos = {
		{  0.0f, 0.0f, 10.0f },
		//{ -3.0f, 0.0f, 12.0f },
		//{  3.0f, 0.0f, 12.0f }
	};

	for (size_t i = 0; i < group1Pos.size(); ++i)
	{
		std::shared_ptr<Enemy> enemy = std::make_shared<Enemy>();
		enemy->SetBoss(false); 
		enemy->Init();         
		enemy->SetPos(group1Pos[i]);
		enemy->SetTarget(player);
		m_objList.push_back(enemy);
	}

	std::vector<Math::Vector3> group2Pos = {
		{  0.0f, 0.0f, 25.0f },
		{ -4.0f, 0.0f, 27.0f },
		//{  4.0f, 0.0f, 27.0f }
	};

	for (size_t i = 0; i < group2Pos.size(); ++i)
	{
		std::shared_ptr<Enemy> enemy = std::make_shared<Enemy>();
		enemy->SetBoss(false); 
		enemy->Init();         
		enemy->SetPos(group2Pos[i]);
		enemy->SetTarget(player);
		m_objList.push_back(enemy);
	}

	// ボス
	std::shared_ptr<Enemy> boss = std::make_shared<Enemy>();
	boss->SetBoss(true); 
	boss->Init();        
	boss->SetPos({ 7.5f, 15.0f, 75.0f });
	boss->SetTarget(player);
	m_objList.push_back(boss);

	std::shared_ptr<TPSCamera> camera = std::make_shared<TPSCamera>();
	camera->Init();
	m_objList.push_back(camera);
	player->SetCamera(camera);
	camera->SetTarget(player);
	KdEffekseerManager::GetInstance().SetCamera(camera->GetCamera());
}