#pragma once

// ============================================================
// プレイヤーのステートの調整用の数値（フレーム数・速さ・アニメーション名など）
// ・PlayerState*.cpp の中だけで include する（PlayerState.h には書かない）
// ============================================================
namespace PlayerStateConst
{
	// 攻撃判定を出すアニメーションフレームの範囲
	constexpr float kAttackHitStartFrame	= 10.0f;
	constexpr float kAttackHitEndFrame		= 45.0f;
	constexpr int	kChargeLoopFrame		= 90;		// 4-3 Loop の持続フレーム

	// 溜め攻撃（4-3）の剣の軌跡
	// Start：16フレームしかなく、2フレーム目から速く振っているので、そこから出す
	// End　：End の最初の姿勢は「剣を地面に叩きつけた形」。Loop の途中から End に切り替わると、
	//        最初の数フレーム（アニメーションをなめらかにつなぐ間）に剣を振り下ろすので、
	//        剣先が本当に地面に届くまで軌跡を出す（念のため kSlamTrailMaxFrame で必ず止める）
	constexpr float kChargeStartTrailFrame	= 2.0f;
	constexpr float kSlamGroundHeight		= 0.15f;	// 右の剣先が足元からこの高さより下に来たら、地面に届いたとみなす
	constexpr float kSlamBlendFrame			= 5.0f;		// Loop → End をなめらかにつなぐフレーム数（この間は Loop の姿勢が残るので、地面の判定をしない）
	constexpr float kSlamTrailMaxFrame		= 15.0f;

	// 04の4段目（AS_Combo_Attack_04_04）：0～10 地面の剣を抜く、11～35 跳び上がる、36～47 落ちる、
	// 48～49 剣を地面に叩きつける。叩きつけるフレームは Player が起動時にアニメーションから求める
	constexpr float kJumpSlamTrailStartFrame	= 1.0f;	// 剣を地面から抜き始めるところから軌跡を出す
	constexpr float kJumpSlamTrailExtraFrame	= 1.0f;	// 叩きつけた後も少し出す（剣の位置は1フレーム遅れて作るので、その分）
	constexpr float kJumpSlamHitExtraFrame		= 4.0f;	// 叩きつけた後も、これだけ攻撃判定を出す
	// ※叩きつけるのは右の剣（End の最初の姿勢で、右の剣が地面に刺さっている）。
	//   左の剣も Loop で回る途中に地面近くを通るので、左は見ない
	constexpr int	kMaxComboStep			= 5;

	// 02の3段目（AS_Combo_Attack_02_03）：回りながら跳び上がる回転攻撃（アニメーションを Python で調べた結果）
	// 4～48：剣を振り回しながら跳び上がる（2本の剣が交互に体のまわりを回る）、64：一番高い所、68～76：落ちて着地
	// ※前は 20 フレームで次の段へ行けたので、連打すると回転のほとんどが途中で切れて当たらなかった
	constexpr float kSpinLeapHitStartFrame	= 4.0f;		// 回り始めから攻撃判定を出す
	constexpr float kSpinLeapHitEndFrame	= 80.0f;	// 着地まで出す
	constexpr float kSpinLeapCancelFrame	= 48.0f;	// 回転が終わってから次の段（空中から叩きつける4段目）へ行ける

	// 回避・被弾で移動するフレーム数
	constexpr float kEvadeMoveFrame			= 25.0f;
	constexpr float kDamageKnockbackFrame	= 15.0f;
	constexpr float kDamageKnockbackSpeed	= 0.06f;

	// ジャンプ
	constexpr float kJumpPower				= 0.25f;
	constexpr float kFallingGravity			= 0.011f;	// これを超えたら落下中とみなす
	constexpr int	kMaxLandingPredictFrame	= 120;		// 着地予測を行う最大フレーム数
	constexpr float kLandingBlendFrame		= 3.0f;		// 空中ループ→着地アニメのブレンドフレーム数
	constexpr float kJumpStartBlendFrame	= 4.0f;		// 待機・走り→ジャンプ開始のブレンドフレーム数
	constexpr float kJumpWindupFrames		= 6.0f;		// ジャンプの「ため」を見せるフレーム数（アニメーションのためが長い時は、途中から再生する）
	constexpr int	kMaxLandingLeadFrames	= 12;		// 着地アニメーションを、着地の何フレーム前まで早めに始めてよいか
	constexpr float kAirLoopBlendFrame		= 8.0f;		// ジャンプ開始→空中ループのブレンドフレーム数（パッと切り替わらないように）
	constexpr float kFallGravityRate		= 1.5f;		// 落ちる時の重力の倍率（上がる時より速く落ちて、ふわふわしない）

	// 空中での移動
	constexpr float kAirMinSpeed			= 0.06f;	// その場ジャンプから入力した時の速さ（1フレームの移動量）
	constexpr float kAirMaxSpeed			= 0.3f;		// 空中の速さの上限
	constexpr float kAirSteerRate			= 0.08f;	// 入力の方向へ曲がる強さ（0～1）
	constexpr float kAirDrag				= 0.99f;	// 入力が無い時の減速（1フレームごとに掛ける。空中で急に止まらないよう弱め）
	constexpr float kAirMaxTurnAngle		= 6.0f;		// 空中で1フレームに向きを変えられる角度（度）
	constexpr float kAirFaceMinSpeed		= 0.02f;	// これより遅い時は向きを変えない（その場ジャンプで体がぶれないように）
	constexpr float kLandingSlideDrag		= 0.75f;	// 着地した後、勢いを少しだけ残して止まる（1フレームごとに掛ける）

