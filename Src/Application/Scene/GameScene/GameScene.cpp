#include "GameScene.h"
#include "../../GameObject/Effect/EffectPlayer.h"
#include "../../Utility/SoundPlayer.h"
#include "../SceneManager.h"
#include "../../Utility/StageEnvironment.h"

// Camera関連
#include "../../GameObject/Camera/TPSCamera/TPSCamera.h"

// Chara関連
#include "../../GameObject/Character/Player/Player.h"
#include "../../GameObject/Character/Enemy/Enemy.h"

// UI関連
#include "../../GameObject/UI/ControlsGuide/ControlsGuide.h"
#include "../../GameObject/UI/StageMessage/StageMessage.h"
#include "../../GameObject/Character/UI/Gage/Enemy/EnemyGage.h"

// Stage関連
#include "../../GameObject/Stage/Ground/Ground.h"
#include "../../GameObject/Stage/Sky/Sky.h"
#include "../../GameObject/Stage/Object/GroundMist/GroundMist.h"
#include "../../Utility/StageSettings.h"
#include "../../Utility/EffectSettings.h"
#include "../../GameObject/Stage/Object/CathedralRuins/CathedralRuins.h"
#include "../../GameObject/Stage/Object/RuinedRoad/RuinedRoad.h"
#include "../../GameObject/Stage/Object/Barrier/Barrier.h"
#include "../../GameObject/Stage/Object/StageLights/StageLights.h"
#include "../../GameObject/Stage/Object/KeyItem/KeyItem.h"
#include "../../GameObject/Stage/Object/HealFountain/HealFountain.h"

// Effekseer関連
#include "../../../Framework/Effekseer/KdEffekseerManager.h"

namespace
{
	constexpr int kGameOverWaitFrame	= 20;	// 倒れて画面が真っ黒になってから、ゲームオーバー画面に移るまでのフレーム数
	constexpr int kClearWaitFrame		= 90;	// 敵が全滅してから、クリア画面に移るまでのフレーム数

	// デバッグキー（発表などで使わない時は false にする）
	constexpr bool	kEnableDebugKeys	= true;
	constexpr float	kDebugSpawnDistance	= 6.0f;		// Shift ＋ F9 で敵を出す、プレイヤーの前方の距離
	constexpr float	kDebugWarpBossDistance	= 12.0f;	// Shift ＋ F11 でボスの手前のどこへ移動するか（ボスから入口の方へ）

	// ============================================================
	// ステージの部屋の並び（Tools/MakeCastleStage.py の図も見る。モデルの座標 × ステージの拡大率 1.25 ＝ ゲームの座標）
	//
	//                    B ボス ── 鍵の扉
	//    K 鍵の部屋 ═══════ T ════════ R 右の部屋
	//              H 真ん中 ═══ E ══╗     ║ V
	//    S スタート ═══╝            ╚═════╡
	//                                    M 中ボス（寄り道）║ F 泉の小部屋
	//
	//   S → H（敵なし）→ 分かれ道 → 上の R（全員倒すと左の出口が開く）
	//   → K 鍵の部屋（全員倒すと鍵が出る）→ 鍵を持って B の扉に近づくと開く → ボス
	//   寄り道：分かれ道の下の M 中ボス（全員倒すと、右の F の回復の泉が光る。倒さなくても先へ進める）
	// ============================================================
	constexpr float kStageScale = 1.25f;	// RuinedRoad.cpp の kScale と同じ

	// モデルの座標（x, z）を、ゲームの座標にする（Tools/MakeCastleStage.py の数字をそのまま書けるように）
	Math::Vector3 StagePos(float x, float z, float y = 0.0f)
	{
		return { x * kStageScale, y, z * kStageScale };
	}

	// プレイヤーが始まる所（スタートの部屋の下の方。奥（+Z）の入口を向く）
	const Math::Vector3 kPlayerStartPos			= StagePos(-26.0f, -3.0f);

