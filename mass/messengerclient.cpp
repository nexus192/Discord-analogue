#include "messengerclient.h"
#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>

MessengerClient::MessengerClient(QWidget* parent)
    : QMainWindow(parent)
    , messageInput(nullptr)
    , chatDisplay(nullptr)
    , networkManager(nullptr)
{
    setupUi();
    setupNetworkManager();
}

void MessengerClient::sendMessage()
{
    QString message = messageInput->text();
    if (message.isEmpty()) return;

    QJsonObject json;
    json["message"] = message;
    json["sender"] = "user1"; // Здесь должен быть идентификатор пользователя

    QNetworkRequest request(QUrl("http://192.168.0.78:5000/"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QByteArray data = QJsonDocument(json).toJson();
    networkManager->post(request, data);

    messageInput->clear();
}

void MessengerClient::onNetworkReply(QNetworkReply* reply)
{
    if (reply->error() == QNetworkReply::NoError)
    {
        QString response = QString::fromUtf8(reply->readAll());
        chatDisplay->append("Message sent: " + response);
    }
    else
    {
        chatDisplay->append("Error: " + reply->errorString());
    }
    reply->deleteLater();
}

void MessengerClient::setupUi()
{
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);

    chatDisplay = new QTextEdit(this);
    chatDisplay->setReadOnly(true);
    layout->addWidget(chatDisplay);

    messageInput = new QLineEdit(this);
    layout->addWidget(messageInput);

    QPushButton* sendButton = new QPushButton("Send", this);
    layout->addWidget(sendButton);

    connect(sendButton, &QPushButton::clicked, this, &MessengerClient::sendMessage);
    connect(messageInput, &QLineEdit::returnPressed, this, &MessengerClient::sendMessage);

    setCentralWidget(centralWidget);
    setWindowTitle("Simple Messenger");
    resize(400, 300);
}

void MessengerClient::setupNetworkManager()
{
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished, this, &MessengerClient::onNetworkReply);
}

