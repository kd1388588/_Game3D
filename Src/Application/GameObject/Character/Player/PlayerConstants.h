#pragma once

// ============================================================
// Player の調整用の数値
// ・Player*.cpp（Player クラスの関数を書いたファイル）の中だけで include する
//   Player.h には書かない（Player.h を include する他のファイルに見せない・
//   数値を変えた時に、関係ないファイルまでビルドし直しにならないようにするため）
// ・ATB の技の CT やモードの倍率など、調整したい数値はここにまとめてある
// ============================================================
namespace PlayerConst
{
	// 移動
	constexpr float kMoveSpeed				= 0.15f;
	constexpr float kMaxTurnAngle			= 18.0f;	// 1フレームの最大旋回角度（度。大きいほど素早く向きを変える）
	constexpr float kMinTurnAngle			= 0.1f;		// これ未満の角度差は旋回しない

	// ステータス
	constexpr int	kMaxHp = 100;
	constexpr int	kDamageInvincibleFrame = 30;		// 被弾後の無敵時間
	constexpr int	kHeavyDamage = 20;		// これ以上のダメージで仰け反る
	// 二刀流なので、1振りで左右の剣がそれぞれ当たる（多段ヒット）。そのぶん1回のダメージは低め
	constexpr int	kAttackDamage = 3;
	constexpr int	kCriticalAttackDamage = 5;
	constexpr int	kMultiHitInterval = 10;	// 同じ剣が同じ敵にもう一度当たれるまでのフレーム数（回転する攻撃などで何度も当たる）
	constexpr int	kAttackTypeNum = 5;		// 攻撃パターンの数

	// 先行入力（ボタンを押してから、何フレームの間「押された」として受け付けるか。60フレーム = 1秒）
	constexpr int	kInputBufferFrame		= 10;

	// バトルモード
	constexpr int	kOperatorAttackType		= 1;		// オペレーターの左クリックの攻撃（攻撃01）
	constexpr int	kPunisherAttackType		= 2;		// パニッシャーの左クリックの攻撃（攻撃02）
	//	 モードの役割分担（両方を使い分けて戦うように、得意・不得意をはっきり分ける）
	//   オペレーター：攻撃が速く、ゲージがよくたまる。そのかわり威力は低い
	//   パニッシャー：攻撃が遅く、移動も遅い。ゲージはたまらない。そのかわり威力が高い
	//   ATBの技（04・05）：モードに関係なく、普通の攻撃より少し強い（攻撃・回避でためて使う）
	constexpr float kOperatorDamageRate		= 0.6f;		// オペレーターのダメージの倍率
	constexpr float kPunisherDamageRate		= 1.6f;		// パニッシャーのダメージの倍率
	constexpr float kOperatorAttackSpeedRate = 1.2f;	// オペレーターの攻撃の再生速度の倍率
	constexpr float kPunisherAttackSpeedRate = 0.8f;	// パニッシャーの攻撃の再生速度の倍率（回避できるようにしたかわりに、0.9 から遅くした）
	constexpr float kPunisherTakenDamageRate = 1.3f;	// パニッシャーの時に受けるダメージの倍率（回避できるようにしたかわりに、多く受ける）
	constexpr float kPunisherMoveRate		= 0.6f;		// パニッシャーの移動の速さ（オペレーターを1.0とした倍率）

	// ATBゲージのたまり方（1本 = 100）
	// ※時間ではたまらない（攻撃を当てる・回避することでだけたまる）
	constexpr float kAtbHitGain				= 2.5f;		// 攻撃を当てた時にたまる量（多段ヒットで何度も当たるので少なめ）
	constexpr float kOperatorAtbHitRate		= 1.15f;		// オペレーターの時、当てた時にたまる量の倍率（よくたまる）
	constexpr float kPunisherAtbHitRate		= 0.75f;	// パニッシャーの時の倍率（少なめ）
	constexpr float kAtbCriticalRate		= 1.25f;		// クリティカルの時、当てた時にたまる量の倍率
	constexpr float kAtbEvadeGain			= 5.0f;		// 回避した時にたまる量（20回で1本）

	// ATBコマンドメニュー
	constexpr float	kCommandTimeScale		= 0.1f;		// メニューを開いている間の時間の速さ（10分の1）