	// バリア（置く場所：下の真ん中、大きさ：幅・高さ、向き：度）
	const Math::Vector3 kRightGateBarrierPos	= StagePos(24.0f, 59.0f);	// 右の部屋の左の出口（右の部屋の敵を全員倒すと開く）
	const Math::Vector3 kBossDoorPos			= StagePos(0.0f, 78.0f);	// ボスの部屋の入口（鍵を持って近づくと開く）
	const Math::Vector2 kDoorBarrierSize		= { 8.0f, 7.5f };			// どれも通路の幅（7.5m）より少し広く
	const Math::Vector3 kBossDoorColor			= { 1.0f, 0.72f, 0.28f };	// 鍵の扉は金色（普通のバリアは青白い）
	constexpr float		kBossDoorOpenRange		= 7.0f;		// 鍵を持ってこの距離まで近づくと、扉が開く
	constexpr float		kBossDoorLockedRange	= 6.0f;		// 鍵を持たずにこの距離まで近づくと、「鍵がかかっている」と出す
	constexpr int		kLockedMessageInterval	= 60 * 4;	// 「鍵がかかっている」を出した後、次に出せるまでの時間（4秒）

	// ボスの部屋に入ったら、入口をふさぐ（扉より奥へこの距離入ったら。入口に立っている時にふさがないように）
	constexpr float		kBossLockDepth			= 4.0f;
	const Math::Vector3	kBossLockColor			= { 1.0f, 0.3f, 0.25f };	// 赤（逃げられないことを伝える）

	// 真ん中の部屋（初めて入った時に、プレイヤーから遠い方の側に敵が 1 体わく）
	//   部屋の広さ：モデルの座標 x -12～12、z 16～36。わく所は、真ん中のかがり火の左右
	constexpr float		kHubMinX = -12.0f * kStageScale, kHubMaxX = 12.0f * kStageScale;
	constexpr float		kHubMinZ = 16.0f * kStageScale, kHubMaxZ = 36.0f * kStageScale;
	constexpr float		kHubEnterMargin			= 2.0f;		// 部屋の端から、これだけ中へ入ったら「入った」とする
	const Math::Vector3	kHubSpawnPosA			= StagePos(-7.0f, 26.0f);
	const Math::Vector3	kHubSpawnPosB			= StagePos(7.0f, 26.0f);

	// 泉の小部屋の入口（仕切りの壁の南の端。中ボスの部屋の敵を全員倒すと開く）
	const Math::Vector3	kFountainBarrierPos		= StagePos(47.0f, -11.0f);

	// 鍵（鍵の部屋の真ん中の床の上に浮かぶ。ジャンプしなくても拾えるように、プレイヤーの胸くらいの高さ）
	const Math::Vector3 kKeyPos					= StagePos(-34.0f, 58.0f, 1.3f);

	// 回復の泉（中ボスの部屋の右の、泉の小部屋の真ん中。中ボスの部屋の敵を全員倒すと光り、
	// 近づくと 1 度だけ HP と ATB を全回復。通路からは仕切りの壁で見えない）
	const Math::Vector3 kFountainPos			= StagePos(53.0f, -3.0f);

	// 敵の配置
	const Math::Vector3 kMidBossPos				= StagePos(30.0f, -4.0f);	// 中ボス
	const std::vector<Math::Vector3> kMidRoomEnemies =		// 中ボスの部屋のザコ
	{
		StagePos(22.0f, 0.0f),
		StagePos(38.0f, -9.0f),
	};
	const std::vector<Math::Vector3> kRightRoomEnemies =	// 右の部屋
	{
		StagePos(31.0f, 54.0f),
		StagePos(38.0f, 60.0f),
		StagePos(33.0f, 65.0f),
		StagePos(41.0f, 57.0f),
	};
	const std::vector<Math::Vector3> kKeyRoomEnemies =		// 鍵の部屋（全員倒すと鍵が出る）
	{
		StagePos(-34.0f, 51.5f),
		StagePos(-34.0f, 64.5f),
		StagePos(-41.0f, 58.0f),
		StagePos(-27.0f, 58.0f),
	};
	const std::vector<Math::Vector3> kBossRoomEnemies =		// ボスの部屋のザコ（入口の近く）
	{
		StagePos(-8.0f, 84.0f),
		StagePos(8.0f, 84.0f),
	};

	// ボスが待っている位置（ボスの部屋。玉座の手前）
	const Math::Vector3 kBossPos				= StagePos(0.0f, 96.0f);
}

