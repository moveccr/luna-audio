options
  -f<freq>
        output frequency(Hz)
        postfix 'k' = kHz
  -i<format>
        set input format
  -o<format>
        set output format
  -O<file>
        output file
  -v    verbose level +1
  -h    show help

format (ignore case)
  LUNAPAM2  LUNAPAM2 format
  LUNAPCM1  LUNAPCM1 format
  PSGPCM1   PSGPCM u8 format
  WAV       WAV file format
  AU        (not implemented)

foo.wav
-i WAV -o LUNAPAM foo.wav
  WAV ファイルフォーマットで foo.wav から入力し、
  LUNAPAM フォーマットで XP デバイス経由で音声出力します。
  この動作がデフォルトです。

-i LUNAPAM -o WAV -O out.wav lunapam.pam
  LUNAPAM フォーマットの lunapam.pam ファイルから入力し、
  WAV ファイルフォーマットで out.wav ファイルへ出力します。

-i WAV -o LUNAPCM1 -O - -- -
  WAV ファイルフォーマットで標準入力から入力し、
  LUNAPCM1 フォーマットで標準出力に出力します。
  出力はバイナリなので、通常はパイプすることになります。


フォーマット
  LUNAPAM2
    2 bytes / sample
    4 bit 2ch PAM (120 level?)
    パックすると、XP (Z80) での分解が遅い
    PAM 周波数は、XP (Z80) 側の実装に依存する。
    ホスト側では指定できない。

    char magic[4] = "LPA2"
    int32BE sampleCount
	int32BE freq
    {
       uint8 ch0;
       uint8 ch1;
    } [sampleCount]

    ch0, ch1
    bit 76543210
        0000SSSS
      SSSS = PSG volume data

LUNAPCM1
    1 byte / sample, 4 bit PSG Volume (16 level)

    char magic[4] = "LPC1"
    int32BE sampleCount
	int32BE freq
    {
       uint8 ch0;
    } [sampleCount]

    ch0
    bit 76543210
        0000SSSS
      SSSS = PSG volume data

LUNAPCM2
    2 bytes / sample (120 level)
    LUNA-I では PSG の出力がアナログ加算になっていないので、
    正しく再生できない。
    改造された LUNA-88K、または出力ピンを外して改造したLUNA-I で、
	PSG 出力をアナログ加算合成した場合のために。

    char magic[4] = "LPC2"
    int32BE sampleCount
	int32BE freq
    {
       uint8 ch0;
       uint8 ch1;
    } [sampleCount]

    ch0, ch1
    bit 76543210
        0000SSSS
      SSSS = PSG volume data

LUNAPCM3
    3 bytes / sample (560 level)
    LUNA-I では PSG の出力がアナログ加算になっていないので、
    正しく再生できない。
    改造された LUNA-88K、または出力ピンを外して改造したLUNA-I で、
	PSG 出力をアナログ加算合成した場合のために。

    char magic[4] = "LPC3"
    int32BE sampleCount
	int32BE freq
    {
       uint8 ch0;
       uint8 ch1;
       uint8 ch2;
    } [sampleCount]

    ch0, ch1, ch2
    bit 76543210
        0000SSSS
      SSSS = PSG volume data

PSGPCM1
    1 byte / sample
    ULINEAR8 を XP (Z80) 側で展開する。
    再生アルゴリズムは XP 側の実装による。

    char magic[4] = "LPC1"
    int32BE sampleCount
	int32BE freq
    {
       uint8 ch0;
    } [sampleCount]

    ch0
    bit 76543210
        UUUUUUUU
      UUUUUUUU = ULINEAR8



