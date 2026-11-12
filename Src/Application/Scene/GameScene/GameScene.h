#pragma once

#include"../BaseScene/BaseScene.h"
#include"../../Utility/InputHelper.h"

class Player;
class Barrier;
class Enemy;

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
	std::shared_ptr<Enemy> SpawnEnemy(const Math::Vector3& pos, bool isBoss, const std::shared_ptr<Player>& target);

	// クリア・ゲームオーバーの判定（条件を満たしてから少し待って画面を切り替える）
	void CheckGameEnd();

	// 部屋ごとの敵を全員倒したら、その先のバリアを開ける
	void UpdateBarrier();

	// バリアと、それを開けるために倒す敵のまとまり
	struct Gate
	{
		std::weak_ptr<Barrier>				barrier;
		std::vector<std::weak_ptr<Enemy>>	enemies;
		std::string							name;
	};
	std::vector<Gate>	m_gates;

	// 生き残っている敵の数
	int GetAliveEnemyCount() const;

	// デバッグキー（F8 でデバッグ表示を出している間だけ効く）
	// F7：ボス以外の敵を全員倒す（その場で）、F9：ATB全開、F10：HP全開、F11：HPを1にする
	// Shift ＋ F9：敵をプレイヤーの前に出す、Shift ＋ F10：無敵の切り替え、Shift ＋ F11：ボス以外を消してボスの手前へ移動、Shift ＋ F8：タイトルへ
	// ※F8 はデバッグ表示の切り替え（main.cpp）。F12 は Visual Studio のデバッグ中に「止める」キーになるので使わない
	void UpdateDebugKeys();
	InputHelper::KeyTrigger	m_debugAtbKey{ InputHelper::Bind::DebugAtb };
	InputHelper::KeyTrigger	m_debugHpKey{ InputHelper::Bind::DebugHpFull };
	InputHelper::KeyTrigger	m_debugSpawnKey{ InputHelper::Bind::DebugSpawn };
	InputHelper::KeyTrigger	m_debugHpOneKey{ InputHelper::Bind::DebugHpOne };
	InputHelper::KeyTrigger	m_debugInvincibleKey{ InputHelper::Bind::DebugInvincible };
	InputHelper::KeyTrigger	m_debugWarpKey{ InputHelper::Bind::DebugWarpBoss };
	InputHelper::KeyTrigger	m_debugTitleKey{ InputHelper::Bind::DebugToTitle };
	InputHelper::KeyTrigger	m_debugKillNormalKey{ InputHelper::Bind::DebugKillNormal };

	std::weak_ptr<Player>	m_wpPlayer;
	int						m_gameOverTimer = 0;	// プレイヤーが倒れてからの経過フレーム
	int						m_clearTimer = 0;		// 敵が全滅してからの経過フレーム
};
