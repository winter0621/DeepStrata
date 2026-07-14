#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Deep Strata");
    app.setApplicationVersion("1.0");

    MainWindow window;
    window.setWindowTitle("Deep Strata 深空底层");
    window.resize(1280, 720);
    window.show();

    return app.exec();
}
