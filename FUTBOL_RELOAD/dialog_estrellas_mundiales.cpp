#include "dialog_estrellas_mundiales.h"
#include "ui_dialog_estrellas_mundiales.h"

dialog_estrellas_mundiales::dialog_estrellas_mundiales(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::dialog_estrellas_mundiales)
{
    ui->setupUi(this);
}

dialog_estrellas_mundiales::~dialog_estrellas_mundiales()
{
    delete ui;
}

int dialog_estrellas_mundiales::set_equipo(EQUIPO ptr_eq)
{
    eq=ptr_eq;
    int n = eq.aforo;
    //meter a los jugadores en el qtableview

    model = new QStandardItemModel(eq.lista_jugadores.size(),2,this);
    for(int i=0;i<eq.lista_jugadores.size();i++)
    {
        QModelIndex index = model->index(i,1,QModelIndex());
        model->setData(index,eq.lista_jugadores.at(i).NombreCorto);

        if(eq.lista_jugadores.at(i).minifoto.isEmpty()==false)
        {
            QImage im = trans.minifoto2QImage(eq.lista_jugadores.at(i).minifoto);
            index = model->index(i,0,QModelIndex());
            QStandardItem *item = new QStandardItem();
            item->setData(QVariant(QPixmap::fromImage(im)), Qt::DecorationRole);
            //model->setData(index,QVariant(QPixmap::fromImage(im)));
            model->setItem(i,0,item);
            // delete item;
        }
    }
    model->setHeaderData(0, Qt::Horizontal, tr("MiniFoto"), Qt::DisplayRole);
    model->setHeaderData(1, Qt::Horizontal, tr("Nombre"), Qt::DisplayRole);
    ui->tableView_jugadores->setModel(model);
    ui->tableView_jugadores->setStyleSheet("QTableView::item:selected { color:white; background:black; font-weight:1000; }"
                                           "QTableCornerButton::section { background-color:#232326; }"
                                           "QHeaderView::section { color:white; background-color:#a2d324; }"
                                           "QTableView {background:white;}");

    model2 = new QStandardItemModel(eq.lista_entrenadores.size(),2,this);
    return 1;

}

int dialog_estrellas_mundiales::set_punteros(QList<quint16> punteros_base)
{
    punteros=punteros_base;
}

QList<quint16> dialog_estrellas_mundiales::get_punteros()
{
 return punteros;
}

EQUIPO dialog_estrellas_mundiales::get_equipo()
{
    return eq;
}

void dialog_estrellas_mundiales::on_tableView_jugadores_doubleClicked(const QModelIndex &index)
{
    //sacar el jugador seleccionado con doble click
        int num_jugador=index.row();

        Dialog_jugador dialog_jug;
        JUGADOR jug;
        jug=eq.lista_jugadores.at(num_jugador);

        dialog_jug.set_jugador(jug);
        if(QDialog::Accepted==dialog_jug.exec())
        {
            jug =dialog_jug.get_jugador();
            eq.lista_jugadores[num_jugador]=jug;
            update_model();
        }
        else
        {

        }

}

void dialog_estrellas_mundiales::update_model()
{
    QModelIndex index;
    delete model;
    model = new QStandardItemModel(eq.lista_jugadores.size(),2,this);

    for(int i=0;i<eq.lista_jugadores.size();i++)
    {
        index = model->index(i,1,QModelIndex());
        model->setData(index,eq.lista_jugadores.at(i).NombreCorto);

        if(eq.lista_jugadores.at(i).minifoto.isEmpty()==false)
        {
            QImage im = trans.minifoto2QImage(eq.lista_jugadores.at(i).minifoto);
            index = model->index(i,0,QModelIndex());
            QStandardItem *item = new QStandardItem();
            item->setData(QVariant(QPixmap::fromImage(im)), Qt::DecorationRole);
            //model->setData(index,QVariant(QPixmap::fromImage(im)));
            model->setItem(i,0,item);
            // delete item;
        }
    }

    ui->tableView_jugadores->setModel(model);

   // model->setHeaderData(0, Qt::Horizontal, tr("MiniFoto"), Qt::DisplayRole);
   // model->setHeaderData(1, Qt::Horizontal, tr("Nombre"), Qt::DisplayRole);
   // ui->tableView_jugadores->setModel(model);
}
