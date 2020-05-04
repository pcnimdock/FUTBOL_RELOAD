#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void tratar_escudo_original();
    void cargar_imagen_reemplazo();
    QVector<QRgb> colors; //iniciar paleta de colores

private slots:
    void on_btn_file_clicked();

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_spinBox_valueChanged(int arg1);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
