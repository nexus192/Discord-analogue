#include <portaudio.h>

#include <functional>
#include <iostream>
#include <vector>

class AudioCapture {
 public:
  AudioCapture() : stream_(nullptr) { Pa_Initialize(); }

  ~AudioCapture() {
    if (stream_) {
      Pa_CloseStream(stream_);
    }
    Pa_Terminate();
  }

  void start_capture(std::function<void(const std::vector<float>&)> callback) {
    data_callback_ = callback;

    Pa_OpenDefaultStream(&stream_,
                         1,          // 1 input channel
                         0,          // 0 output channels
                         paFloat32,  // 32 bit floating point output
                         44100,      // Sample rate
                         256,        // Frames per buffer
                         audio_callback, this);

    Pa_StartStream(stream_);
  }

  void stop_capture() {
    if (stream_) {
      Pa_StopStream(stream_);
    }
  }

 private:
  static int audio_callback(const void* input, void* output,
                            unsigned long frameCount,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags, void* userData) {
    auto* capture = static_cast<AudioCapture*>(userData);
    const float* inputBuffer = static_cast<const float*>(input);

    std::vector<float> audioData(inputBuffer, inputBuffer + frameCount);
    capture->data_callback_(audioData);

    return paContinue;
  }

  PaStream* stream_;
  std::function<void(const std::vector<float>&)> data_callback_;
};
