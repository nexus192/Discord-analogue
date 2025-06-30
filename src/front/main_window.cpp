#include "main_window.h"

#include <QMessageBox>

#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      socket_(io_context_),
      is_connected_(false),
      is_capturing_(false) {
  ui->setupUi(this);
  io_thread_ = std::thread([this]() { io_context_.run(); });
}

MainWindow::~MainWindow() {
  io_context_.stop();
  io_thread_.join();
  delete ui;
}

void MainWindow::connectToServer(const std::string& host,
                                 const std::string& port) {
  tcp::resolver resolver(io_context_);
  auto endpoints = resolver.resolve(host, port);
  boost::asio::async_connect(
      socket_, endpoints,
      [this, host, port](boost::system::error_code ec, tcp::endpoint) {
        if (!ec) {
          is_connected_ = true;
          QMessageBox::information(this, "Connection",
                                   "Connected to " +
                                       QString::fromStdString(host) + ":" +
                                       QString::fromStdString(port));
        } else {
          QMessageBox::critical(
              this, "Connection",
              "Connection failed: " + QString::fromStdString(ec.message()));
        }
      });
}

void MainWindow::on_connectButton_clicked() {
  QString host = ui->hostLineEdit->text();
  QString port = ui->portLineEdit->text();
  connectToServer(host.toStdString(), port.toStdString());
}

void MainWindow::on_startAudioButton_clicked() {
  if (!is_connected_) {
    QMessageBox::warning(this, "Warning",
                         "Not connected to server. Please connect first.");
    return;
  }
  if (is_capturing_) {
    QMessageBox::information(this, "Info", "Audio capture is already running.");
    return;
  }
  is_capturing_ = true;
  audio_capture_.start_capture(
      [this](const std::vector<float>& audioData) { send_audio(audioData); });
  receive_audio();
  QMessageBox::information(this, "Info", "Audio capture started.");
}

void MainWindow::on_stopAudioButton_clicked() {
  if (!is_capturing_) {
    QMessageBox::information(this, "Info", "Audio capture is not running.");
    return;
  }
  is_capturing_ = false;
  audio_capture_.stop_capture();
  QMessageBox::information(this, "Info", "Audio capture stopped.");
}

void MainWindow::send_audio(const std::vector<float>& audioData) {
  std::vector<char> buffer(audioData.size() * sizeof(float));
  std::memcpy(buffer.data(), audioData.data(), buffer.size());

  boost::asio::async_write(
      socket_, boost::asio::buffer(buffer),
      [this](boost::system::error_code ec, std::size_t /*length*/) {
        if (ec) {
          QMessageBox::critical(
              this, "Error",
              "Error sending audio: " + QString::fromStdString(ec.message()));
        }
      });
}

void MainWindow::receive_audio() {
  socket_.async_read_some(
      boost::asio::buffer(receive_buffer_),
      [this](boost::system::error_code ec, std::size_t length) {
        if (!ec) {
          // Обработка полученных данных
          receive_audio();
        } else if (ec == boost::asio::error::eof) {
          QMessageBox::information(this, "Info",
                                   "Server closed the connection.");
        } else {
          QMessageBox::critical(
              this, "Error",
              "Error receiving audio: " + QString::fromStdString(ec.message()));
        }
      });
}
