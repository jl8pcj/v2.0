# v2.0　switch2に対応
HORIコンに偽装して認識させる。

1.BOOTを押しながらUSBをPCに接続
2.ArduinoIdeで"HORIPAD_piezo＿Contoroller.ide"を開く
3.ボード＿"Waveshare RP2040 Zero"　ポート＿"接続されているポート"を選択
4.ツール＿USB stack＿"Adafruits Tiny USB"を選択
5.書き込み

switchに繋ぐと有線HORIコンとして認識します。
GP26につながれているpiezoをたたくと"L"
GP27につながれているpiezoをたたくと"右"
GP28につながれているpiezoをたたくと"Y"
GP29につながれているpiezoをたたくと"R"　が押されます。

ボードに電源が入ると自動でキャリブレーションが行われます。

【感度調整】
#define THRESHOLD_OFFSET 120　を書き換える。
推奨値
値	状態
80	超敏感
120	標準（今）
180	強打のみ
250	誤爆ほぼゼロ
