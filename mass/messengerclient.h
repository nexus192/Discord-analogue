#ifndef MESSENGERCLIENT_H
#define MESSENGERCLIENT_H

#include <QMainWindow>
#include <QMainWindow>
#include <QNetworkAccessManager>

class QLineEdit;
class QTextEdit;
class QNetworkReply;

class MessengerClient : public QMainWindow
{
    Q_OBJECT

public:
    explicit MessengerClient(QWidget* parent = nullptr);
    virtual ~MessengerClient() = default;

    // Запрещаем копирование
    MessengerClient(const MessengerClient&) = delete;
    MessengerClient& operator=(const MessengerClient&) = delete;

private slots:
    void sendMessage();
    void onNetworkReply(QNetworkReply* reply);

private:
    void setupUi();
    void setupNetworkManager();

    QLineEdit* messageInput;
    QTextEdit* chatDisplay;
    QNetworkAccessManager* networkManager;
};

#endif // MESSENGERCLIENT_H