	// アビリティ（メニューで選ぶ。攻撃04・05）
	struct AbilityInfo
	{
		int			attackType;		// 出す攻撃の種類
		int			atbCost;		// 使うATBゲージの本数
		int			coolTime;		// CT（使った後、もう一度使えるまでのフレーム数）
		const char*	name;			// ログに出す名前
	};
	// メニューの上から順番（AbilitySlot の順番）
	constexpr AbilityInfo kAbilityInfos[] =
	{
		{ 5, 1, 60 * 15, "Attack1" },	// 攻撃05：ゲージ1本、CT 15秒（上）
		{ 4, 2, 60 * 20, "Attack2" },	// 攻撃04：ゲージ2本、CT 20秒（下）
	};
	constexpr int	kModeLockAttackType		= 5;		// 使った後にオペレーターに固定する技（攻撃05）
	constexpr int	kModeLockFrame			= 60 * 5;	// 攻撃05を使った後、オペレーターに固定する時間（5秒）
	constexpr int	kModeSwitchCoolFrame	= 60 * 3;	// モードを切り替えた後、次に切り替えられるまでの時間（3秒）
	constexpr int	kAbilityStunFrame		= 50;		// ATBコマンドの技が当たった敵が、のけぞり続けるフレーム数（当たるたびに延びる）

	// ヒットストップ（攻撃が当たった瞬間に止めるフレーム数と、カメラの揺れの強さ）
	// ※プレイヤーの攻撃はすべて AttackOBB で、敵の攻撃は OnDamage で、ここの値を使う
	constexpr int	kHitStopFrame			= 4;		// 普通の攻撃
	constexpr int	kCriticalHitStopFrame	= 7;		// クリティカル
	constexpr int	kHeavyHitStopBonus		= 2;		// パニッシャー・アビリティの時に足すフレーム数
	constexpr float	kHitShakePower			= 0.04f;	// カメラの揺れ（普通）
	constexpr float	kCriticalHitShakePower	= 0.08f;	// カメラの揺れ（クリティカル）
	constexpr int	kDamageHitStopFrame		= 6;		// 敵の攻撃を受けた時
	constexpr float	kDamageShakePower		= 0.1f;
	constexpr int	kGuardHitStopFrame		= 4;		// 敵の攻撃をガードした時
	constexpr float	kGuardShakePower		= 0.05f;

	// 回避中に攻撃を受けた時のダメージの倍率（半分）
	constexpr float	kEvadeDamageRate		= 0.5f;

	// ガード
	constexpr float	kGuardDamageRate		= 0.2f;		// ガード中に受けるダメージの倍率
	constexpr float	kGuardMoveRate			= 0.5f;		// ガードしながら歩く速さ（普段の走る速さの何倍か）
	constexpr float	kGuardWalkMinAnimSpeed	= 0.6f;		// ガード歩きのアニメーションの再生速度の範囲
	constexpr float	kGuardWalkMaxAnimSpeed	= 1.8f;		// （移動の速さに合わせて変え、足が滑って見えないようにする）

	// ガードの耐久値（攻撃を受け止めるたびに減り、0 になるとガードを崩される）
	constexpr float	kGuardDurabilityMax		= 100.0f;
	constexpr float	kGuardCostPerDamage		= 3.0f;		// 受け止めたダメージ × これだけ減る（ダメージ10なら30。4回で崩される）
	constexpr float	kGuardCriticalCostRate	= 1.5f;		// クリティカルの攻撃は多く減る
	constexpr int	kGuardRegenDelay		= 120;		// 最後に受け止めてから回復し始めるまでのフレーム数（2秒）
	constexpr float	kGuardRegenSpeed		= 0.15f;	// 1フレームに回復する量（0 から満タンまで約11秒）
	constexpr int	kGuardBreakFrame		= 60 * 2;	// 崩された後、ガードできない時間（2秒。その間も耐久値は少しずつ回復する）
	constexpr int	kGuardBreakHitStopFrame	= 10;		// 崩された時のヒットストップ
	constexpr float	kGuardBreakShakePower	= 0.15f;

	// 効果音（ファイル名は Utility/SoundPlayer.h の SoundName）
	// pitch：音の高さ（-1.0 低い ～ 0 そのまま ～ 1.0 高い）
	constexpr float	kSlashSEVolume			= 0.7f;		// 斬った音（普通の攻撃）
	constexpr float	kSlashSEHeavyPitch		= -0.2f;	// パニッシャー・アビリティの重い攻撃は低く
	constexpr float	kSlashSECriticalPitch	= -0.35f;	// クリティカルはもっと低く
	constexpr float	kSlashSEPitchRandom		= 0.1f;		// 毎回少し高さを変える（同じ音の繰り返しが機械的に聞こえないように）
	constexpr float	kGuardSEVolume			= 0.6f;		// 盾で受け止めた音
	constexpr float	kGuardSEPitch			= 0.4f;		// 受けた時より高く軽い音にする
	constexpr float	kGuardBreakSEPitch		= -0.4f;	// 盾が砕けた音は低く重く

