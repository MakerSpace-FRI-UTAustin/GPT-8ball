AnalogAudioStream adc;
ConverterScaler<int16_t> scaler(1.0, -26427, 32700);

//Audio Information. May vary.
int sampleRate = 43000;
int channels = 1;
int samplingBits = 16;
int duration = 3;
int fileSize = (samplingBits * sampleRate * channels * duration) / 8;
int blockSize = 1024;
AudioInfo info(sampleRate, channels, samplingBits);

File audioFile;
const char* filename = "/speech.wav";

EncodedAudioStream out(&audioFile, new WAVEncoder());
StreamCopy copier(out, adc);

void recordSetup();
void recordClip();
