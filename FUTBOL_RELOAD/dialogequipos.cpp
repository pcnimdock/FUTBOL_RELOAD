#include "dialogequipos.h"
#include "pcfutbol_plantillas.h"
#include "pcfutbol_temporada_real.h"
#include <QMessageBox>
#include "util_ventanas.h"
#include "ui_dialogequipos.h"
#include "dialog_equipacion.h"
#include <QString>
#include <QFileDialog>
#include <QFile>
#include <QPalette>
#include <QColor>
#include <QPushButton>


DialogEquipos::DialogEquipos(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogEquipos)
{
    ui->setupUi(this);
    // Botones de maximizar/minimizar: en Linux los QDialog no los traen
    setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint);
    connect(&boton_group, &QButtonGroup::idClicked, this, &DialogEquipos::boton_group_clicked);

    // --- Botón para abrir el editor de equipación (camiseta) ---
    QPushButton *btnEquipacion = new QPushButton("Editar equipación", this);
    btnEquipacion->setObjectName("btnEditarEquipacion");
    ui->gridLayout_2->addWidget(btnEquipacion, 1, 0, 1, 4);
    connect(btnEquipacion, &QPushButton::clicked, this, [this]() {
        DialogEquipacion dlgEquipacion(this);
        if(!rutaJuego.isEmpty())
        {
            dlgEquipacion.setRutaEquiposDat(rutaJuego + "/PALETAS/EQUIPOS.DAT");
        }
        if(eq.numero_kit > 0)
        {
            dlgEquipacion.setNumeroEquipo(eq.numero_kit);
        }
        if(!eq.equipacion1_textura.isEmpty())
        {
            dlgEquipacion.mostrarTexturaActual(true, eq.equipacion1_textura);
        }
        if(!eq.equipacion2_textura.isEmpty())
        {
            dlgEquipacion.mostrarTexturaActual(false, eq.equipacion2_textura);
        }
        dlgEquipacion.exec();

        // El editor puede haber cambiado las texturas (pintadas a mano o por
        // patrón). Las recogemos para que el EQUIPO en memoria quede al día.
        const QByteArray t1 = dlgEquipacion.texturaEditada(true);
        const QByteArray t2 = dlgEquipacion.texturaEditada(false);
        if(t1.size() == PCFutbolEquipacion::TAM_TEXTURA)
        {
            eq.equipacion1_textura = t1;
        }
        if(t2.size() == PCFutbolEquipacion::TAM_TEXTURA)
        {
            eq.equipacion2_textura = t2;
        }
    });

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

DialogEquipos::~DialogEquipos()
{
    delete ui;
}

int DialogEquipos::set_equipo(EQUIPO ptr_eq)
{
    eq=ptr_eq;
    ui->le_Aforo->setText(QString::number(eq.aforo));
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
        if(eq.esc_sim.size()>1500)
        {
    ui->btn_ESCUDO_SIM->setIconEQ(trans.fotosim2QImage(eq.esc_sim));
        }
        else
        {
    ui->btn_ESCUDO_SIM->setIconEQ_italiano(trans.fotosim2QImage(eq.esc_sim));
        }
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

void DialogEquipos::setRutaJuego(const QString &ruta)
{
    rutaJuego = ruta;
}

void DialogEquipos::boton_group_clicked(int id)
{
    //se ha apretado uno de los botones de los escudos
    //escudo de equipo
    if(id==0)
    {
        QString fileName = QFileDialog::getOpenFileName(this,tr("Archivo sustitución de Escudo"),"./" ,tr("Imagenes (*.bmp *.BMP)"));
        QImage im = QImage(fileName);
        if (im.isNull()) return;    // cancelado o imagen no válida: no tocar el escudo
        ui->btn_ESCUDO->setIconEQ(im);
        m_escudoCambiado = true;
    }
    else
    {
        //escudo de simulador
        QString fileName = QFileDialog::getOpenFileName(this,tr("Archivo sustitución de EscudoSim"),"./" ,tr("Imagenes (*.bmp *.BMP)"));
        QImage im = QImage(fileName);
        if (im.isNull()) return;    // cancelado o imagen no válida: no tocar el escudo
        m_escudoSimCambiado = true;
        if(eq.esc_sim.size()<1500)
        {
            ui->btn_ESCUDO_SIM->setIconEQ_italiano(im);
        }
        else
        {
        ui->btn_ESCUDO_SIM->setIconEQ(im);
        }
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
    // Antes se volvían a convertir siempre, aunque no se hubieran tocado
    if (m_escudoCambiado)
        eq.miniesc= trans.devolver_miniesc_reemplazo(ui->btn_ESCUDO->getIconEQ());
    if (m_escudoSimCambiado)
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
    // Jugador nuevo con dorsal libre, el último en la alineación y un puntero
    // (número de foto) que no use nadie. Antes se escribía por error el
    // número en la «posición en el campo» (0-3) y la alineación quedaba sin
    // poner, lo que podía colgar el juego.
    quint16 p = 1;
    while (punteros.contains(p)) p++;
    for (const JUGADOR &j : eq.lista_jugadores)
        while (j.puntero == p || punteros.contains(p)) p++;
    JUGADOR j = PCFutbolPlantillas::jugadorNuevo(eq, p);
    punteros.append(p);
    eq.lista_jugadores.append(j);
    update_model();
    ui->tableView_jugadores->selectRow(eq.lista_jugadores.size() - 1);
}

void DialogEquipos::on_btn_menos_jugador_clicked()
{
    const QModelIndex inx = ui->tableView_jugadores->currentIndex();
    const int num_jug = inx.row();
    if (!inx.isValid() || num_jug < 0 || num_jug >= eq.lista_jugadores.size())
    {
        QMessageBox::information(this, "Quitar jugador", "Selecciona primero un jugador de la lista.");
        return;
    }
    if (m_real && m_real->juegaPartidosReales(eq.EquipoIdDBC, eq.lista_jugadores.at(num_jug).puntero))
    {
        QMessageBox::warning(this, "Quitar jugador",
            eq.lista_jugadores.at(num_jug).NombreCorto + " juega partidos de la temporada real 95/96 "
            "con este equipo. Si se quita, el juego no arranca («Jugador no encontrado»).\n\n"
            "Para llevarlo a otro equipo usa «Traspasos…»: se deja una copia aquí.");
        return;
    }
    if (QMessageBox::question(this, "Quitar jugador",
            "¿Quitar a " + eq.lista_jugadores.at(num_jug).NombreCorto + " de la plantilla?",
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;
    const quint16 puntero_jugador = eq.lista_jugadores.at(num_jug).puntero;
    // la alineación se renumera para que siga yendo de 1 a N
    PCFutbolPlantillas::quitarJugador(eq, num_jug);
    bool sigueUsado = false;
    for (const JUGADOR &j : eq.lista_jugadores) sigueUsado |= j.puntero == puntero_jugador;
    if (!sigueUsado) punteros.removeOne(puntero_jugador);
    update_model();
}