	// 足音：足の骨（つま先の付け根）が、地面に着いている高さからこれより上がったら「離れた」、
	// その後これより下がったら「着いた」とみなして1歩鳴らす（間を空けて、細かい揺れで何度も鳴らないように）
	constexpr float	kFootstepLiftHeight		= 0.10f;
	constexpr float	kFootstepPlantHeight	= 0.04f;
	constexpr int	kFootstepCoolFrame		= 6;		// 1歩鳴らしてから、次を鳴らせるまでのフレーム数
	constexpr float	kFootGroundRiseSpeed	= 0.001f;	// 地面に着いている高さを、1フレームに上げていく量

	// ガードしながら歩くアニメーション（Sequence2/03_Walk/03_Walk_Block の Loop）
	// angle：キャラの向きから見て、どの方向へ歩くアニメーションか（度。右が＋）
	//        ※実際の向きは、読み込んだ時にアニメーションの動きから求め直す（左右の取り違えを防ぐため）
	struct GuardWalkAnimDef
	{
		const char*	name;
		const char*	folder;
		const char*	dir;
		float		angle;
	};
	const GuardWalkAnimDef kGuardWalkAnimDefs[] =
	{
		{ "GuardWalk_F_0",		"01_Walk_Block_F_0",	"F_0",		0.0f },
		{ "GuardWalk_F_L_45",	"02_Walk_BLock_F_L_45",	"F_L_45",	-45.0f },
		{ "GuardWalk_F_R_45",	"03_Walk_BLock_F_R_45",	"F_R_45",	45.0f },
		{ "GuardWalk_F_L_90",	"04_Walk_BLock_F_L_90",	"F_L_90",	-90.0f },
		{ "GuardWalk_F_R_90",	"05_Walk_BLock_F_R_90",	"F_R_90",	90.0f },
		{ "GuardWalk_B_180",	"06_Walk_Block_B_180",	"B_180",	180.0f },
		{ "GuardWalk_B_L_45",	"07_Walk_Block_B_L_45",	"B_L_45",	-135.0f },
		{ "GuardWalk_B_R_45",	"08_Walk_Block_B_R_45",	"B_R_45",	135.0f },
	};
	constexpr float	kMinWalkTravel			= 0.05f;	// アニメーションでこれ以上進んでいれば、その動きから向きを求める

	// ダッシュ
	constexpr float	kDashMoveRate			= 1.25f;		// ダッシュ中の移動の速さの倍率

	// 敵との押し出し（敵の体を通り抜けて、反対側に回り込まないようにする）
	constexpr float	kEnemyBodyRadiusRate	= 0.55f;	// 敵の体の半径（敵の大きさに掛ける。ザコ m・ボス 1.4m くらい）
	constexpr float	kPlayerBodyRadius		= 0.4f;		// プレイヤーの体の半径

	// ロックオン
	constexpr float	kLockOnRange			= 10.0f;	// この距離に近づいた敵に、自動でロックオンする（一番近い敵）
	constexpr float	kLockOnLoseMargin		= 0.5f;		// ロックオンを外すのは 10m ＋ これだけ離れた時（境目で付いたり外れたりしないように）

	// リムライト（バトルモードを切り替えた時に1回だけ光らせる）
	constexpr float	kLimPowSpeed			= 0.1f;		// 1フレームで pow を増やす量
	constexpr float	kLimPowMax				= 10.0f;	// pow がここまで来たら消す（0.1ずつなので100フレーム）
	constexpr float	kLimFadeStartRate		= 0.4f;		// 全体のこの割合を過ぎたら、色をだんだん暗くして消す
	const Math::Vector3 kOperatorLimColor	= { 0.0f, 0.5f, 1.0f };		// オペレーター：青
	const Math::Vector3 kPunisherLimColor	= { 1.0f, 0.3f, 0.0f };		// パニッシャー：オレンジ

	// いつもの薄いリムライト（モードを切り替えた時の光とは別に、体全体のふちを今のモードの色で薄く光らせておく）
	constexpr float		kBaseLimBright		= 0.2f;		// 明るさ（モードの色にかける）
	constexpr float		kBaseLimPow			= 3.0f;			// 鋭さ（大きいほどふちだけが光る）

