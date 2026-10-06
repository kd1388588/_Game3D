#pragma once

#include"../BaseScene/BaseScene.h"
#include"../../Utility/InputHelper.h"

// ゲームクリア画面（Enterでタイトルへ）
class GameClearScene : public BaseScene
{
public :

	GameClearScene()  { Init(); }
	~GameClearScene() {}

private :

	void Event() override;
	void Init()  override;

	InputHelper::KeyTrigger m_decideKey{ VK_RETURN };
};