// ============================================================
// 【役割】ゲーム本編のシーン
// 【構成】Init：光・霧 → ステージ → バリア → プレイヤー → 敵 → カメラ → 操作ガイド の順に置く
//         Event（毎フレーム）：霧を反映 → 影の中心 → デバッグキー → バリアを開ける → クリア・ゲームオーバーの判定
// 【なぜこの順番で置くのか】
//   ・カメラはプレイヤーを注視するので、プレイヤーの後に作る
//   ・オブジェクトはリストの順に更新・描画される。カメラは最後の方に置くと、全員の PostUpdate の後に位置が決まる
// ============================================================
void GameScene::Event()
{
	// 霧（ステージエディタで変えた値を、毎フレームすぐに反映する）
	StageSettings::Instance().ApplyFog();

	// 影はプレイヤーのまわりに作る（カメラのまわりだと、カメラを引いた時にプレイヤーの影が範囲から外れる）
	if (auto player = m_wpPlayer.lock())
	{
		StageEnvironment::SetShadowCenter(player->GetPos());
	}

	UpdateDebugKeys();
	UpdateBarrier();
	UpdateHubSpawn();
	UpdateFountain();
	UpdateKey();
	CheckGameEnd();
}

void GameScene::UpdateBarrier()
{
	// 部屋ごとに、手前の敵を全員倒したら、その先のバリアを開ける
	for (Gate& gate : m_gates)
	{
		auto barrier = gate.barrier.lock();
		if (!barrier || barrier->IsOpen()) continue;

		bool isAllDead = true;
		for (const auto& wpEnemy : gate.enemies)
		{
			auto enemy = wpEnemy.lock();
			if (enemy && enemy->IsAlive() && !enemy->IsExpired()) { isAllDead = false; break; }
		}
		if (!isAllDead) continue;

		barrier->Open();
		KdDebugGUI::Instance().AddLog("%s のバリアが開いた\n", gate.name.c_str());
		if (auto message = m_wpMessage.lock(); message && !gate.message.empty()) message->Show(gate.message);
	}
}

void GameScene::UpdateHubSpawn()
{
	if (m_isHubSpawned) return;
	auto player = m_wpPlayer.lock();
	if (!player || !player->IsAlive()) return;

	// 真ん中の部屋に入ったか（部屋の端から少し中へ入ったら）
	const Math::Vector3& pos = player->GetPos();
	bool isInside = pos.x > kHubMinX + kHubEnterMargin && pos.x < kHubMaxX - kHubEnterMargin
		&& pos.z > kHubMinZ + kHubEnterMargin && pos.z < kHubMaxZ - kHubEnterMargin;
	if (!isInside) return;

	// プレイヤーから遠い方に、敵を 1 体わかせる（目の前に急に出ないように）
	float distA = (kHubSpawnPosA - pos).LengthSquared();
	float distB = (kHubSpawnPosB - pos).LengthSquared();
	SpawnEnemy(distA > distB ? kHubSpawnPosA : kHubSpawnPosB, false, player);
	m_isHubSpawned = true;
	KdDebugGUI::Instance().AddLog("真ん中の部屋に敵がわいた\n");
}

void GameScene::UpdateFountain()
{
	auto fountain = m_wpFountain.lock();
	if (!fountain) return;
	auto message = m_wpMessage.lock();

	// 中ボスの部屋の敵を全員倒したら、泉を光らせる
	if (fountain->IsSleeping())
	{
		bool isAllDead = true;
		for (const auto& wpEnemy : m_midRoomEnemies)
		{
			auto enemy = wpEnemy.lock();
			if (enemy && enemy->IsAlive() && !enemy->IsExpired()) { isAllDead = false; break; }
		}
		if (isAllDead)
		{
			fountain->Activate();
			if (auto barrier = m_wpFountainBarrier.lock()) barrier->Open();
			if (message) message->Show("泉への道が開いた");
		}
	}

	// 使われたら知らせる
	if (fountain->ConsumeUsed())
	{
		KdDebugGUI::Instance().AddLog("回復の泉で HP と ATB ゲージを全回復した\n");
		if (message) message->Show("泉の力で、HP と ATB ゲージが回復した");
	}
}

