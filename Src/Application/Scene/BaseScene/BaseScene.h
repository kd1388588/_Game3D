#pragma once

// ============================================================
// 【役割】シーンの親クラス（元のテンプレートのまま。e01ded2 から変えていない）
//         シーンにいる全オブジェクトをリストで持ち、更新・描画をまとめて呼ぶ
//
// 【構成】・m_objList：シーンにいる全オブジェクト（KdGameObject を継承した物）
//         ・Init()  ：シーンを作った時に 1 回（子のシーンでオブジェクトを置く）
//         ・Event() ：毎フレーム、全員の Update の前に 1 回（子のシーンで画面の切り替えなどを判定する）
//
// 【なぜ親クラスにするのか】どのシーンでも「全員を更新・描画する」流れは同じなので、
//         そこは親にまとめ、シーンごとに違う所（何を置くか・いつ切り替えるか）だけを子が書く
//
// 【C++ の注意】BaseScene() の中で Init() を呼んでも、子の Init() にはならない
//         （コンストラクタの中では、仮想関数は自分のクラスの物が呼ばれる）
//         そのため、子のシーンのコンストラクタでも Init() を呼んでいる（TitleScene() { Init(); } など）
//
// 【参考】Game Programming Patterns「Update Method」（全員を同じ呼び方で更新する考え方）
//         https://gameprogrammingpatterns.com/update-method.html
// ============================================================
class BaseScene
{
public :

	BaseScene()			 { Init(); }
	virtual ~BaseScene() {}

	void PreUpdate();
	void Update();
	void PostUpdate();

	void PreDraw();
	void Draw();
	void DrawSprite();
	void DrawDebug();

	// オブジェクトリストを取得
	const std::list<std::shared_ptr<KdGameObject>>& GetObjList()
	{
		return m_objList;
	}
	
	// オブジェクトリストに追加
	void AddObject(const std::shared_ptr<KdGameObject>& _obj)
	{
		m_objList.push_back(_obj);
	}

protected :

	// 継承先シーンで必要ならオーバーライドする
	virtual void Event();
	virtual void Init();

	// 全オブジェクトのアドレスをリストで管理
	std::list<std::shared_ptr<KdGameObject>> m_objList;
};
