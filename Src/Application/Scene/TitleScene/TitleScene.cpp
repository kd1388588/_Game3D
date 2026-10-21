#include "TitleScene.h"
#include "../../Utility/StageEnvironment.h"
#include "../../Utility/SoundPlayer.h"
#include "../../GameObject/Effect/EffectPlayer.h"
#include "../SceneManager.h"
#include "../../main.h"

// Stage関連（ゲーム画面と同じ背景）
#include "../../GameObject/Stage/Ground/Ground.h"
#include "../../GameObject/Stage/Sky/Sky.h"
#include "../../GameObject/Stage/Object/RuinedRoad/RuinedRoad.h"

// タイトル画面の演出
#include "../../GameObject/Title/TitleChara/TitleChara.h"
#include "../../GameObject/Title/TitleCamera/TitleCamera.h"
#include "../../GameObject/Title/TitleEmber/TitleEmber.h"
#include "../../GameObject/Title/TitleUI/TitleUI.h"

// Effekseer関連
#include "../../../Framework/Effekseer/KdEffekseerManager.h"

#include "../../GameObject/UI/Text/UIText.h"

void TitleScene::Event()
{
	auto ui = m_wpUI.lock();
	if (!ui) return;

	// メニューで決定し、フェードアウトが終わったら画面を切り替える
	switch (ui->GetResult())
	{
	case TitleUI::Result::Start:
		SceneManager::Instance().SetNextScene(SceneManager::SceneType::Game);
		break;
	case TitleUI::Result::Exit:
		Application::Instance().End();
		break;
	default:
		break;
	}
}

void TitleScene::Init()
{
	// タイトル画面では、エフェクシアのエフェクトを出さない（ゲーム画面に入ったら出す）
	EffectPlayer::SetEnabled(false);

	// BGM
	SoundPlayer::PlayBGM(SoundName::TitleBGM);

	// 光・フォグ・影（ゲーム画面と同じ）
	StageEnvironment::Apply();

	// 背景（ゲーム画面と同じステージ）
	CreateObject<Ground>();
	CreateObject<Sky>();
	CreateObject<RuinedRoad>();	// ゲームと同じステージ（廃墟の参道と崩れた大聖堂）

	// 待機と連続攻撃を繰り返すキャラクター
	std::shared_ptr<TitleChara> chara = CreateObject<TitleChara>();

	// キャラクターの周りをゆっくり動き続けるカメラ
	std::shared_ptr<TitleCamera> camera = CreateObject<TitleCamera>();
	camera->SetChara(chara);
	KdEffekseerManager::GetInstance().SetCamera(camera->GetCamera());

	// 2D：舞い上がる火の粉 → ロゴ・メニュー（後に追加したものほど手前に描かれる）
	CreateObject<TitleEmber>();
	m_wpUI = CreateObject<TitleUI>();
}
