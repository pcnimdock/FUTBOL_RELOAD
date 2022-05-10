#include "dialogequipos.h"
#include "ui_dialogequipos.h"
#include <QString>
#include <QFileDialog>


DialogEquipos::DialogEquipos(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogEquipos)
{
    ui->setupUi(this);
    connect(&boton_group,SIGNAL(buttonClicked(int)), this, SLOT(boton_group_clicked(int)));
}

DialogEquipos::~DialogEquipos()
{
    delete ui;
}

int DialogEquipos::set_equipo(EQUIPO ptr_eq)
{
    eq=ptr_eq;
    int n = eq.aforo;
    ui->le_Aforo->setText(QString::number(n));
    ui->le_Estadio->setText(eq.estadio);
    ui->le_n_socios->setText(QString::number(eq.num_socios));
    ui->le_proveedor->setText(eq.proveedor);
    ui->le_presidente->setText(eq.presidente);
    ui->le_Nombre_Corto->setText(eq.NombreCorto);
    ui->le_Nombre_Largo->setText(eq.NombreLargo);
    ui->le_patrocinador->setText(eq.patrocinador);

    ui->le_anyo_fundacion->setText(QString::number(eq.anyo_fundacion));
    ui->le_presupuesto_bbdd->setText(QString::number(eq.presupuesto_bbdd));
    ui->le_Nombre_Mayusculas->setText(eq.NombreMayusculas);
    ui->le_presupuesto_juego->setText(QString::number(eq.presupuesto_juego));
    ui->le_nombre_corto_mayusculas->setText(eq.nombre_corto_mayusculas);

    ui->btn_ESCUDO->setId(0);
    ui->btn_ESCUDO_SIM->setId(1);
    ui->btn_ESCUDO->setEQname("ESCUDO");
    ui->btn_ESCUDO_SIM->setEQname("ESCUDO_SIM");
    ui->btn_ESCUDO->setIconEQ(trans.miniesc2QImage(eq.miniesc));
    if(eq.EquipoIdDBC!=959901)
    {
    ui->btn_ESCUDO_SIM->setIconEQ(trans.fotosim2QImage(eq.esc_sim));
    }
    boton_group.addButton(ui->btn_ESCUDO,0);
    boton_group.addButton(ui->btn_ESCUDO_SIM,1);

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
    for(int i=0;i<eq.lista_entrenadores.size();i++)
    {
        QModelIndex index = model2->index(i,1,QModelIndex());
        model2->setData(index,eq.lista_entrenadores.at(i).NombreCorto);

        if(eq.lista_entrenadores.at(i).minifoto.isEmpty()==false)
        {
            QImage im = trans.minifoto2QImage(eq.lista_entrenadores.at(i).minifoto);
            index = model2->index(i,0,QModelIndex());
            QStandardItem *item = new QStandardItem();
            item->setData(QVariant(QPixmap::fromImage(im)), Qt::DecorationRole);
            //model->setData(index,QVariant(QPixmap::fromImage(im)));
            model2->setItem(i,0,item);
            // delete item;
        }
    }
    ui->tableView_entrenadores->setModel(model2);
    ui->tableView_entrenadores->setStyleSheet("QTableView::item:selected { color:white; background:black; font-weight:1000; }"
                                              "QTableCornerButton::section { background-color:#232326; }"
                                              "QHeaderView::section { color:white; background-color:#a2d324; }"
                                              "QTableView {background:white;}");

    return 1;

}

int DialogEquipos::set_punteros(QList<quint16> punteros_base)
{
   punteros=punteros_base;

   return 1;

}

QList<quint16> DialogEquipos::get_punteros()
{
    return punteros;
}

EQUIPO DialogEquipos::get_equipo()
{
    return eq;

}

void DialogEquipos::boton_group_clicked(int id)
{
    //se ha apretado uno de los botones de los escudos
    //escudo de equipo
    if(id==0)
    {
        QString fileName = QFileDialog::getOpenFileName(this,tr("Archivo sustitución de EscudoSim"),"./" ,tr("Imagenes (*.bmp *.BMP)"));
        QImage im = QImage(fileName);
        ui->btn_ESCUDO->setIconEQ(im);
    }
    else
    {
        //escudo de simulador
        QString fileName = QFileDialog::getOpenFileName(this,tr("Archivo sustitución de Escudo"),"./" ,tr("Imagenes (*.bmp *.BMP)"));
        QImage im = QImage(fileName);
        ui->btn_ESCUDO_SIM->setIconEQ(im);
    }
}

void DialogEquipos::on_btn_guardar_clicked()
{
    int n = eq.aforo;
    eq.aforo=ui->le_Aforo->text().toUInt();
    eq.estadio=ui->le_Estadio->text();
    eq.num_socios=ui->le_n_socios->text().toUInt();
    eq.proveedor=ui->le_proveedor->text();
    eq.presidente=ui->le_presidente->text();
    eq.NombreCorto=ui->le_Nombre_Corto->text();
    eq.NombreLargo=ui->le_Nombre_Largo->text();
    eq.patrocinador=ui->le_patrocinador->text();

    eq.anyo_fundacion=ui->le_anyo_fundacion->text().toUInt();
    eq.presupuesto_bbdd=ui->le_presupuesto_bbdd->text().toUInt();
    eq.NombreMayusculas=ui->le_Nombre_Mayusculas->text();
    eq.presupuesto_juego=ui->le_presupuesto_juego->text().toUInt();
    eq.nombre_corto_mayusculas=ui->le_nombre_corto_mayusculas->text();
    eq.miniesc= trans.devolver_miniesc_reemplazo(ui->btn_ESCUDO->getIconEQ());
    eq.esc_sim = trans.devolver_fotosim_reemplazo(ui->btn_ESCUDO_SIM->getIconEQ());


}

void DialogEquipos::on_tableView_jugadores_doubleClicked(const QModelIndex &index)
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

void DialogEquipos::update_model()
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

void DialogEquipos::on_btn_mas_jugador_clicked()
{
    //se ha apretado para añadir un nuevo jugador
    //hacer un jugador genérico
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

    //sacar una lista de los numeros de camiseta
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

    //buscar un puntero no usado
    i=1;
    while(punteros.indexOf(i)>=0)
    {i++;}
    j.puntero=i;
    punteros.append(i);
    eq.lista_jugadores.append(j);
    update_model();

 }

void DialogEquipos::on_btn_menos_jugador_clicked()
{
    QModelIndex inx = ui->tableView_jugadores->currentIndex();
    int num_jug=inx.row();
    quint16 puntero_jugador=eq.lista_jugadores.at(num_jug).puntero;
    punteros.removeAt(punteros.indexOf(puntero_jugador));
    eq.lista_jugadores.removeAt(num_jug);
    update_model();

}
