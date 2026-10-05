#pragma once

#include"../BaseScene/BaseScene.h"

class Player;

class GameScene : public BaseScene
{
public :

	GameScene()  { Init(); }
	~GameScene() {}

private:

	void Event() override;
	void Init()  override;

	// オブジェクトを初期化してリストに追加する
	template<class T>
	std::shared_ptr<T> CreateObject()
	{
		std::shared_ptr<T> obj = std::make_shared<T>();
		obj->Init();
		m_objList.push_back(obj);
		return obj;
	}

	// 敵を生成して配置する
	void SpawnEnemy(const Math::Vector3& pos, bool isBoss, const std::shared_ptr<Player>& target);
};
