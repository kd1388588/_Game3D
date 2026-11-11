#pragma once

class BaseScene;

class SceneManager
{
public:

	enum SceneType
	{
		Title,
		Game,
		GameClear,
		GameOver,
	};

	// 初期設定
	void Init()
	{
		ChangeScene(m_currentSceneType);
	}

	// ゲーム内更新
	void PreUpdate();
	void Update();
	void PostUpdate();

	// ゲーム内描画
	void PreDraw();
	void Draw();
	void DrawSprite();
	void DrawDebug();

	// 次のシーンをセットする（次フレームで遷移）
	void SetNextScene(SceneType nextScene) { m_nextSceneType = nextScene; }

	// 描画中のシーンを取得
	SceneType GetCurrentSceneType() const { return m_currentSceneType; }

	// 描画中のシーン内のオブジェクトリスト
	const std::list<std::shared_ptr<KdGameObject>>& GetObjlist();

	// オブジェクト追加
	void AddObject(const std::shared_ptr<KdGameObject>& obj);

private:

	SceneManager() {}
	~SceneManager() {}

	void ChangeScene(SceneType scenetype);

	std::shared_ptr<BaseScene>	m_currentScene = nullptr;			// 描画中のシーン
	SceneType					m_currentSceneType = SceneType::Title;  // そのシーンの種類

	SceneType					m_nextSceneType = m_currentSceneType;	// 次のシーンの種類

public: 

	static SceneManager& Instance()
	{
		static SceneManager instance;
		return instance;
	}
};