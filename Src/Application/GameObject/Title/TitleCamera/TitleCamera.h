#pragma once
#include "../../Camera/CameraBase.h"

class TitleChara;

// タイトル画面のカメラ
// 最初は遠くから寄っていき、その後はキャラクターの周りをゆっくり揺れ動き続ける
class TitleCamera : public CameraBase
{
public:

	TitleCamera() {}
	~TitleCamera() override {}

	void Init()			override;
	void PostUpdate()	override;

	void SetChara(const std::shared_ptr<TitleChara>& chara) { m_wpChara = chara; }

private:

	std::weak_ptr<TitleChara>	m_wpChara;
	int							m_frame = 0;	// 経過フレーム
};
