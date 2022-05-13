#ifndef DIALOG_ESTRELLAS_MUNDIALES_H
#define DIALOG_ESTRELLAS_MUNDIALES_H

#include <QDialog>
#include "equipo.h"
#include "transform_minifoto.h"
#include "mybutton.h"
#include <QButtonGroup>
#include "dialog_jugador.h"

namespace Ui {
class dialog_estrellas_mundiales;
}

class dialog_estrellas_mundiales : public QDialog
{
    Q_OBJECT

public:
    explicit dialog_estrellas_mundiales(QWidget *parent = nullptr);
    ~dialog_estrellas_mundiales();

    int set_equipo(EQUIPO ptr_eq);
    int set_punteros(QList <quint16> punteros);
    QList <quint16> get_punteros();
    EQUIPO get_equipo();
    EQUIPO eq;
    TRANSFORM_MINIFOTO trans;
private slots:
//    void boton_group_clicked(int id);

//    void on_btn_guardar_clicked();

    void on_tableView_jugadores_doubleClicked(const QModelIndex &index);

//    void on_btn_mas_jugador_clicked();

//    void on_btn_menos_jugador_clicked();


private:
    Ui::dialog_estrellas_mundiales *ui;
    QStandardItemModel *model, *model2;
    QButtonGroup boton_group;
    void update_model();
    QList <quint16> punteros;
};

#endif // DIALOG_ESTRELLAS_MUNDIALES_H
