#include "SceneManager.h"

#include "BaseScene/BaseScene.h"
#include "TitleScene/TitleScene.h"
#include "GameScene/GameScene.h"
#include "GameClearScene/GameClearScene.h"
#include "GameOverScene/GameOverScene.h"

#include "../GameObject/Effect/EffectPlayer.h"
#include "../Utility/SoundPlayer.h"

// ============================================================
// 【役割】SceneManager の中身
// 【構成】ほとんどの関数は、今のシーンの同じ名前の関数を呼ぶだけ（処理を任せる）
//         ChangeScene だけが、前のシーンの後片付けと、次のシーンを作る仕事をする
// 【元のテンプレートから変えた所】
//   ・GameClear・GameOver のシーンを足した
//   ・切り替える時に、エフェクト・音・画面の色（HP が少ない時の白黒など）を元に戻すようにした
//   ・（.h）今のシーンの種類を返す GetCurrentSceneType を足し、起動時のシーンを Game から Title にした
// ============================================================

void SceneManager::PreUpdate()
{
	// シーン切替
	if (m_currentSceneType != m_nextSceneType)
	{
		ChangeScene(m_nextSceneType);
	}

	m_currentScene->PreUpdate();
}

void SceneManager::Update()
{
	m_currentScene->Update();
}

void SceneManager::PostUpdate()
{
	m_currentScene->PostUpdate();
}

void SceneManager::PreDraw()
{
	m_currentScene->PreDraw();
}

void SceneManager::Draw()
{
	m_currentScene->Draw();
}

void SceneManager::DrawSprite()
{
	m_currentScene->DrawSprite();
}

void SceneManager::DrawDebug()
{
	m_currentScene->DrawDebug();
}

const std::list<std::shared_ptr<KdGameObject>>& SceneManager::GetObjList()
{
	return m_currentScene->GetObjList();
}

void SceneManager::AddObject(const std::shared_ptr<KdGameObject>& _obj)
{
	m_currentScene->AddObject(_obj);
}

void SceneManager::ChangeScene(SceneType _sceneType)
{
	// 前のシーンのエフェクトが残らないように全て止める
	EffectPlayer::StopAll();

	// 前のシーンの音（BGM・効果音）も止める（BGM はシーンの Init で流し直す）
	SoundPlayer::StopAll();

	// HP が少ない時の画面の色（白黒・暗さ・周辺減光）を元に戻す
	KdShaderManager::Instance().m_postProcessShader.ResetScreenColor();

	// 次のシーンを作成し、現在のシーンにする
	// （make_shared で作ると、シーンのコンストラクタの中で Init が呼ばれ、オブジェクトが置かれる）
	switch (_sceneType)
	{
	case SceneType::Title:
		m_currentScene = std::make_shared<TitleScene>();
		break;
	case SceneType::Game:
		m_currentScene = std::make_shared<GameScene>();
		break;
	case SceneType::GameClear:
		m_currentScene = std::make_shared<GameClearScene>();
		break;
	case SceneType::GameOver:
		m_currentScene = std::make_shared<GameOverScene>();
		break;
	}

	// 現在のシーン情報を更新
	m_currentSceneType = _sceneType;
}
