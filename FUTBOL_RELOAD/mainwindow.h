#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QFile>
#include <bbdd_futbol.h>
#include <entrenador.h>
#include <equipo.h>
#include <jugador.h>
#include <QList>
#include "mybutton.h"
#include "transform_minifoto.h"
#include "estrellas_mundiales.h"

#include <QTableView>
#include <QItemDelegate>
#include <QStandardItemModel>
#include <dialogequipos.h>
#include <dialog_jugador.h>




QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();

    void on_btn_procesar_clicked();

    void mouseOnEqButton(int,char *);

    void on_horizontalScrollBar_valueChanged(int value);

    void EQ_button_clicked(int);

    void on_tableView_equipos_doubleClicked(const QModelIndex &index);

    void on_pushButton_2_clicked();

    void on_tableView_jugadores_doubleClicked(const QModelIndex &index);

private:
    Ui::MainWindow *ui;
    QStringList lista_archivos;
    BBDD_FUTBOL db;
    QFile *file;
    EQUIPO equipo;
    ENTRENADOR entrenador;
    JUGADOR jugador;
    QList <ENTRENADOR> lista_entrenadores;
    QList <JUGADOR>    lista_jugadores;
    QList <mybutton *> list_eq_buttons;
    QList <JUGADOR>    lista_jugadores_totales;
    TRANSFORM_MINIFOTO transf_minifoto;
    QList <EQUIPO> lista_equipos;
    QButtonGroup  EqButtonGroup;
    ESTRELLAS_MUNDIALES estrellas;

    QStandardItemModel *model;
    QStandardItemModel *model2;

};
#endif // MAINWINDOW_H