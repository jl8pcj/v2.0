# v2.1　switch2に対応
HORIコンに偽装して認識させる。<BR>
<BR>
1.BOOTを押しながらUSBをPCに接続<BR>
2.ArduinoIdeで"HORIPAD_piezo＿Contoroller.ide"を開く<BR>
3.ボード＿"Waveshare RP2040 Zero"　ポート＿"接続されているポート"を選択<BR>
4.ツール＿USB stack＿"Adafruits Tiny USB"を選択<BR>
5.書き込み<BR>
<BR><BR>
switchに繋ぐと有線HORIコンとして認識します。<BR>
GP26につながれているpiezoをたたくと"L"<BR>
GP27につながれているpiezoをたたくと"右"<BR>
GP28につながれているpiezoをたたくと"Y"<BR>
GP29につながれているpiezoをたたくと"R"　が押されます。<BR>
<BR>
ボードに電源が入ると自動でキャリブレーションが行われます。<BR>
<BR>
【感度調整】<BR>
#define THRESHOLD_OFFSET 120　を書き換える。<BR>
推奨値<BR>
値	状態<BR>
80	超敏感<BR>
120	標準（今）<BR>
180	強打のみ<BR>
250	誤爆ほぼゼロ<BR>
