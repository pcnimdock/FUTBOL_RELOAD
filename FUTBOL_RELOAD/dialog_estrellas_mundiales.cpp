#include "dialog_estrellas_mundiales.h"
#include "pcfutbol_plantillas.h"
#include <QMessageBox>
#include "util_ventanas.h"
#include "ui_dialog_estrellas_mundiales.h"
#include <QFile>
#include <QPalette>
#include <QColor>

dialog_estrellas_mundiales::dialog_estrellas_mundiales(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::dialog_estrellas_mundiales)
{
    ui->setupUi(this);
    // Botones de maximizar/minimizar: en Linux los QDialog no los traen
    setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint);

    QFile estiloFile(":/style/style.qss");
    if (estiloFile.open(QFile::ReadOnly | QFile::Text)) {
        setStyleSheet(QString::fromUtf8(estiloFile.readAll()));
        estiloFile.close();
    }

    QPalette palTabla = ui->tableView_jugadores->palette();
    palTabla.setColor(QPalette::Base, QColor("#202325"));
    palTabla.setColor(QPalette::AlternateBase, QColor("#26292b"));
    palTabla.setColor(QPalette::Text, QColor("#eaeaea"));
    palTabla.setColor(QPalette::WindowText, QColor("#eaeaea"));
    palTabla.setColor(QPalette::Highlight, QColor("#86aa1c"));
    palTabla.setColor(QPalette::HighlightedText, QColor("#16210a"));
    ui->tableView_jugadores->setPalette(palTabla);

    // Que la ventana quepa en pantallas pequeñas (contenido con barras de desplazamiento)
    UtilVentanas::hacerDesplazable(this);
}

dialog_estrellas_mundiales::~dialog_estrellas_mundiales()
{
    delete ui;
}

int dialog_estrellas_mundiales::set_equipo(EQUIPO ptr_eq)
{
    eq=ptr_eq;

    ui->le_Nombre_Corto->setText(eq.NombreCorto);
    ui->le_nombre_corto_mayusculas->setText(eq.nombre_corto_mayusculas);

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
            model->setItem(i,0,item);
        }
    }
    model->setHeaderData(0, Qt::Horizontal, tr("MiniFoto"), Qt::DisplayRole);
    model->setHeaderData(1, Qt::Horizontal, tr("Nombre"), Qt::DisplayRole);
    ui->tableView_jugadores->setModel(model);

    model2 = new QStandardItemModel(eq.lista_entrenadores.size(),2,this);
    return 1;

}

int dialog_estrellas_mundiales::set_punteros(QList<quint16> punteros_base)
{
    punteros=punteros_base;
    return 1;
}

QList<quint16> dialog_estrellas_mundiales::get_punteros()
{
 return punteros;
}

EQUIPO dialog_estrellas_mundiales::get_equipo()
{
    return eq;
}

void dialog_estrellas_mundiales::on_btn_guardar_clicked()
{
    eq.NombreCorto=ui->le_Nombre_Corto->text();
    eq.nombre_corto_mayusculas=ui->le_nombre_corto_mayusculas->text();
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

void dialog_estrellas_mundiales::on_btn_mas_jugador_clicked()
{
    //se ha apretado para añadir un nuevo jugador a Estrellas Mundiales
    if (eq.lista_jugadores.size() >= PCFutbolPlantillas::MAX_ESTRELLAS)
    {
        QMessageBox::warning(this, "Añadir jugador",
            QString("Ya hay %1 Estrellas Mundiales: con más la Liga Manager se cuelga al cargar.")
                .arg(PCFutbolPlantillas::MAX_ESTRELLAS));
        return;
    }
    JUGADOR j;
    j.NombreCorto="xxx";
    j.NombreLargo="xxx";
    j.Pelo=0;
    j.Peso=0;
    j.Piel=0;
    j.Altura=180;
    j.Precio=20;
    j.Regate=50;
    j.ActPase=0;
    j.ActTiro=0;
    j.bigfoto.clear();
    j.Agilidad=50;
    j.minifoto.clear();
    j.ActRemate=0;
    j.Velocidad=50;
    j.ActTecnica=0;
    j.Comentario=" ";
    j.Extranjero=0;
    j.Agresividad=50;
    j.EquipoIdDBC= eq.EquipoIdDBC;
    j.EstadoFichaje=0;
    j.Internacional="0";
    j.DemarcacionPref=1;
    j.DemarcacionesSecundarias.clear();
    j.DemarcacionesSecundarias.append(5,0);
    j.EquipoProcedencia="Ninguno";
    j.FechaDeNacimiento.clear();
    j.FechaDeNacimiento.append(0x01);
    j.FechaDeNacimiento.append(0x01);
    j.FechaDeNacimiento.append(0x01);
    j.LugarDeNacimiento="Narnia";
    j.PosicionEnCampo=1;

    //buscar un número de camiseta no usado
    QList <quint16> num_camiseta_list;
    for(int i=0;i<eq.lista_jugadores.size();i++)
    {
        num_camiseta_list.append(eq.lista_jugadores.at(i).NumeroCamiseta);
    }
    int i=1;
    while(num_camiseta_list.indexOf(i)>=0)
    {i++;}
    j.NumeroCamiseta=i;
    j.PosicionEnCampo=i;

    //buscar un puntero no usado (global, para no chocar con jugadores de otros equipos)
    i=1;
    while(punteros.indexOf(i)>=0)
    {i++;}
    j.puntero=i;
    punteros.append(i);

    eq.lista_jugadores.append(j);
    update_model();
}

void dialog_estrellas_mundiales::on_btn_menos_jugador_clicked()
{
    QModelIndex inx = ui->tableView_jugadores->currentIndex();
    int num_jug=inx.row();
    if(num_jug<0 || num_jug>=eq.lista_jugadores.size())
    {
        return; //no hay ningún jugador seleccionado
    }
    quint16 puntero_jugador=eq.lista_jugadores.at(num_jug).puntero;
    punteros.removeAt(punteros.indexOf(puntero_jugador));
    eq.lista_jugadores.removeAt(num_jug);
    update_model();
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
            model->setItem(i,0,item);
        }
    }

    model->setHeaderData(0, Qt::Horizontal, tr("MiniFoto"), Qt::DisplayRole);
    model->setHeaderData(1, Qt::Horizontal, tr("Nombre"), Qt::DisplayRole);
    ui->tableView_jugadores->setModel(model);
}