void GameScene::UpdateKey()
{
	auto player = m_wpPlayer.lock();
	auto message = m_wpMessage.lock();
	if (m_lockedMessageTimer > 0) m_lockedMessageTimer--;

	// ① 鍵の部屋の敵を全員倒したら、真ん中の台の上に鍵を出す
	if (!m_isKeySpawned && player)
	{
		bool isAllDead = true;
		for (const auto& wpEnemy : m_keyRoomEnemies)
		{
			auto enemy = wpEnemy.lock();
			if (enemy && enemy->IsAlive() && !enemy->IsExpired()) { isAllDead = false; break; }
		}
		if (isAllDead)
		{
			std::shared_ptr<KeyItem> key = std::make_shared<KeyItem>();
			key->SetPos(kKeyPos);
			key->SetTarget(player);
			key->Init();
			m_objList.push_back(key);
			m_wpKey = key;
			m_isKeySpawned = true;
			if (message) message->Show("鍵が現れた");
		}
	}

	// ② 鍵を拾ったか
	if (m_isKeySpawned && !m_hasKey)
	{
		auto key = m_wpKey.lock();
		if (!key || key->IsPicked())
		{
			m_hasKey = true;
			if (message)
			{
				message->Show("鍵を手に入れた");
				message->SetHasKey(true);
			}
		}
	}

	// ③ ボスの部屋に入ったら、後ろの入口を赤いバリアでふさぐ（ボスを倒すまで逃げられない）
	if (m_isBossDoorOpened && !m_isBossRoomLocked && player && player->GetPos().z > kBossDoorPos.z + kBossLockDepth)
	{
		std::shared_ptr<Barrier> lock = std::make_shared<Barrier>();
		lock->SetArea(kBossDoorPos, kDoorBarrierSize, 0.0f);
		lock->SetColor(kBossLockColor);
		lock->Init();
		m_objList.push_back(lock);
		m_isBossRoomLocked = true;
		if (message) message->Show("もう逃げられない");
	}

	// ④ ボスの部屋の扉：鍵を持って近づくと開く。持たずに近づくと「鍵がかかっている」
	auto door = m_wpBossDoor.lock();
	if (!door || door->IsOpen() || !player) return;

	Math::Vector3 diff = player->GetPos() - door->GetBasePos();
	diff.y = 0.0f;
	float dist = diff.Length();
	if (m_hasKey && dist <= kBossDoorOpenRange)
	{
		door->Open();
		m_isBossDoorOpened = true;
		KdDebugGUI::Instance().AddLog("ボスの部屋の扉が開いた\n");
		if (message)
		{
			message->Show("鍵を使った。扉が開いた");
			message->SetHasKey(false);
		}
	}
	else if (!m_hasKey && dist <= kBossDoorLockedRange && m_lockedMessageTimer <= 0)
	{
		m_lockedMessageTimer = kLockedMessageInterval;
		if (message) message->Show("扉には鍵がかかっている");
	}
}

