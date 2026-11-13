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

	// 画面の外に出た時は、画面のふちの内側に止めて出す（大きいボスの頭の上が、画面の上にはみ出すため）
	constexpr int		kEdgeMarginTop		= 30;		// 上のふちから、これだけ内側
	constexpr int		kEdgeMarginSide		= 20;		// 左右のふちから（ゲージの幅の半分に足す）
	constexpr int		kEdgeMarginBottom	= 200;		// 下のふちから（左下・右下の HUD と重ならないように）
	constexpr float		kEdgeAlphaRate		= 0.8f;		// ふちに止めている時は、少し薄くする
	constexpr int		kArrowGap			= 8;		// 矢印とゲージのすき間
	constexpr int		kArrowLength		= 7;		// 矢印の長さ
	constexpr int		kArrowHalfWidth		= 6;		// 矢印の幅の半分

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
	Math::Vector2 halfScreen = SpriteUtil::GetScreenSize() * 0.5f;

	Math::Vector4 clip = Math::Vector4::Transform(Math::Vector4(pos.x, pos.y, pos.z, 1.0f), cam.mView * cam.mProj);
	if (clip.w > 0.01f)
	{
		// カメラの前：x, y を w で割ると -1～1（画面の左下 ～ 右上）になる
		out.x = clip.x / clip.w * halfScreen.x;
		out.y = clip.y / clip.w * halfScreen.y;
		return true;
	}

	// カメラの後ろ：そのまま割ると左右・上下が逆になるので、
	// カメラから見て左右どちらにいるかだけを使い、画面の下のずっと外に置く（あとで下のふちに止める）
	Math::Vector3 viewPos = Math::Vector3::Transform(pos, cam.mView);
	out.x = (viewPos.x >= 0.0f) ? halfScreen.x * 2.0f : -halfScreen.x * 2.0f;
	if (std::abs(viewPos.x) < 1.0f) out.x = 0.0f;		// ほぼ真後ろ：真ん中
	out.y = -halfScreen.y * 4.0f;
	return false;
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

	// カメラから遠すぎる敵には出さない（カメラの後ろの敵は、画面の下のふちに出す）
	const auto& cam = KdShaderManager::Instance().GetCameraCB();
	Math::Vector3 camPos = cam.mView.Invert().Translation();
	if ((e.headPos - camPos).Length() > kMaxShowDistance) return;

	Math::Vector2 screenPos;
	WorldToScreen(e.headPos, screenPos);

	// 画面のふちの内側に止める（ゲージの幅の分だけ、左右は内側にする）
	int halfW = e.isBig ? kBigHalfW : kSmallHalfW;
	Math::Vector2 halfScreen = SpriteUtil::GetScreenSize() * 0.5f;
	Math::Vector2 clamped =
	{
		std::clamp(screenPos.x, -halfScreen.x + halfW + kEdgeMarginSide, halfScreen.x - halfW - kEdgeMarginSide),
		std::clamp(screenPos.y, -halfScreen.y + kEdgeMarginBottom, halfScreen.y - kEdgeMarginTop),
	};
	Math::Vector2 outDir = screenPos - clamped;		// 止めた所から、本当の位置への向き
	bool isOnEdge = outDir.LengthSquared() > 1.0f;

	int x = static_cast<int>(clamped.x);
	int y = static_cast<int>(clamped.y);

	auto& spriteShader = KdShaderManager::Instance().m_spriteShader;
	float alpha = e.alpha * (isOnEdge ? kEdgeAlphaRate : 1.0f);
	Math::Color back = kBackColor;		back.w *= alpha;
	Math::Color frame = kFrameColor;	frame.w *= alpha;

	// ふちに止めている時は、本当の位置の方へ向いた三角の矢印を出す（「画面の外にいる」と分かるように）
	if (isOnEdge)
	{
		outDir.Normalize();
		Math::Vector2 side = { -outDir.y, outDir.x };
		// 上下に出ている時はゲージの上・下に、左右に出ている時はゲージの横に出す
		float gap = (std::abs(outDir.y) >= std::abs(outDir.x)) ? (kHpHalfH + kArrowGap) : (halfW + kArrowGap);
		Math::Vector2 base = clamped + outDir * gap;
		Math::Vector2 tip = base + outDir * kArrowLength;
		Math::Vector2 b1 = base + side * kArrowHalfWidth;
		Math::Vector2 b2 = base - side * kArrowHalfWidth;
		Math::Color arrow = kHpColor;
		arrow.w *= alpha;
		spriteShader.DrawTriangle(
			static_cast<int>(tip.x), static_cast<int>(tip.y),
			static_cast<int>(b1.x), static_cast<int>(b1.y),
			static_cast<int>(b2.x), static_cast<int>(b2.y), &arrow, true);
	}

	// ---- HP ゲージ ----
	spriteShader.DrawBox(x, y, halfW, kHpHalfH, &back, true);
	DrawBar(x, y, halfW, kHpHalfH, e.delayRatio, kDelayColor, alpha);
	DrawBar(x, y, halfW, kHpHalfH, e.hpRatio, kHpColor, alpha);
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
	DrawBar(x, breakY, halfW, kBreakHalfH, e.breakRatio, breakColor, alpha);
	spriteShader.DrawBox(x, breakY, halfW, kBreakHalfH, &frame, false);

	// ブレイク中は、ゲージの上に「ブレイク」と出す
	if (e.isBroken && m_breakText)
	{
		Math::Color textColor = breakColor;
		textColor.w *= alpha;
		m_breakText->SetColor(textColor);
		// 画面の上のふちに止めている時は、上の矢印と重ならないように、ゲージの下に出す
		bool isTopEdge = isOnEdge && outDir.y > 0.0f;
		int textY = isTopEdge ? (breakY - kBreakHalfH - kBreakTextOffsetY + kHpHalfH) : (y + kBreakTextOffsetY);
		m_breakText->SetCenterPos({ static_cast<float>(x), static_cast<float>(textY) });
		m_breakText->DrawSprite();
	}
}

void EnemyGage::DrawSprite()
{
	KdShaderManager::Instance().m_spriteShader.SetMatrix(Math::Matrix::Identity);
	for (const Entry& e : m_entries) DrawEntry(e);
}
