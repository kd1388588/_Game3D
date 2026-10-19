#pragma once

#include"../BaseScene/BaseScene.h"
#include"../../Utility/InputHelper.h"

class TitleUI;

class TitleScene : public BaseScene
{
public :

	TitleScene()  { Init(); }
	~TitleScene() {}

private :

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

	std::weak_ptr<TitleUI>	m_wpUI;
};
