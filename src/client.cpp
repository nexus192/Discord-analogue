#include <portaudio.h>

#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <vector>

using boost::asio::ip::tcp;

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

class Client {
 public:
  Client(boost::asio::io_context& io_context, const std::string& host,
         const std::string& port)
      : socket_(io_context), audio_capture_() {
    connect(host, port);
  }

  void start_audio() {
    audio_capture_.start_capture(
        [this](const std::vector<float>& audioData) { send_audio(audioData); });
    receive_audio();
  }

 private:
  void connect(const std::string& host, const std::string& port) {
    tcp::resolver resolver(socket_.get_executor());
    auto endpoints = resolver.resolve(host, port);
    boost::asio::async_connect(
        socket_, endpoints,
        [this](boost::system::error_code ec, tcp::endpoint) {
          if (!ec) {
            std::cout << "Connected to server" << std::endl;
            start_audio();
          }
        });
  }

  void send_audio(const std::vector<float>& audioData) {
    std::vector<char> buffer(audioData.size() * sizeof(float));
    std::memcpy(buffer.data(), audioData.data(), buffer.size());

    boost::asio::async_write(
        socket_, boost::asio::buffer(buffer),
        [this](boost::system::error_code ec, std::size_t /*length*/) {
          if (ec) {
            std::cerr << "Error sending audio: " << ec.message() << std::endl;
          }
        });
  }

  void receive_audio() {
    socket_.async_read_some(
        boost::asio::buffer(receive_buffer_),
        [this](boost::system::error_code ec, std::size_t length) {
          if (!ec) {
            std::cout << "Received " << length << " bytes of audio data"
                      << std::endl;

            // Выводим первые несколько байт для отладки
            std::cout << "First few bytes: ";
            for (size_t i = 0; i < std::min(length, size_t(10)); ++i) {
              std::cout << static_cast<int>(receive_buffer_[i]) << " ";
            }
            std::cout << std::endl;

            // Продолжаем получать данные
            receive_audio();
          } else if (ec == boost::asio::error::eof) {
            std::cout << "Server closed the connection." << std::endl;
          } else {
            std::cerr << "Error receiving audio: " << ec.message() << std::endl;
          }
        });
  }

  tcp::socket socket_;
  AudioCapture audio_capture_;
  std::array<char, 1024> receive_buffer_;
};

int main() {
  try {
    boost::asio::io_context io_context;
    Client client(io_context, "localhost", "8080");
    io_context.run();
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }

  return 0;
}