	// ガードの盾のエフェクトを出す位置（体の前）
	constexpr float	kGuardEffectHeight		= 1.0f;		// 高さ
	constexpr float	kGuardEffectForward		= 0.6f;		// 体からの距離

	// パニッシャーのオーラ（パニッシャーの間、体のまわりでオレンジの光がゆらめく）
	// 腰を中心に少しずつ大きくした体を何枚か重ね、裏側だけを光の色で描く
	constexpr int	kAuraLayerNum			= 3;		// 重ねる枚数
	constexpr float	kAuraBaseScale			= 1.04f;	// 1枚目の大きさ
	constexpr float	kAuraScaleStep			= 0.045f;	// 1枚ごとに大きくする量
	constexpr float	kAuraStretchY			= 0.04f;	// 外側ほど縦に伸ばす量（炎のように上へのびて見せる）
	constexpr float	kAuraRiseY				= 0.04f;	// 外側ほど上にずらす量
	constexpr float	kAuraWaveScale			= 0.02f;	// 大きさのゆらぎ
	constexpr float	kAuraWaveSpeed			= 0.45f;	// 大きさのゆらぎの速さ
	constexpr float	kAuraFlickerSpeed		= 0.9f;		// 明るさのちらつきの速さ
	constexpr float	kAuraFlickerRange		= 0.35f;	// 明るさのちらつきの強さ
	constexpr float	kAuraFadeSpeed			= 0.06f;	// モードを切り替えた時に出る・消える速さ
	const Math::Vector3 kAuraColor			= { 1.6f, 0.6f, 0.15f };	// オレンジ

	// 接地判定に使う重力の上限（落下し始めの1フレーム分）
	constexpr float kOnGroundGravityMax = 0.011f;

	// 剣の刃の位置（剣のローカルY軸上。攻撃判定の箱と同じ範囲）
	// ※別ファイルの剣を持たせる場合に使う。モデルに剣が含まれている場合はメッシュから自動で求める
	constexpr float kSwordBladeBaseY		= 0.05f;	// 刃の根元
	constexpr float kSwordBladeTipY			= 1.15f;	// 刃の先端
	constexpr float kSwordOBBHalfWidth		= 0.1f;		// 攻撃判定の箱の太さ（半分）
	// 剣が速く動いた時（回転攻撃など）は、前のフレームとの間の位置にも判定の箱を置く
	// （1フレームに剣先が大きく動くと、箱と箱の間に敵がいても当たらなかったため）
	constexpr float kSwordSweepStep			= 0.15f;	// 剣先がこの距離だけ動くごとに、間に箱を1つ置く
	constexpr int	kSwordSweepMaxStep		= 8;

	// モデルに含まれる剣のメッシュ（上から順に探して、最初に見つかったものを使う）
	const std::vector<std::string> kBladeMeshNodeNamesR	= { "SM_Blade_R" };
	const std::vector<std::string> kBladeMeshNodeNamesL	= { "SM_Blade_L" };
	constexpr float kBladeBaseOffset		= 0.1f;		// 手から刃の根元（つば）までの距離

	// 剣の軌跡
	constexpr float kSwordTrailWidthRate	= 1.0f;		// 帯の幅の倍率（1.0で刃の根元から先端まで）
	constexpr UINT	kSwordTrailLength		= 15;		// 軌跡を残すフレーム数
	constexpr int	kSwordTrailSubdivision	= 6;		// 1フレームの間を何分割して補間するか（多いほどなめらか）

	// 軌跡テクスチャ（横：刃の先端→根元、縦：新しい→古い）
	constexpr int	kSwordTrailTexWidth		= 64;
	constexpr int	kSwordTrailTexHeight	= 256;
	const Math::Vector3 kOperatorTrailColor	= { 0.25f, 0.65f, 1.0f };	// オペレーターの軌跡の色（青）
	const Math::Vector3 kPunisherTrailColor	= { 1.0f, 0.4f, 0.08f };	// パニッシャーの軌跡の色（オレンジ）
	const Math::Vector3 kSwordTrailCoreColor = { 1.0f, 1.0f, 1.0f };	// 刃先付近の芯の色
	// 軌跡の画像（Tools/MakeTrailTexture.py で作成。無い時は上の色からプログラムで作る）
	const std::string kOperatorTrailTexPath	= "Asset/Textures/Trail/SwordTrail_Operator.png";
	const std::string kPunisherTrailTexPath	= "Asset/Textures/Trail/SwordTrail_Punisher.png";
	constexpr float kSwordTrailBaseBrightness = 0.15f;	// 刃の根元側の明るさ（先端を1.0とした割合）

