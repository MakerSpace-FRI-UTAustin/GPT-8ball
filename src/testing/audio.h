AnalogAudioStream adc;
// ConverterScaler<int16_t> scaler(1.0, -26427, 32700 );

//Audio Information. May vary.
int sampleRate = 44000 ;
int channels = 1;
int samplingBits = 16;
int duration = 5;
int fileSize = sampleRate * (samplingBits / 8) * channels * duration;
int blockSize = 1024;
AudioInfo info(sampleRate, channels, samplingBits);

File audioFile;
const char* filename = "/speech.wav";

WAVEncoder wav;
EncodedAudioStream out(&audioFile, &wav);
StreamCopy copier(out, adc);

bool recordSetup();
bool recordClip();



