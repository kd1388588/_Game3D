#include "EnemyGage.h"
#include "../../../Player/Player.h"
#include "../../../Enemy/Enemy.h"
#include "../../../../UI/Text/UIText.h"
#include "../../../../../Scene/SceneManager.h"
#include "../../../../../Utility/SpriteUtil.h"

namespace
{
	// 位置
	constexpr float		kHeadHeightRate		= 1.95f;	// 頭の高さ（体の大きさに掛ける）
	constexpr float		kHeadMargin			= 0.35f;	// 頭からさらに上へ（m）
	constexpr float		kMaxShowDistance	= 30.0f;	// カメラからこれより遠い敵には出さない

	// 大きさ（画面のピクセル。遠くても同じ大きさにして、読みやすくする）
	constexpr int		kSmallHalfW			= 45;		// ザコ：幅 90
	constexpr int		kBigHalfW			= 90;		// ボス・中ボス：幅 180
	constexpr int		kHpHalfH			= 4;		// HP ゲージの太さ 8
	constexpr int		kBreakGap			= 3;		// HP ゲージとブレイクゲージのすき間
	constexpr int		kBreakHalfH			= 2;		// ブレイクゲージの太さ 4
	constexpr float		kBreakTextScale		= 0.6f;
	constexpr int		kBreakTextOffsetY	= 16;		// 「ブレイク」の文字（HP ゲージの上）

	// どの敵に出すか
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
}

void EnemyGage::Init()
{
	m_breakText = std::make_shared<UIText>();
	m_breakText->SetText("ブレイク", UIText::FontSize::Small);
	m_breakText->SetScale(kBreakTextScale);
}

bool EnemyGage::ShouldShow(const std::shared_ptr<Player>& player, const std::shared_ptr<Enemy>& enemy) const
{
	if (!enemy->IsAlive() || enemy->IsExpired()) return false;
	if (player->GetLockTarget() == enemy) return true;
	if (enemy->GetFramesSinceHit() <= kShowAfterHitFrame) return true;
	if (enemy->IsBroken()) return true;

	if (enemy->IsBoss() || enemy->IsMidBoss())
	{
		Math::Vector3 diff = enemy->GetPos() - player->GetPos();
		diff.y = 0.0f;
		if (diff.Length() <= enemy->GetSearchRange() + kBossEngageMargin) return true;
	}
	return false;
}

void EnemyGage::Update()
{
	m_frame++;

	auto player = m_wpPlayer.lock();
	if (!player) { m_entries.clear(); return; }

	// 出すべき敵で、まだ一覧に無いものを足す
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		auto enemy = std::dynamic_pointer_cast<Enemy>(obj);
		if (!enemy || !ShouldShow(player, enemy)) continue;

		bool isFound = false;
		for (const Entry& e : m_entries)
		{
			if (e.wpEnemy.lock() == enemy) { isFound = true; break; }
		}
		if (isFound) continue;

		Entry entry;
		entry.wpEnemy = enemy;
		entry.hpRatio = enemy->GetHpRatio();
		entry.delayRatio = entry.hpRatio;
		entry.isBig = enemy->IsBoss() || enemy->IsMidBoss();
		m_entries.push_back(entry);
	}

	// 一覧の敵の状態を更新する
	for (Entry& e : m_entries)
	{
		auto enemy = e.wpEnemy.lock();
		bool isAlive = enemy && enemy->IsAlive() && !enemy->IsExpired();
		bool isShow = false;

		if (isAlive)
		{
			isShow = ShouldShow(player, enemy);
			float hpRatio = enemy->GetHpRatio();
			if (hpRatio < e.hpRatio) e.delayWait = kDelayWaitFrame;	// HP が減った瞬間：白い部分を少し待たせる
			e.hpRatio = hpRatio;
			e.hasBreak = enemy->CanBreak();
			e.breakRatio = enemy->GetBreakRatio();
			e.isBroken = enemy->IsBroken();
			e.headPos = enemy->GetPos() + Math::Vector3(0.0f, enemy->GetBodyScale() * kHeadHeightRate + kHeadMargin, 0.0f);
		}
		else
		{
			// 倒した敵は、HP 0 のゲージを少し見せてから消す
			if (e.hpRatio > 0.0f) e.delayWait = kDelayWaitFrame;
			e.hpRatio = 0.0f;
			e.hasBreak = false;
		}

		// 減った HP の白い部分：少し待ってから、ゆっくり縮める（回復した時はすぐに合わせる）
		if (e.delayRatio < e.hpRatio)	e.delayRatio = e.hpRatio;
		else if (e.delayWait > 0)		e.delayWait--;
		else							e.delayRatio = std::max(e.delayRatio - kDelaySpeed, e.hpRatio);

		// 不透明度：出す間は濃く、出さなくなったら薄く（倒した敵は白い部分が縮み終わってから）
		bool isFadeOut = isAlive ? !isShow : (e.delayRatio <= 0.0f);
		if (isFadeOut)		e.alpha = std::max(e.alpha - kFadeSpeed, 0.0f);
		else if (isAlive)	e.alpha = std::min(e.alpha + kFadeSpeed, 1.0f);

		e.isShow = isShow;
	}

	// 消えきった物を一覧から外す
	m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(),
		[](const Entry& e) { return e.alpha <= 0.0f && !e.isShow; }),
		m_entries.end());
}

