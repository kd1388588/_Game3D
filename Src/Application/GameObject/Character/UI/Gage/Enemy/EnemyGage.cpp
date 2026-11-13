#include "EnemyGage.h"
#include "../../../Player/Player.h"
#include "../../../Enemy/Enemy.h"
#include "../../../../UI/Text/UIText.h"
#include "../../../../../Scene/SceneManager.h"

namespace
{
	// 位置（画面の真ん中が原点、上が +Y）
	constexpr int		kBarY				= 300;		// HP ゲージの高さ
	constexpr int		kBigHalfW			= 280;		// ボス・中ボス：幅 560
	constexpr int		kBigHalfH			= 6;		// 太さ 12
	constexpr int		kSmallHalfW			= 150;		// ザコ：幅 300
	constexpr int		kSmallHalfH			= 4;		// 太さ 8
	constexpr int		kBreakGap			= 4;		// HP ゲージとブレイクゲージのすき間
	constexpr int		kBreakHalfH			= 3;		// ブレイクゲージの太さ 6
	const Math::Vector2	kNameOffset			= { 0.0f, 22.0f };	// 名前の文字（ゲージの上）
	constexpr float		kTextScale			= 0.7f;
	constexpr int		kBreakTextOffsetX	= 50;		// 「ブレイク」の文字（ブレイクゲージの右）

	// どの敵を出すか
	constexpr int		kShowAfterHitFrame	= 60 * 5;	// 最後に当ててから、この間は出しておく
	constexpr float		kBossEngageMargin	= 4.0f;		// ボスがこちらに気づく距離 ＋ これ以内なら出す

	// 動き
	constexpr float		kFadeSpeed			= 0.1f;		// 出てくる・消える速さ（1 フレームに変わる不透明度）
	constexpr int		kDelayWaitFrame		= 30;		// HP が減ってから、白い部分が縮み始めるまで
	constexpr float		kDelaySpeed			= 0.01f;	// 白い部分が縮む速さ（1 フレームに縮む割合）
	constexpr float		kBreakPulseSpeed	= 0.2f;		// ブレイク中の点滅の速さ

	// 色
	const Math::Color	kBackColor			= { 0.0f, 0.0f, 0.0f, 0.6f };
	const Math::Color	kFrameColor			= { 1.0f, 1.0f, 1.0f, 0.45f };
	const Math::Color	kHpColor			= { 0.85f, 0.16f, 0.12f, 1.0f };	// 赤
	const Math::Color	kDelayColor			= { 1.0f, 0.88f, 0.75f, 0.85f };	// 減った分（白っぽい）
	const Math::Color	kBreakColor			= { 1.0f, 0.55f, 0.12f, 1.0f };		// たまっている途中（オレンジ）
	const Math::Color	kBrokenColorDark	= { 1.0f, 0.7f, 0.2f, 1.0f };		// ブレイク中（金色で点滅）
	const Math::Color	kBrokenColorBright	= { 1.0f, 0.95f, 0.6f, 1.0f };
	const Math::Color	kNameColor			= { 1.0f, 0.92f, 0.8f, 1.0f };
}

void EnemyGage::Init()
{
	m_nameText = std::make_shared<UIText>();
	m_nameText->SetScale(kTextScale);

	m_breakText = std::make_shared<UIText>();
	m_breakText->SetText("ブレイク", UIText::FontSize::Small);
	m_breakText->SetScale(kTextScale);
}

std::shared_ptr<Enemy> EnemyGage::FindTarget() const
{
	auto player = m_wpPlayer.lock();
	if (!player) return nullptr;

	// ① ロックオンしている敵
	if (auto lock = player->GetLockTarget(); lock && lock->IsAlive()) return lock;

	// ② 近くで戦っているボス・中ボス　③ 最後に攻撃を当てた敵
	std::shared_ptr<Enemy> boss = nullptr;
	std::shared_ptr<Enemy> lastHit = nullptr;
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		auto enemy = std::dynamic_pointer_cast<Enemy>(obj);
		if (!enemy || !enemy->IsAlive() || enemy->IsExpired()) continue;

		if (enemy->IsBoss() || enemy->IsMidBoss())
		{
			Math::Vector3 diff = enemy->GetPos() - player->GetPos();
			diff.y = 0.0f;
			if (diff.Length() <= enemy->GetSearchRange() + kBossEngageMargin) boss = enemy;
		}
		if (enemy->GetFramesSinceHit() <= kShowAfterHitFrame
			&& (!lastHit || enemy->GetFramesSinceHit() < lastHit->GetFramesSinceHit()))
		{
			lastHit = enemy;
		}
	}
	return boss ? boss : lastHit;
}

