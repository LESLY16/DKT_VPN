#include <QApplication>
#include <QIcon>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("DKT VPN");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("DKT");
    app.setOrganizationDomain("dkt.vpn");

    MainWindow window;
    window.show();

    return app.exec();
}
