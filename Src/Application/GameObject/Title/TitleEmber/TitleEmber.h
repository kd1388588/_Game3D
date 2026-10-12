#pragma once

// タイトル画面で、画面の下から舞い上がり続ける火の粉と光の玉（2D）
class TitleEmber : public KdGameObject
{
public:

	TitleEmber() {}
	~TitleEmber() override {}

	void Init()			override;
	void Update()		override;
	void DrawSprite()	override;

private:

	// 粒1つ分
	struct Particle
	{
		Math::Vector2	pos;					// 画面中央が原点、上が+Y
		Math::Vector2	velocity;				// 1フレームの移動量
		Math::Color		color;
		float			size = 0.0f;			// 直径（ピクセル）
		float			swayPhase = 0.0f;		// 左右に揺れる周期のずれ
		float			swayWidth = 0.0f;		// 左右に揺れる幅
		int				life = 0;				// 残りフレーム
		int				maxLife = 0;
	};

	// 粒を作り直す（isFirst なら画面全体に散らばせ、それ以外は画面の下から出す）
	void SpawnEmber(Particle& p, bool isFirst);
	void SpawnBokeh(Particle& p, bool isFirst);

	// 粒の移動・寿命の更新（寿命が尽きたら false）
	static bool UpdateParticle(Particle& p, int frame);

	// 寿命に応じた明るさ（出てくる時・消える時はふわっと変化させる）
	static float CalcFade(const Particle& p);

	void DrawParticles(const std::vector<Particle>& particles, float brightness) const;

	std::vector<Particle>		m_embers;			// 小さな火の粉
	std::vector<Particle>		m_bokehs;			// 大きくぼやけた光の玉
	std::shared_ptr<KdTexture>	m_glowTex = nullptr;
	Math::Vector2				m_screenSize = { 1280.0f, 720.0f };
	int							m_frame = 0;
};
