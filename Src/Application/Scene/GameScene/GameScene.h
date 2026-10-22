#pragma once

#include"../BaseScene/BaseScene.h"
#include"../../Utility/InputHelper.h"

class Player;
class Barrier;

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

	// クリア・ゲームオーバーの判定（条件を満たしてから少し待って画面を切り替える）
	void CheckGameEnd();

	// ザコ敵を全員倒したら、ボスの広間のバリアを開ける
	void UpdateBarrier();

	// 生き残っている敵の数
	int GetAliveEnemyCount() const;

	// デバッグキー（F1：ATB全開、F2：HP全開、F3：敵をプレイヤーの前に出す、F5：HPを1にする、F6：無敵の切り替え、F7：ボスの手前へ移動）
	// ※F4 はデバッグ表示の切り替え（main.cpp）
	void UpdateDebugKeys();
	InputHelper::KeyTrigger	m_debugAtbKey{ VK_F1 };
	InputHelper::KeyTrigger	m_debugHpKey{ VK_F2 };
	InputHelper::KeyTrigger	m_debugSpawnKey{ VK_F3 };
	InputHelper::KeyTrigger	m_debugHpOneKey{ VK_F5 };
	InputHelper::KeyTrigger	m_debugInvincibleKey{ VK_F6 };
	InputHelper::KeyTrigger	m_debugWarpKey{ VK_F7 };

	std::weak_ptr<Player>	m_wpPlayer;
	std::weak_ptr<Barrier>	m_wpBarrier;
	int						m_gameOverTimer = 0;	// プレイヤーが倒れてからの経過フレーム
	int						m_clearTimer = 0;		// 敵が全滅してからの経過フレーム
};
