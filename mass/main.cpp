// #include "mainwindow.h"
#include "messengerclient.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    MessengerClient client;
    client.show();
    return app.exec();
}
