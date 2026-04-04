# v2.1　派生しました
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
✔ Piezo ごとに感度（threshold）を個別調整
L / Y は軽打対応（+80）

HR は中間（+90）

R は誤反応しやすいので高め（+120）

✔ Piezo ごとにデバウンス時間も個別設定
L / Y は 40ms → 軽打でも反応しやすい

R は 70ms → 誤反応防止

HR は 50ms → 標準
