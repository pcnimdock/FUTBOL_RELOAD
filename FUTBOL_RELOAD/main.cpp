#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setApplicationName("Fútbol Reload");
    QApplication::setApplicationVersion("2.0");
    QApplication::setOrganizationName("FUTBOL_RELOAD");

    // Fusion como base: garantiza que la hoja de estilos se aplique igual
    // en Windows, Linux y macOS (los estilos nativos ignoran parte del QSS).
    a.setStyle(QStyleFactory::create("Fusion"));

    QFile estiloFile(":/style/style.qss");
    if (estiloFile.open(QFile::ReadOnly | QFile::Text)) {
        a.setStyleSheet(QString::fromUtf8(estiloFile.readAll()));
        estiloFile.close();
    }

    MainWindow w;
    w.show();
    return a.exec();
}
