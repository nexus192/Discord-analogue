#include <portaudio.h>

#include <functional>
#include <iostream>
#include <vector>

class AudioCapture {
 public:
  AudioCapture() : stream_(nullptr) {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
      std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
    }
  }

  ~AudioCapture() {
    if (stream_) {
      Pa_CloseStream(stream_);
    }
    Pa_Terminate();
  }

  void start_capture(std::function<void(const std::vector<float>&)> callback) {
    data_callback_ = callback;

    PaError err =
        Pa_OpenDefaultStream(&stream_,
                             1,          // 1 input channel
                             0,          // 0 output channels
                             paFloat32,  // 32 bit floating point output
                             44100,      // Sample rate
                             256,        // Frames per buffer
                             audio_callback, this);

    if (err != paNoError) {
      std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
      return;
    }

    err = Pa_StartStream(stream_);
    if (err != paNoError) {
      std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
      return;
    }

    std::cout << "Audio capture started." << std::endl;
  }

  void stop_capture() {
    if (stream_) {
      PaError err = Pa_StopStream(stream_);
      if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
      } else {
        std::cout << "Audio capture stopped." << std::endl;
      }
    }
  }

 private:
  static int audio_callback(const void* input, void* output,
                            unsigned long frameCount,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags, void* userData) {
    auto* capture = static_cast<AudioCapture*>(userData);
    const float* inputBuffer = static_cast<const float*>(input);

    // Копируем данные из входного буфера в вектор
    std::vector<float> audioData(inputBuffer, inputBuffer + frameCount);

    // Вызываем пользовательский callback с захваченными данными
    capture->data_callback_(audioData);

    return paContinue;
  }

  PaStream* stream_;
  std::function<void(const std::vector<float>&)> data_callback_;
};
