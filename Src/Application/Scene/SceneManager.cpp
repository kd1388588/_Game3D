#include "SceneManager.h"

#include "BaseScene/BaseScene.h"
#include "TitleScene/TitleScene.h"

void SceneManager::PreUpdate()
{
	if (m_currentSceneType != m_nextSceneType)
	{
		ChangeScene(m_nextSceneType);
	}

	m_currentScene->PreUpdate();
}

void SceneManager::Update() { m_currentScene->Update(); }

void SceneManager::PostUpdate() { m_currentScene->PostUpdate(); }

void SceneManager::PreDraw() { m_currentScene->PreDraw(); }

void SceneManager::Draw() { m_currentScene->Draw(); }

void SceneManager::DrawSprite() { m_currentScene->DrawSprite(); }

void SceneManager::DrawDebug() { m_currentScene->DrawDebug(); }

const std::list<std::shared_ptr<KdGameObject>>& SceneManager::GetObjlist()
{
	return m_currentScene->GetObjList();
}

void SceneManager::AddObject(const std::shared_ptr<KdGameObject>& obj)
{
	m_currentScene->AddObject(obj);
}

void SceneManager::ChangeScene(SceneType sceneType)
{
	switch (sceneType)
	{
	case SceneManager::Title:
		m_currentScene = std::make_shared<TitleScene>();
		break;
	case SceneManager::Game:
		break;
	case SceneManager::GameClear:
		break;
	case SceneManager::GameOver:
		break;
	default:
		break;
	}

	m_currentSceneType = sceneType;
}
