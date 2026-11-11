#pragma once

// 前方宣言：中身を使わない（ポインタとして持つだけ）ので、ヘッダーを読まずに名前だけ知らせる
// 【なぜ】ヘッダーを読む数が減り、ビルドが速くなる。お互いを読み合う（循環 include）も防げる
class BaseScene;

// ============================================================
// 【役割】シーン（タイトル・ゲーム・クリア・ゲームオーバー）を 1 つ持ち、切り替える
//
// 【構成】・今のシーン（m_currentScene）に、Update・Draw などをそのまま任せる
//         ・SetNextScene で「次のシーン」を予約し、次のフレームの最初（PreUpdate）に切り替える
//
// 【なぜ予約するのか】シーンの Update の途中で切り替えると、今動いているシーン（自分自身）を
//         消すことになり、落ちる。フレームの最初なら、誰も動いていないので安全に消せる
//
// 【参考】Game Programming Patterns「State」（場面ごとにクラスを分けて切り替える考え方）
//         https://gameprogrammingpatterns.com/state.html
// ============================================================

class SceneManager
{
public :

	// シーン情報
	enum class SceneType
	{
		Title,
		Game,
		GameClear,
		GameOver,
	};

	void PreUpdate();
	void Update();
	void PostUpdate();

	void PreDraw();
	void Draw();
	void DrawSprite();
	void DrawDebug();

	// 次のシーンをセット (次のフレームから切り替わる)
	void SetNextScene(SceneType _nextScene)
	{
		m_nextSceneType = _nextScene;
	}

	// 現在のシーンの種類
	SceneType GetCurrentSceneType() const { return m_currentSceneType; }

	// 現在のシーンのオブジェクトリストを取得
	// （当たり判定などで「シーンにいる全員」を調べる時に使う。例：剣の攻撃・敵どうしの押し出し）
	const std::list<std::shared_ptr<KdGameObject>>& GetObjList();

	// 現在のシーンにオブジェクトを追加
	void AddObject(const std::shared_ptr<KdGameObject>& _obj);

private :

	// マネージャーの初期化
	// インスタンス生成(アプリ起動)時にコンストラクタで自動実行
	void Init()
	{
		// 開始シーンに切り替え
		ChangeScene(m_currentSceneType);
	}

	// シーン切り替え関数
	void ChangeScene(SceneType _sceneType);

	// 現在のシーンのインスタンスを保持しているポインタ
	// （shared_ptr なので、新しいシーンを代入すると、古いシーンは自動で消える）
	std::shared_ptr<BaseScene> m_currentScene = nullptr;

	// 現在のシーンの種類を保持している変数
	SceneType m_currentSceneType = SceneType::Title;	// 起動時のシーン
	
	// 次のシーンの種類を保持している変数
	SceneType m_nextSceneType = m_currentSceneType;

private:

	SceneManager() { Init(); }
	~SceneManager() {}

public:

	// シングルトンパターン
	// 常に存在する && 必ず1つしか存在しない(1つしか存在出来ない)
	// どこからでもアクセスが可能で便利だが
	// 何でもかんでもシングルトンという思考はNG
	static SceneManager& Instance()
	{
		static SceneManager instance;
		return instance;
	}
};
