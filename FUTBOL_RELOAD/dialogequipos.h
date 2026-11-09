#ifndef DIALOGEQUIPOS_H
#define DIALOGEQUIPOS_H

#include <QDialog>
#include "equipo.h"
#include "transform_minifoto.h"
#include "mybutton.h"
#include <QButtonGroup>
#include "dialog_jugador.h"

class PCFutbolTemporadaReal;


namespace Ui {
class DialogEquipos;
}

class DialogEquipos : public QDialog
{
    Q_OBJECT

public:
    explicit DialogEquipos(QWidget *parent = nullptr);
    ~DialogEquipos();
    int set_equipo(EQUIPO ptr_eq);
    int set_punteros(QList <quint16> punteros);
    QList <quint16> get_punteros();
    EQUIPO get_equipo();
    void setRutaJuego(const QString &ruta);
    // Jugadores de la temporada real: no se pueden quitar de la plantilla
    void setTemporadaReal(const PCFutbolTemporadaReal *real) { m_real = real; }
     EQUIPO eq;
     TRANSFORM_MINIFOTO trans;

private slots:
    void boton_group_clicked(int id);

    void on_btn_guardar_clicked();

    void on_tableView_jugadores_doubleClicked(const QModelIndex &index);

    void on_btn_mas_jugador_clicked();

    void on_btn_menos_jugador_clicked();

private:
    Ui::DialogEquipos *ui;
    QStandardItemModel *model, *model2;
    QButtonGroup boton_group;
    void update_model();
    QList <quint16> punteros;
    QString rutaJuego;
    // Sólo se vuelven a convertir los escudos que el usuario ha cambiado
    bool m_escudoCambiado = false;
    const PCFutbolTemporadaReal *m_real = nullptr;
    bool m_escudoSimCambiado = false;


};

#endif // DIALOGEQUIPOS_H