void GameScene::UpdateDebugKeys()
{
	// キーの状態は毎フレーム更新する（押した瞬間を正しく判定するため）
	bool isAtb = m_debugAtbKey.Update();
	bool isHp = m_debugHpKey.Update();
	bool isSpawn = m_debugSpawnKey.Update();
	bool isHpOne = m_debugHpOneKey.Update();
	bool isInvincible = m_debugInvincibleKey.Update();
	bool isWarp = m_debugWarpKey.Update();
	bool isTitle = m_debugTitleKey.Update();
	bool isKillNormal = m_debugKillNormalKey.Update();
	if (!kEnableDebugKeys) return;

	// デバッグ表示（F8）を出している間だけ効く（遊んでいる時にうっかり押しても何も起きないように）
	if (!KdDebugGUI::Instance().IsVisible()) return;

	// Shift ＋ F8：タイトルへ
	if (isTitle)
	{
		SceneManager::Instance().SetNextScene(SceneManager::SceneType::Title);
		return;
	}

	auto player = m_wpPlayer.lock();
	if (!player) return;

	// F9：ATBゲージ全開
	if (isAtb)
	{
		player->DebugFillAtb();
		KdDebugGUI::Instance().AddLog("[Debug] ATBゲージ全開\n");
	}

	// F10：HP全開
	if (isHp)
	{
		player->DebugFillHp();
		KdDebugGUI::Instance().AddLog("[Debug] HP全開\n");
	}

	// F11：HPを1にする（あと1発で倒れる状態。ゲームオーバーの確認用）
	if (isHpOne)
	{
		player->DebugSetHpOne();
		KdDebugGUI::Instance().AddLog("[Debug] HPを1にした\n");
	}

	// Shift ＋ F10：無敵の切り替え（もう一度押すと元に戻る）
	if (isInvincible)
	{
		bool on = player->DebugToggleInvincible();
		KdDebugGUI::Instance().AddLog(on ? "[Debug] 無敵 ON\n" : "[Debug] 無敵 OFF\n");
	}

	// Shift ＋ F11：ボス以外の敵を全員消し（バリアも開く）、ボスの手前（広間の中）へ瞬間移動して、ボスの方を向く
	if (isWarp)
	{
		Math::Vector3 pos = kBossPos;
		for (const auto& obj : m_objList)
		{
			auto enemy = std::dynamic_pointer_cast<Enemy>(obj);
			if (!enemy || !enemy->IsAlive()) continue;
			if (enemy->IsBoss())	pos = enemy->GetPos();
			else					enemy->DebugKill();
		}
		pos.z -= kDebugWarpBossDistance;
		player->DebugWarp(pos, 0.0f);
		if (auto door = m_wpBossDoor.lock()) door->Open();	// ボスの部屋の扉も開けておく
		m_isBossDoorOpened = true;
		m_isBossRoomLocked = true;	// デバッグで移動した時は、入口をふさがない（部屋から出られるように）
		KdDebugGUI::Instance().AddLog("[Debug] ボス以外の敵を倒して、ボスの手前へ移動\n");
	}

	// F7：ボス以外の敵を全員倒す（その場で。プレイヤーは移動しない。部屋のバリアもすべて開く）
	// （Shift ＋ F11 と同じ倒し方だが、ボスの手前へは移動しない）
	if (isKillNormal)
	{
		int count = 0;
		for (const auto& obj : m_objList)
		{
			auto enemy = std::dynamic_pointer_cast<Enemy>(obj);
			if (!enemy || !enemy->IsAlive() || enemy->IsBoss()) continue;
			enemy->DebugKill();
			++count;
		}
		KdDebugGUI::Instance().AddLog("[Debug] ボス以外の敵を %d 体倒した\n", count);
	}

	// Shift ＋ F9：プレイヤーの前方にザコ敵を出す
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
	// ゲームオーバー：プレイヤーが倒れて、画面が真っ黒になってから少し待つ
	// （真っ黒にするのは Player。かかる時間はステージエディタの「画面」タブの「倒れて真っ黒になるまで」）
	auto player = m_wpPlayer.lock();
	if (player && player->IsDead())
	{
		if (player->IsDeathFadeEnd())
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

	// BGM
	SoundPlayer::PlayBGM(SoundName::GameBGM);

	// 光・フォグ・影
	StageEnvironment::Apply(true);	// ゲームはタイトルより明るくする
	StageSettings::Instance().Load();	// ステージエディタで保存した値（霧など）があれば読み込む
	EffectSettings::Instance().OnGameStart();	// エフェクトエディタで保存した値を読み込む（設定によっては Effekseer との同期も始める）

	// ステージ
	CreateObject<Ground>();
	CreateObject<Sky>();
	// 夜の廃墟の城（Tools/MakeCastleStage.py で作ったもの。6 つの部屋を通路でつなぐ。壁は厚く、すり抜けにくい）
	// ※前の大聖堂の廃墟（CathedralRuins）と重なるので、そちらは置かない
	//CreateObject<CathedralRuins>();
	CreateObject<RuinedRoad>();
	// 松明・かがり火・燭台の光（点光源と炎の光）
	CreateObject<StageLights>();
	// 地面の霧（濃さなどはステージエディタで調整する）
	CreateObject<GroundMist>();

	// 部屋の入口のバリア（向き：0 度は奥へ進む通路、90 度は横へ進む通路をふさぐ）
	auto createBarrier = [this](const Math::Vector3& pos, const Math::Vector2& size, float yawDeg)
	{
		std::shared_ptr<Barrier> barrier = std::make_shared<Barrier>();
		barrier->SetArea(pos, size, yawDeg);
		barrier->Init();
		m_objList.push_back(barrier);
		return barrier;
	};
	std::shared_ptr<Barrier> rightGateBarrier = createBarrier(kRightGateBarrierPos, kDoorBarrierSize, 90.0f);
	std::shared_ptr<Barrier> bossDoor = createBarrier(kBossDoorPos, kDoorBarrierSize, 0.0f);
	bossDoor->SetColor(kBossDoorColor);
	m_wpBossDoor = bossDoor;

	// プレイヤー（Init内でHPゲージを追加するため、先にオーナーをセットする）
	std::shared_ptr<Player> player = std::make_shared<Player>();
	player->SetOwner(this);
	player->Init();
	// 始まる所へ置く（DebugWarp は、位置・向き・前のフレームの位置をまとめて決めるので、
	// 壁のすり抜け防止の処理で、元の位置から引き戻されない）
	player->DebugWarp(kPlayerStartPos, 0.0f);
	m_objList.push_back(player);
	m_wpPlayer = player;

	// 敵の配置（部屋ごと。その部屋の敵を全員倒すと、先のバリアが開く）
	// 泉の小部屋の入口（横の向きの通路なので 90 度）
	m_wpFountainBarrier = createBarrier(kFountainBarrierPos, kDoorBarrierSize, 90.0f);

	// 中ボスの部屋（寄り道。全員倒すと泉の小部屋の入口が開き、泉が光る。UpdateFountain）
	m_midRoomEnemies.push_back(SpawnEnemy(kMidBossPos, false, player, true));
	for (const auto& pos : kMidRoomEnemies) m_midRoomEnemies.push_back(SpawnEnemy(pos, false, player));

	// 回復の泉（中ボスの部屋の奥。光るのは、中ボスの部屋の敵を全員倒した後）
	std::shared_ptr<HealFountain> fountain = std::make_shared<HealFountain>();
	fountain->SetPos(kFountainPos);
	fountain->SetTarget(player);
	fountain->Init();
	m_objList.push_back(fountain);
	m_wpFountain = fountain;

	Gate rightGate;
	rightGate.barrier = rightGateBarrier;
	rightGate.name = "右の部屋の出口";
	rightGate.message = "道が開いた";
	for (const auto& pos : kRightRoomEnemies) rightGate.enemies.push_back(SpawnEnemy(pos, false, player));
	m_gates.push_back(rightGate);

	// 鍵の部屋（全員倒すと鍵が出る。UpdateKey）
	for (const auto& pos : kKeyRoomEnemies) m_keyRoomEnemies.push_back(SpawnEnemy(pos, false, player));

	// ボスの部屋：入口の近くにザコ、玉座の手前にボス（プレイヤーが近づくと動き出す）
	for (const auto& pos : kBossRoomEnemies) SpawnEnemy(pos, false, player);
	SpawnEnemy(kBossPos, true, player);

	// カメラ
	// （Effekseer にも同じカメラを渡す。エフェクトを、3D の物と同じカメラから見た位置に描くため）
	std::shared_ptr<TPSCamera> camera = CreateObject<TPSCamera>();
	player->SetCamera(camera);
	camera->SetTarget(player);
	KdEffekseerManager::GetInstance().SetCamera(camera->GetCamera());

	// 右下の操作キーの一覧
	std::shared_ptr<ControlsGuide> controlsGuide = CreateObject<ControlsGuide>();
	controlsGuide->SetPlayer(player);

	// 敵の HP ゲージとブレイクゲージ（敵の頭の上。ロックオン中の敵・最後に殴った敵・戦っているボスなど）
	std::shared_ptr<EnemyGage> enemyGage = CreateObject<EnemyGage>();
	enemyGage->SetPlayer(player);

	// 画面のメッセージ（「鍵を手に入れた」など）と、鍵を持っている表示
	std::shared_ptr<StageMessage> message = CreateObject<StageMessage>();
	m_wpMessage = message;
	message->Show("鍵を見つけて、奥の扉を開けろ", 240);
}

std::shared_ptr<Enemy> GameScene::SpawnEnemy(const Math::Vector3& pos, bool isBoss, const std::shared_ptr<Player>& target, bool isMidBoss)
{
	std::shared_ptr<Enemy> enemy = std::make_shared<Enemy>();
	enemy->SetBoss(isBoss);
	enemy->SetMidBoss(isMidBoss);
	enemy->Init();
	enemy->SetPos(pos);
	enemy->SetTarget(target);
	m_objList.push_back(enemy);
	return enemy;
}
