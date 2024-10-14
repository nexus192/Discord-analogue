// mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <atomic>
#include <boost/asio.hpp>
#include <thread>

#include "../audiocapture.h"  // Ваш класс AudioCapture

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(QWidget* parent = nullptr);
  ~MainWindow();

 private slots:
  void on_connectButton_clicked();
  void on_startAudioButton_clicked();
  void on_stopAudioButton_clicked();

 private:
  Ui::MainWindow* ui;
  boost::asio::io_context io_context_;
  std::thread io_thread_;
  tcp::socket socket_;
  AudioCapture audio_capture_;
  std::atomic<bool> is_connected_;
  std::atomic<bool> is_capturing_;

  void connectToServer(const std::string& host, const std::string& port);
  void send_audio(const std::vector<float>& audioData);
  void receive_audio();
};

#endif  // MAINWINDOW_H