	// 走り（ルートモーション）
	constexpr float kRunBlendFrame			= 4.0f;		// 待機⇔走りのブレンドフレーム数（小さいほど素早く切り替わる）
	constexpr float kRunAnimSpeedShare		= 0.6f;		// 速くする分のうち、アニメーションの再生速度で上げる割合

	// 足音の大きさ（足が地面に着くたびに鳴らす。Player::UpdateFootsteps）
	constexpr float kRunFootstepVolume		= 0.5f;		// 走る
	constexpr float kDashFootstepVolume		= 0.8f;		// 全力疾走・急ブレーキ（強く踏む）
	constexpr float kGuardFootstepVolume	= 0.3f;		// ガードしながら歩く（そっと歩く）

	// 回避・着地エフェクトの大きさ
	constexpr float kEvadeEffectScale		= 1.0f;
	constexpr float kLandingEffectScale		= 1.0f;

	// プレイヤーは常に剣を抜いているので、剣を持った（Combat_）アニメーションを使う
	const std::string kIdleAnimName			= "Combat_Idle";
	const std::string kRunAnimName			= "Combat_Run";
	const std::string kRunSheatheAnimName	= "Run_Sheathe";	// 走りながら納刀
	const std::string kDashAnimName			= "Dash_Run";		// 剣を納めて全力で走る
	const std::string kDashBrakeAnimName	= "Dash_Brake";		// 全力疾走から急ブレーキ
	const std::string kIdleDrawAnimName		= "Idle_Draw";		// 立ったまま抜刀
	constexpr float kIdleDrawAnimSpeed		= 1.4f;		// 抜刀の再生速度（少し速くして、すぐ戦えるようにする）
	constexpr float kIdleDrawCancelRate		= 0.6f;		// 抜刀のこの割合まで進んだら、移動で走り出せる
	constexpr float kDashBlendFrame			= 6.0f;		// ダッシュの各アニメーションへのブレンドフレーム数
	const std::string kJumpStartAnimName	= "Combat_JumpStart";
	const std::string kJumpLoopAnimName		= "Combat_JumpLoop";
	const std::string kJumpEndAnimName		= "Combat_JumpEnd";

	// アビリティ（ATBコマンドメニューで選ぶ攻撃04・05。使うゲージの量・CTは PlayerConstants.h の kAbilityInfos）
	// アビリティの攻撃は、ボタンを押さなくても最後の段まで自動で出る
	constexpr float kAbilityDamageRate		= 1.5f;		// 普通の攻撃（倍率1.0）の何倍のダメージか。モードの倍率はかからない

	// ガード（止まっている時に ガード・ダッシュのキー（Shift ／ RB）を押している間）
	// アニメーション：Sequence2/08_Hit/14_Block の Block_Start / Loop / Hit / End
	const std::string kGuardStartAnimName	= "GuardStart";
	const std::string kGuardLoopAnimName	= "GuardLoop";
	const std::string kGuardHitAnimName		= "GuardHit";
	const std::string kGuardEndAnimName		= "GuardEnd";
	constexpr float kGuardBlendFrame		= 4.0f;		// ガードのアニメーションどうしのブレンドフレーム数
	constexpr float kGuardStartAnimSpeed	= 1.5f;		// 構えるのは素早く（すぐ守れるように）
	constexpr float kGuardEndCancelRate		= 0.3f;		// 構えを解くアニメーションがこの割合まで進んだら、次の行動に移れる
	constexpr float kGuardWalkBlendFrame	= 6.0f;		// ガード歩きの向きが変わった時のブレンドフレーム数
	constexpr float kGuardTurnAngle			= 12.0f;	// ガード中にカメラの方へ向きを変える速さ（度/フレーム）
	const std::string kGuardBreakAnimName	= "GuardBreak";

	// ダッシュ（移動中に ガード・ダッシュのキー（Shift ／ RB）を押している間）
	constexpr float kDashSpeedRate			= 1.6f;		// 走る速さの何倍か

	// 回避
	constexpr float kEvadeAnimSpeed			= 1.8f;		// 回避の再生速度（移動距離は同じまま、素早く転がる）
	constexpr float kEvadeReturnMoveRate	= 0.8f;		// 回避のアニメーションのこの割合まで進んだら、移動キーで走りに戻る（キャンセルはできない）
	constexpr float kEvadeBlendFrame		= 3.0f;		// 前の動き→回避のブレンドフレーム数

	// 攻撃
	constexpr float kAttackAnimSpeed		= 1.15f;	// 攻撃の再生速度（少し速くして、もっさりしないようにする）
	constexpr float kAttack5_3AnimSpeed		= 0.9f;		// 攻撃05の3段目だけは、少しゆっくり再生する
	constexpr float kAttackEvadeCancelFrame	= 12.0f;	// 攻撃のこのフレーム以降なら、回避でキャンセルできる
	constexpr float kAttackRecoveryFrame	= 6.0f;		// 次の攻撃に移れるフレームから、さらにこれだけたったら、移動で攻撃の後のすきをキャンセルできる

	// 回避した時に、パニッシャーからオペレーターに戻すか（true で FF7リメイクのクラウドと同じ動き）
	// false にすると、回避してもモードはそのまま
	constexpr bool	kEvadeReturnsToOperator	= false;	// パニッシャーのまま回避できるようにした（そのかわり、パニッシャーは攻撃が遅く、受けるダメージが多い）

	// 着地
	constexpr float kLandingCancelFrame		= 4.0f;		// 着地してからこのフレーム数がたったら、攻撃・回避・ジャンプに移れる
}