	// エフェクト
	constexpr float kHitEffectScale			= 0.35f;	// 攻撃が敵に当たった時のエフェクト（斬撃1本）の大きさ
	constexpr float kCriticalHitEffectScale	= 0.5f;		// クリティカル時のエフェクトの大きさ
	constexpr float kHitSlashBaseAngle		= -70.0f;	// _Sword1-1 の斬撃の、エフェクトの中での傾き（Angle ノードの Z 回転）
	constexpr float kSword5SlashScale		= 0.22f;	// 05の1段目のヒット（_Sword5 の斬撃1本）の大きさ
	constexpr float kSword5SpinScale		= 0.15f;	// 05の2段目のヒット（回転する斬撃）の大きさ
	constexpr float kFinisherEffectScaleRate	= 1.4f;		// 決めの攻撃の専用エフェクトの大きさ（斬撃1本の大きさに掛ける。前と同じ 0.5 くらい）

	// カメラの注視点を腰の骨から下げる量
	constexpr float kCameraTargetOffsetY	= -0.9f;
	constexpr float kCameraFollowRateY		= 0.25f;	// ジャンプなどで高さが変わった時、カメラが付いていく速さ（0～1）
	constexpr float kCameraRiseDeadZone		= 0.15f;	// 腰がこれ以上上がった時だけカメラも上げる（攻撃の小さな上下では動かさない）

	// モデル・アニメーション（UEでSK_Assassinにリターゲットして書き出したもの）
	const std::string kPlayerAssetDir		= "Asset/Models/GameObject/Player/New/";
	const std::string kPlayerModelPath		= kPlayerAssetDir + "CharaBase/SK_Assassin.gltf";
	const std::string kAnimDir				= kPlayerAssetDir + "Sequence2/";

	// 武器のモデル（空文字なら読み込まない）
	// ※SK_Assassin には剣（SM_Blade）が最初から含まれているので、別の剣は読み込まない
	const std::string kSwordModelPath		= "";

	// 武器を付けるノード（上から順に探して、最初に見つかったものを使う）
	const std::vector<std::string> kSwordNodeNamesR		= { "Blade_R", "Weapon_R", "hand_r" };
	const std::vector<std::string> kSwordNodeNamesL		= { "Blade_L", "Weapon_L", "hand_l" };

	// 剣を投げる攻撃（投げた剣がまっすぐ前へ飛ぶように補正する）
	const std::string kThrowAnimName		= "Attack_5_2";
	constexpr float kThrowCorrectStartDist	= 0.3f;		// 手からこの距離までは補正しない（手に持っている間）
	constexpr float kThrowCorrectFullDist	= 1.0f;		// 手からこの距離以上離れたら、補正を全部かける

	// 着地アニメーション（両足の接地フレームを事前に計算する）
	// ※プレイヤーは常に剣を抜いているので、剣を持った（Combat_）アニメーションだけを使う
	const std::vector<std::string> kLandingAnimNames	= { "Combat_JumpEnd" };
	const std::vector<std::string> kTakeoffAnimNames	= { "Combat_JumpStart" };
	constexpr float kMaxTakeoffFrame		= 60.0f;	// 踏み切りがこれより遅いアニメーションは、待たずにすぐ跳ぶ
	// （ためが長いアニメーションでも、ためを途中から再生するので操作は遅れない（PlayerStateConstants.h の kJumpWindupFrames））

	// 04の4段目：跳び上がって、落ちてきて剣を地面に叩きつける（AS_Combo_Attack_04_04。49フレーム目あたりで叩きつける）
	const std::string kJumpSlamAnimName		= "Attack_4_4";
	constexpr float kJumpSlamTolerance		= 0.15f;	// 跳んだ後の一番低い剣先の高さから、これ以内に来たら「叩きつけた」とみなす

	// 足が地面に着いている・離れたかを調べる骨
	// つま先の付け根（ball）を優先して使う。足首（foot）は、着地でひざを曲げるとかかとが浮いて高くなるので、
	// 足首で調べると「まだ着地していない」と間違えてしまう（ball が無いモデルの時だけ foot を使う）
	const std::vector<std::string> kToeNodeNames		= { "ball_l", "ball_r" };
	const std::vector<std::string> kFootNodeNames		= { "foot_l", "foot_r" };

	// 空中ループのアニメーション（空中で腰が上がってよい高さを、このアニメーションの腰の高さにする）
	const std::string kAirLoopAnimName		= "Combat_JumpLoop";
}