bool EnemyGage::WorldToScreen(const Math::Vector3& pos, Math::Vector2& out)
{
	const auto& cam = KdShaderManager::Instance().GetCameraCB();
	Math::Vector4 clip = Math::Vector4::Transform(Math::Vector4(pos.x, pos.y, pos.z, 1.0f), cam.mView * cam.mProj);
	if (clip.w <= 0.01f) return false;		// カメラの後ろ

	Math::Vector2 screen = SpriteUtil::GetScreenSize();
	out.x = clip.x / clip.w * screen.x * 0.5f;
	out.y = clip.y / clip.w * screen.y * 0.5f;
	return true;
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

void EnemyGage::DrawEntry(const Entry& e)
{
	if (e.alpha <= 0.0f) return;

	// カメラから遠すぎる・カメラの後ろの敵には出さない
	const auto& cam = KdShaderManager::Instance().GetCameraCB();
	Math::Vector3 camPos = cam.mView.Invert().Translation();
	if ((e.headPos - camPos).Length() > kMaxShowDistance) return;

	Math::Vector2 screenPos;
	if (!WorldToScreen(e.headPos, screenPos)) return;
	int x = static_cast<int>(screenPos.x);
	int y = static_cast<int>(screenPos.y);

	auto& spriteShader = KdShaderManager::Instance().m_spriteShader;
	int halfW = e.isBig ? kBigHalfW : kSmallHalfW;
	Math::Color back = kBackColor;		back.w *= e.alpha;
	Math::Color frame = kFrameColor;	frame.w *= e.alpha;

	// ---- HP ゲージ ----
	spriteShader.DrawBox(x, y, halfW, kHpHalfH, &back, true);
	DrawBar(x, y, halfW, kHpHalfH, e.delayRatio, kDelayColor, e.alpha);
	DrawBar(x, y, halfW, kHpHalfH, e.hpRatio, kHpColor, e.alpha);
	spriteShader.DrawBox(x, y, halfW, kHpHalfH, &frame, false);

	// ---- ブレイクゲージ（HP ゲージの下） ----
	if (!e.hasBreak) return;

	int breakY = y - kHpHalfH - kBreakGap - kBreakHalfH;
	spriteShader.DrawBox(x, breakY, halfW, kBreakHalfH, &back, true);

	Math::Color breakColor = kBreakColor;
	if (e.isBroken)
	{
		float pulse = 0.5f + 0.5f * std::sin(m_frame * kBreakPulseSpeed);
		breakColor = Math::Color::Lerp(kBrokenColorDark, kBrokenColorBright, pulse);
	}
	DrawBar(x, breakY, halfW, kBreakHalfH, e.breakRatio, breakColor, e.alpha);
	spriteShader.DrawBox(x, breakY, halfW, kBreakHalfH, &frame, false);

	// ブレイク中は、ゲージの上に「ブレイク」と出す
	if (e.isBroken && m_breakText)
	{
		Math::Color textColor = breakColor;
		textColor.w *= e.alpha;
		m_breakText->SetColor(textColor);
		m_breakText->SetCenterPos({ static_cast<float>(x), static_cast<float>(y + kBreakTextOffsetY) });
		m_breakText->DrawSprite();
	}
}

void EnemyGage::DrawSprite()
{
	KdShaderManager::Instance().m_spriteShader.SetMatrix(Math::Matrix::Identity);
	for (const Entry& e : m_entries) DrawEntry(e);
}
