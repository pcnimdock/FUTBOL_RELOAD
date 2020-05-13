#ifndef DIALOG_JUGADOR_H
#define DIALOG_JUGADOR_H
#include <mybutton.h>
#include <jugador.h>
#include <transform_minifoto.h>
#include <QDialog>

namespace Ui {
class Dialog_jugador;
}

class Dialog_jugador : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog_jugador(QWidget *parent = nullptr);
    ~Dialog_jugador();

 int set_jugador(JUGADOR ju);
 JUGADOR get_jugador();

private slots:
 void on_btn_guardar_clicked();

 void on_btn_BigFoto_clicked();

 void on_btn_MiniFoto_clicked();

private:
    Ui::Dialog_jugador *ui;
    JUGADOR jug;
    TRANSFORM_MINIFOTO trans;
};

#endif // DIALOG_JUGADOR_H
