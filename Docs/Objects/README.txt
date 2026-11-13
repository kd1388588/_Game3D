============================================================
BRAVER　オブジェクトごとの技術のファイル（目次と書き方）
============================================================
オブジェクト 1 つにつき、1 つのテキストファイルに「使った技術」を書いている。

■ 書き方のルール
  ・新しいオブジェクトを作ったら、同じ形のファイルをこのフォルダの分類の中に作る
  ・オブジェクトに技術を足した・変えた時は、そのファイルの「使った技術」を直し、
    「追加・変更の記録」に「日付：内容」を書き足す
  ・ファイルの形：
      見出し（オブジェクト名と、何をする物か）
      分類／ファイル（Src/Application/ からの場所）
      ■ 使った技術　　　 … 「技術の名前：どう使っているか」
      ■ 追加・変更の記録 … 「日付：内容」
  ・文字コードは BOM 付き UTF-8（ほかの Docs と同じ）
  ※用語の意味は ../TechTerms.txt、画面の要素の位置・色は ../UI_Elements.txt

■ 目次
  [Scene] アプリとシーン
      Application.txt           Application（アプリ全体）
      BaseScene.txt             BaseScene（シーンの共通部分）
      GameClearScene.txt        GameClearScene
      GameOverScene.txt         GameOverScene
      GameScene.txt             GameScene（ゲーム本編）
      SceneManager.txt          SceneManager（シーンの切り替え）
      TitleScene.txt            TitleScene（タイトル）
  [Player] プレイヤー
      Player.txt                Player（プレイヤー。役割ごとに 7 つのファイルに分けている）
  [Enemy] 敵
      Enemy.txt                 Enemy（ザコとボス）
      EnemyState.txt            EnemyState（敵の状態）
  [Character] キャラの共通部分
      BaseChara.txt             BaseChara（プレイヤー・敵の親）
  [Camera] カメラ
      CameraBase.txt            CameraBase（カメラの共通部分）
      FPSCamera.txt             FPSCamera（一人称のカメラ。今は使っていない）
      TPSCamera.txt             TPSCamera（ゲーム中のカメラ）
      TitleCamera.txt           TitleCamera（タイトルのカメラ）
  [Stage] ステージ
      Barrier.txt               Barrier（先へ進めない光の膜）
      BaseStageObject.txt       BaseStageObject（ステージの物の親）
      CathedralRuins.txt        CathedralRuins（大聖堂の廃墟のモデル。今は使っていない）
      Ground.txt                Ground（地面）
      HealFountain.txt          HealFountain（中ボスの部屋の回復の泉）
      KeyItem.txt               KeyItem（鍵の部屋に出てくる鍵）
      GroundMist.txt            GroundMist（地面の霧）
      RuinedRoad.txt            RuinedRoad（ステージのモデル）
      Sky.txt                   Sky（夜空と月）
      StageEnvironment.txt      StageEnvironment（光と霧の初期設定）
      StageLights.txt           StageLights（松明・かがり火・燭台の光）
      StageSettings.txt         StageSettings（ステージエディタ）
  [Effect] エフェクト
      Effect.txt                Effect（エフェクトのオブジェクト）
      EffectPlayer.txt          EffectPlayer（Effekseer のエフェクトを出す補助）
      EffectSettings.txt        EffectSettings（エフェクトエディタ）
  [UI] 戦闘の HUD・UI
      ATBCommandMenu.txt        ATBCommandMenu（右下の ATB コマンド）
      ATBGage.txt               ATBGage（左下のモード・ガード・ATB ゲージ・技の枠）
      BaseGage.txt              BaseGage（ゲージの共通部分）
      EnemyGage.txt             EnemyGage（画面の上の、敵の HP ゲージとブレイクゲージ）
      ControlsGuide.txt         ControlsGuide（右下の操作の一覧）
      HPGage.txt                HPGage（左下の HP ゲージ）
      StageMessage.txt          StageMessage（進み具合のメッセージと、鍵の表示）
      UIBackground.txt          UIBackground（画面いっぱいの背景）
      UIHeading.txt             UIHeading（見出しの画像）
      UIText.txt                UIText（文字）
  [Title] タイトル画面
      TitleChara.txt            TitleChara（背景で演技するキャラ）
      TitleEmber.txt            TitleEmber（舞い上がる火の粉と光の玉）
      TitleUI.txt               TitleUI（ロゴ・メニュー・操作説明）
  [Utility] 共通の道具
      GameTime.txt              GameTime（時間の速さ）
      HitStop.txt               HitStop（ヒットストップ）
      InputHelper.txt           InputHelper（入力）
      SoundPlayer.txt           SoundPlayer（音）
      SpriteUtil.txt            SpriteUtil（2D の補助）
  [Framework] フレームワークに足した物
      KdDebugGUI.txt            KdDebugGUI（デバッグのウィンドウ）
      KdEffekseerManager.txt    KdEffekseerManager（Effekseer の管理）
      KdPostProcessShader.txt   KdPostProcessShader（画面全体の加工）
  [Tools] Python のツール
      Tools.txt                 Tools（ステージ・画像・音・エフェクトを作る Python のスクリプト）