void EnemyGage::Update()
{
	m_frame++;

	std::shared_ptr<Enemy> target = FindTarget();
	std::shared_ptr<Enemy> current = m_wpTarget.lock();
	bool isSwitching = (target != current);

	// 出す敵が変わった時は、今のゲージが消えきってから、新しい敵のゲージを出す
	if (isSwitching && (!current || m_alpha <= 0.0f))
	{
		m_wpTarget = target;
		current = target;
		isSwitching = false;
		m_alpha = 0.0f;
		if (current)
		{
			m_delayRatio = current->GetHpRatio();
			m_lastHpRatio = m_delayRatio;
			m_delayWait = 0;
			m_isBigBar = current->IsBoss() || current->IsMidBoss();
			m_nameText->SetText(current->IsBoss() ? "ボス" : "中ボス", UIText::FontSize::Small);
		}
	}

	if (!current)
	{
		m_alpha = 0.0f;
		return;
	}

	bool isAlive = current->IsAlive() && !current->IsExpired();
	if (isAlive)
	{
		m_hpRatio = current->GetHpRatio();
		m_hasBreak = current->CanBreak();
		m_breakRatio = current->GetBreakRatio();
		m_isBroken = current->IsBroken();
	}
	else
	{
		// 倒した敵は、HP 0 のゲージを少し見せてから消す
		m_hpRatio = 0.0f;
		m_hasBreak = false;
	}

	// 減った HP の白い部分：少し待ってから、ゆっくり縮める（回復した時はすぐに合わせる）
	if (m_hpRatio < m_lastHpRatio) m_delayWait = kDelayWaitFrame;
	m_lastHpRatio = m_hpRatio;
	if (m_delayRatio < m_hpRatio)	m_delayRatio = m_hpRatio;
	else if (m_delayWait > 0)		m_delayWait--;
	else							m_delayRatio = std::max(m_delayRatio - kDelaySpeed, m_hpRatio);

	// 不透明度：倒した敵は白い部分が縮み終わってから、ほかの敵に変わる時はすぐに消していく
	bool isFadeOut = isAlive ? isSwitching : (m_delayRatio <= 0.0f);
	if (isFadeOut)	m_alpha = std::max(m_alpha - kFadeSpeed, 0.0f);
	else if (isAlive) m_alpha = std::min(m_alpha + kFadeSpeed, 1.0f);
}

void EnemyGage::DrawBar(int centerX, int centerY, int halfW, int halfH, float ratio, const Math::Color& color, float alpha)
{
	auto& spriteShader = KdShaderManager::Instance().m_spriteShader;
	int fillHalfW = static_cast<int>(halfW * std::clamp(ratio, 0.0f, 1.0f));
	if (fillHalfW <= 0) return;

	Math::Color c = color;
	c.w *= alpha;
	int left = centerX - halfW;
	spriteShader.DrawBox(left + fillHalfW, centerY, fillHalfW, halfH, &c, true);
}

void EnemyGage::DrawSprite()
{
	if (m_alpha <= 0.0f) return;

	auto& spriteShader = KdShaderManager::Instance().m_spriteShader;
	spriteShader.SetMatrix(Math::Matrix::Identity);

	int halfW = m_isBigBar ? kBigHalfW : kSmallHalfW;
	int halfH = m_isBigBar ? kBigHalfH : kSmallHalfH;
	Math::Color back = kBackColor;	back.w *= m_alpha;
	Math::Color frame = kFrameColor;	frame.w *= m_alpha;

	// ---- HP ゲージ ----
	spriteShader.DrawBox(0, kBarY, halfW, halfH, &back, true);
	DrawBar(0, kBarY, halfW, halfH, m_delayRatio, kDelayColor, m_alpha);
	DrawBar(0, kBarY, halfW, halfH, m_hpRatio, kHpColor, m_alpha);
	spriteShader.DrawBox(0, kBarY, halfW, halfH, &frame, false);

	// 名前（ボス・中ボスだけ）
	if (m_isBigBar && m_nameText)
	{
		Math::Color nameColor = kNameColor;
		nameColor.w *= m_alpha;
		m_nameText->SetColor(nameColor);
		m_nameText->SetCenterPos({ 0.0f, kBarY + kNameOffset.y });
		m_nameText->DrawSprite();
	}

	// ---- ブレイクゲージ（HP ゲージの下） ----
	if (!m_hasBreak) return;

	int breakY = kBarY - halfH - kBreakGap - kBreakHalfH;
	spriteShader.DrawBox(0, breakY, halfW, kBreakHalfH, &back, true);

	Math::Color breakColor = kBreakColor;
	if (m_isBroken)
	{
		float pulse = 0.5f + 0.5f * std::sin(m_frame * kBreakPulseSpeed);
		breakColor = Math::Color::Lerp(kBrokenColorDark, kBrokenColorBright, pulse);
	}
	DrawBar(0, breakY, halfW, kBreakHalfH, m_breakRatio, breakColor, m_alpha);
	spriteShader.DrawBox(0, breakY, halfW, kBreakHalfH, &frame, false);

	// ブレイク中は、ゲージの右に「ブレイク」と出す
	if (m_isBroken && m_breakText)
	{
		Math::Color textColor = breakColor;
		textColor.w *= m_alpha;
		m_breakText->SetColor(textColor);
		m_breakText->SetCenterPos({ static_cast<float>(halfW + kBreakTextOffsetX), static_cast<float>(breakY) });
		m_breakText->DrawSprite();
	}
}
