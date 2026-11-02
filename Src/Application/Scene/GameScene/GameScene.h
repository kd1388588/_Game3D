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

	// デバッグキー（F12 でデバッグ表示を出している間だけ効く）
	// F9：ATB全開、F10：HP全開、F11：HPを1にする
	// Shift ＋ F9：敵をプレイヤーの前に出す、Shift ＋ F10：無敵の切り替え、Shift ＋ F11：ボス以外を消してボスの手前へ移動、Shift ＋ F12：タイトルへ
	// ※F12 はデバッグ表示の切り替え（main.cpp）
	void UpdateDebugKeys();
	InputHelper::KeyTrigger	m_debugAtbKey{ InputHelper::Bind::DebugAtb };
	InputHelper::KeyTrigger	m_debugHpKey{ InputHelper::Bind::DebugHpFull };
	InputHelper::KeyTrigger	m_debugSpawnKey{ InputHelper::Bind::DebugSpawn };
	InputHelper::KeyTrigger	m_debugHpOneKey{ InputHelper::Bind::DebugHpOne };
	InputHelper::KeyTrigger	m_debugInvincibleKey{ InputHelper::Bind::DebugInvincible };
	InputHelper::KeyTrigger	m_debugWarpKey{ InputHelper::Bind::DebugWarpBoss };
	InputHelper::KeyTrigger	m_debugTitleKey{ InputHelper::Bind::DebugToTitle };

	std::weak_ptr<Player>	m_wpPlayer;
	std::weak_ptr<Barrier>	m_wpBarrier;
	int						m_gameOverTimer = 0;	// プレイヤーが倒れてからの経過フレーム
	int						m_clearTimer = 0;		// 敵が全滅してからの経過フレーム
};
