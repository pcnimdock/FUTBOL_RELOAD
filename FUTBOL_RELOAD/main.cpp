#include "mainwindow.h"

#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFile estiloFile(":/style/style.qss");
    if (estiloFile.open(QFile::ReadOnly | QFile::Text)) {
        a.setStyleSheet(QString::fromUtf8(estiloFile.readAll()));
        estiloFile.close();
    }

    MainWindow w;
    w.show();
    return a.exec();
}
