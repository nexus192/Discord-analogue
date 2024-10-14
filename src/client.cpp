#include <portaudio.h>

#include <atomic>
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
  Client(boost::asio::io_context& io_context)
      : io_context_(io_context),
        socket_(io_context),
        audio_capture_(),
        is_connected_(false),
        is_capturing_(false) {}

  void connect(const std::string& host, const std::string& port) {
    std::cout << "Attempting to connect to " << host << ":" << port << "..."
              << std::endl;
    tcp::resolver resolver(io_context_);
    auto endpoints = resolver.resolve(host, port);
    boost::asio::async_connect(
        socket_, endpoints,
        [this, host, port](boost::system::error_code ec, tcp::endpoint) {
          if (!ec) {
            is_connected_ = true;
            std::cout << "Connected to " << host << ":" << port << std::endl;
          } else {
            std::cerr << "Connection failed: " << ec.message() << std::endl;
          }
        });

    // Ждем немного, чтобы асинхронное подключение успело выполниться
    std::this_thread::sleep_for(std::chrono::seconds(2));

    if (is_connected_) {
      std::cout << "Status: Connected" << std::endl;
    } else {
      std::cout << "Status: Not connected" << std::endl;
    }
  }

  void start_audio() {
    if (!is_connected_) {
      std::cout << "Not connected to server. Please connect first."
                << std::endl;
      return;
    }
    if (is_capturing_) {
      std::cout << "Audio capture is already running." << std::endl;
      return;
    }
    is_capturing_ = true;
    audio_capture_.start_capture(
        [this](const std::vector<float>& audioData) { send_audio(audioData); });
    receive_audio();
    std::cout << "Audio capture started." << std::endl;
  }

  void stop_audio() {
    if (!is_capturing_) {
      std::cout << "Audio capture is not running." << std::endl;
      return;
    }
    is_capturing_ = false;
    audio_capture_.stop_capture();
    std::cout << "Audio capture stopped." << std::endl;
  }

  bool is_connected() const { return is_connected_; }

 private:
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

  boost::asio::io_context& io_context_;
  tcp::socket socket_;
  AudioCapture audio_capture_;
  std::array<char, 1024> receive_buffer_;
  std::atomic<bool> is_connected_;
  std::atomic<bool> is_capturing_;
};

void print_menu() {
  std::cout << "\n--- Menu ---" << std::endl;
  std::cout << "1. Connect to server" << std::endl;
  std::cout << "2. Start audio" << std::endl;
  std::cout << "3. Stop audio" << std::endl;
  std::cout << "4. Exit" << std::endl;
  std::cout << "Enter your choice: ";
}

int main() {
  try {
    boost::asio::io_context io_context;
    Client client(io_context);

    std::thread io_thread([&io_context]() { io_context.run(); });

    while (true) {
      print_menu();
      int choice;
      std::cin >> choice;

      switch (choice) {
        case 1: {
          std::string host, port;
          std::cout << "Enter server host: ";
          std::cin >> host;
          std::cout << "Enter server port: ";
          std::cin >> port;
          client.connect(host, port);
          break;
        }
        case 2:
          client.start_audio();
          break;
        case 3:
          client.stop_audio();
          break;
        case 4:
          std::cout << "Exiting..." << std::endl;
          io_context.stop();
          io_thread.join();
          return 0;
        default:
          std::cout << "Invalid choice. Please try again." << std::endl;
      }
    }
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }

  return 0;
}
