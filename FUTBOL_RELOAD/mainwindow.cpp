#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDir>
#include <QFile>
#include <QMessageBox>



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    list_eq_buttons.append(ui->EQ_BOTON_1);
    list_eq_buttons.append(ui->EQ_BOTON_2);
    list_eq_buttons.append(ui->EQ_BOTON_3);
    list_eq_buttons.append(ui->EQ_BOTON_4);
    list_eq_buttons.append(ui->EQ_BOTON_5);
    list_eq_buttons.append(ui->EQ_BOTON_6);
    list_eq_buttons.append(ui->EQ_BOTON_7);
    list_eq_buttons.append(ui->EQ_BOTON_8);
    list_eq_buttons.append(ui->EQ_BOTON_9);
    list_eq_buttons.append(ui->EQ_BOTON_10);
    list_eq_buttons.append(ui->EQ_BOTON_11);
    list_eq_buttons.append(ui->EQ_BOTON_12);
    list_eq_buttons.append(ui->EQ_BOTON_13);
    list_eq_buttons.append(ui->EQ_BOTON_14);
    list_eq_buttons.append(ui->EQ_BOTON_15);
    list_eq_buttons.append(ui->EQ_BOTON_16);
    list_eq_buttons.append(ui->EQ_BOTON_17);
    list_eq_buttons.append(ui->EQ_BOTON_18);
    list_eq_buttons.append(ui->EQ_BOTON_19);
    list_eq_buttons.append(ui->EQ_BOTON_20);
    QImage im(":/img/img/dataempty.bmp");
    for(int i=0;i<list_eq_buttons.size();i++)
    {
        list_eq_buttons.at(i)->setId(i);
        list_eq_buttons.at(i)->setEQname(QString("Empty %1").arg(QString::number(i)));
        list_eq_buttons.at(i)->setIconEQ(im);
        list_eq_buttons.at(i)->setText("");
        EqButtonGroup.addButton(list_eq_buttons.at(i),i);
        connect(list_eq_buttons.at(i),SIGNAL(encima(int,char *)),this,SLOT(mouseOnEqButton(int,char *)));
    }
    connect(&EqButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(EQ_button_clicked(int)));

    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect;
    QGraphicsDropShadowEffect *effect1 = new QGraphicsDropShadowEffect;
    QGraphicsDropShadowEffect *effect2 = new QGraphicsDropShadowEffect;
    effect->setBlurRadius(5);
    effect->setXOffset(5);
    effect->setYOffset(5);
    effect->setColor(Qt::darkGray);

    effect1->setBlurRadius(5);
    effect1->setXOffset(5);
    effect1->setYOffset(5);
    effect1->setColor(Qt::darkGray);

    effect2->setBlurRadius(5);
    effect2->setXOffset(5);
    effect2->setYOffset(5);
    effect2->setColor(Qt::darkGray);
    ui->label->setGraphicsEffect(effect);
    ui->label_2->setGraphicsEffect(effect1);
    ui->label_3->setGraphicsEffect(effect2);

    model = new QStandardItemModel(lista_jugadores_totales.size(),3,this);
    model2 = new QStandardItemModel(lista_equipos.size(),1,this);

}

MainWindow::~MainWindow()
{
    delete model;
    delete model2;
    delete ui;

}


void MainWindow::on_pushButton_clicked()
{
    QString fileName = QFileDialog::getExistingDirectory(this,
                                                         tr("Ruta a DBDAT"));
    ui->lineEdit->setText(fileName);

}

void MainWindow::on_btn_procesar_clicked()
{
    quint32 temp;
    lista_jugadores_totales.clear();

    if(ui->lineEdit->text().isEmpty())
        return;
    QDir dir(ui->lineEdit->text()+"/DBDAT");
    lista_archivos = dir.entryList(QStringList() << "*.dbc" << "*.DBC",QDir::Files);

    if(lista_archivos.empty())
    {
        QMessageBox::critical(this, tr("FUTBOL RELOAD"),
                              tr("No hay archivos en el directorio\n"
                                 "seleccionado"),
                              QMessageBox::Ok);
        return;
    }

    //procesado de archivos
    QString eq_file_str;

    quint8 num_boton_eq=0;
    lista_equipos.clear();
    if(QString::compare(lista_archivos.last(),"EQ959900.DBC", Qt::CaseInsensitive)==0)
    {
        QString file_path=ui->lineEdit->text()+"/DBDAT/EQ959900.DBC";
        QFile file_estrellas(file_path);
        if(!file_estrellas.open(QIODevice::ReadOnly))
        {
            QMessageBox::critical(this, tr("FUTBOL RELOAD"),
                                  tr("Error abriendo EQ959900.DBC\n")+eq_file_str,
                                  QMessageBox::Ok);

            return;
        }
        QByteArray datos;
        datos=file_estrellas.readAll();
        file_estrellas.close();
        estrellas.guardar_datos_de_archivo(datos);
        lista_archivos.removeLast();
    }
    foreach (eq_file_str, lista_archivos) {
        QString file_path;
        quint32 num_eq;
        QString num_str=eq_file_str;
        num_str=num_str.remove(8,4).remove(0,2);;
        num_eq=num_str.toLong();

        file_path = ui->lineEdit->text() + +"/DBDAT/" + eq_file_str;
        file = new QFile(file_path);
        if(!file->open(QIODevice::ReadOnly))
        {

            QMessageBox::critical(this, tr("FUTBOL RELOAD"),
                                  tr("Error abriendo archivo\n")+eq_file_str,
                                  QMessageBox::Ok);
            delete file;
            return;
        }

        //se ha abierto el archivo
        //procesar
        QByteArray arch;
        arch = file->readAll();
        quint32 pos_memoria=0;
        equipo.clear();
        equipo.EquipoIdDBC=num_eq;
        quint32 size_cad;

        if(procesar_datos_equipo(&equipo, arch,&pos_memoria)!=0)
        {//error
        }

        //EXTRAS

        QString path_minifoto_eq=ui->lineEdit->text() + "/DBDAT/MINIESC/" +
                QString("EQ%1.DFG").arg(QString::number(equipo.EquipoIdDBC),6,QChar('0'));

        QFile minifoto_eq(path_minifoto_eq);
        if(minifoto_eq.open(QIODevice::ReadOnly))
        {
            equipo.miniesc=minifoto_eq.readAll();

        }



        QString path_bigfoto_eq=ui->lineEdit->text() +"/DBDAT/BIGESC/" +
                QString("EQ%1.DFG").arg(QString::number(equipo.EquipoIdDBC),6,QChar('0'));

        QFile bigfoto_eq(path_bigfoto_eq);
        if(bigfoto_eq.open(QIODevice::ReadOnly))
        {
            equipo.bigesc=bigfoto_eq.readAll();
        }

        //foto de escudo de simulacion
        QString path_esc_sim=ui->lineEdit->text();
        path_esc_sim.append("/ESCUDOS/");
        path_esc_sim.append(QString("EQ%1.EMP").arg(QString::number(equipo.EquipoIdDBC),6,QChar('0')));
        QFile esc_sim(path_esc_sim);
        if(esc_sim.open(QIODevice::ReadOnly))
        {
            equipo.esc_sim=esc_sim.readAll();
        }


        //---- aquí se acaba la captura de datos del equipo
        pos_memoria++; // se presupone un 0x02 de entrenador
        lista_entrenadores.clear();


        quint8 isEntrenador=1;
        if(equipo.EquipoIdDBC==959900)
        {
            isEntrenador=0;
            pos_memoria=0x189;;
        }
        while(isEntrenador)
        {

            entrenador.clear();
            entrenador.EquipoIdDBC=equipo.EquipoIdDBC;
            entrenador.puntero=arch.at(pos_memoria++)&0xFF;
            entrenador.puntero+=(arch.at(pos_memoria++)&0xFF)*256;
            size_cad=arch.at(pos_memoria++);
            for(temp=0;temp<size_cad;temp++){entrenador.NombreCorto.append(db.descodificar_caracter(arch.at(pos_memoria++)));}
            if(equipo.isBBDD_neg==0)
            {
                size_cad=arch.at(pos_memoria++);
                for(temp=0;temp<size_cad;temp++){entrenador.NombreLargo.append(db.descodificar_caracter(arch.at(pos_memoria++)));}

                quint8 num_cad_entr=arch.at(pos_memoria++);
                quint8 temp3=0;
                size_cad=arch.at(pos_memoria++);
                while(temp3<=num_cad_entr)
                {
                    for(temp=0;temp<size_cad;temp++){entrenador.Info.append(db.descodificar_caracter(arch.at(pos_memoria++)));}
                    size_cad=arch.at(pos_memoria++);
                    temp3++;
                }
                num_cad_entr=size_cad;
                size_cad=arch.at(pos_memoria++);
                temp3=0;
                while(temp3<=num_cad_entr)
                {
                    for(temp=0;temp<size_cad;temp++){entrenador.Declaraciones.append(db.descodificar_caracter(arch.at(pos_memoria++)));}
                    size_cad=arch.at(pos_memoria++);
                    temp3++;
                }
            }

            //EXTRAS ---------------
            //pcfutbol/FUTBOL45/DBDAT/MINIENTR/
            QString path_minifoto_entr=ui->lineEdit->text() + "/DBDAT/MINIENTR/" +
                    QString("E95%1.DFG").arg(QString::number(entrenador.puntero),5,QChar('0'));

            QFile minifoto_entr(path_minifoto_entr);
            if(minifoto_entr.open(QIODevice::ReadOnly))
            {
                entrenador.minifoto=minifoto_entr.readAll();
            }

            QString path_bigfoto_entr=ui->lineEdit->text() + "/DBDAT/BIGENTR/" +
                    QString("E95%1.DFG").arg(QString::number(entrenador.puntero),5,QChar('0'));

            QFile bigfoto_entr(path_bigfoto_entr);
            if(bigfoto_entr.open(QIODevice::ReadOnly))
            {
                entrenador.bigfoto=bigfoto_entr.readAll();
            }

            //Almacenar al entrenador en memoria
            lista_entrenadores.append(entrenador);
            //comprobar si el siguiente es un entrenador o un jugador
            if(arch.at(pos_memoria++)!=2)
            {
                isEntrenador=0;
            }
            //FIN BUCLE BUSQUEDA ENTRENADORES
        }
        if(equipo.isBBDD_neg==0)
        {
            if(arch.at(pos_memoria-1)!=(1))
            {
                QMessageBox::critical(this, tr("FUTBOL RELOAD"),
                                      tr("Error al leer el archivo\n"
                                         "Se esperaba un 0x01 de jugador"),
                                      QMessageBox::Ok);
                return;
            }
        }
        else if(arch.at(pos_memoria++)!=(1))
        {

            QMessageBox::critical(this, tr("FUTBOL RELOAD"),
                                  tr("Error al leer el archivo de segunda B\n"
                                     "Se esperaba un 0x01 de jugador"),
                                  QMessageBox::Ok);
            return;
        }

        quint8 isJugador=1;

        while(isJugador)
        {
            jugador.clear();
            jugador.EquipoIdDBC=equipo.EquipoIdDBC;
            jugador.puntero=arch.at(pos_memoria++)&0xFF;
            jugador.puntero+=(arch.at(pos_memoria++)&0xFF)*256;
            size_cad=arch.at(pos_memoria++);
            for(temp=0;temp<size_cad;temp++){jugador.NombreCorto.append(db.descodificar_caracter(arch.at(pos_memoria++)));}
            size_cad=arch.at(pos_memoria++);
            for(temp=0;temp<size_cad;temp++){jugador.NombreLargo.append(db.descodificar_caracter(arch.at(pos_memoria++)));}


            jugador.NumeroCamiseta=arch.at(pos_memoria++);
            jugador.EstadoFichaje=arch.at(pos_memoria++); //de la base de datos (de 0 a 3 //por comprobar)
            jugador.DemarcacionPref=arch.at(pos_memoria++);
            for(temp=0;temp<5;temp++){
                jugador.DemarcacionesSecundarias.append(arch.at(pos_memoria++));}

            //------------------------ que coño es esto
            pos_memoria++; //lo mismo que el número de camiseta
            pos_memoria++; // hay un byte 0x00
            //-----------------------------------------------------


            jugador.Extranjero=arch.at(pos_memoria++);
            jugador.Piel=arch.at(pos_memoria++);
            jugador.Pelo=arch.at(pos_memoria++);
            jugador.PosicionEnCampo=arch.at(pos_memoria++); //0 portero, 1 defensor, 2 volante, 3 delantero
            if(equipo.isBBDD_neg==0)
            {
                size_cad=arch.at(pos_memoria++);
                for(temp=0;temp<size_cad;temp++){jugador.LugarDeNacimiento.append(db.descodificar_caracter(arch.at(pos_memoria++)));}
                jugador.FechaDeNacimiento.append(arch.at(pos_memoria++)); //DIA
                jugador.FechaDeNacimiento.append(arch.at(pos_memoria++)); //MES
                jugador.FechaDeNacimiento.append(arch.at(pos_memoria++)); //AÑO
                size_cad=arch.at(pos_memoria++);
                for(temp=0;temp<size_cad;temp++){jugador.EquipoProcedencia.append(db.descodificar_caracter(arch.at(pos_memoria++)));}
                size_cad=arch.at(pos_memoria++);
                for(temp=0;temp<size_cad;temp++){jugador.Internacional.append(db.descodificar_caracter(arch.at(pos_memoria++)));}
                jugador.Altura=arch.at(pos_memoria++);
                jugador.Peso=arch.at(pos_memoria++);
                quint8 num_cadenas=arch.at(pos_memoria++);
                quint8 temp2=0;
                size_cad=arch.at(pos_memoria++);
                while(temp2<=num_cadenas)
                {
                    for(temp=0;temp<size_cad;temp++){jugador.Comentario.append(db.descodificar_caracter(arch.at(pos_memoria++)));}
                    size_cad=arch.at(pos_memoria++);
                    temp2++;
                }

                jugador.Precio=size_cad&0xFF;
                jugador.Precio+=(arch.at(pos_memoria++)&0xFF)*256;
            }

            if(equipo.EquipoIdDBC==959900)
            {
                jugador.Precio=arch.at(pos_memoria++)&0xFF;
                jugador.Precio+=(arch.at(pos_memoria++)&0xFF)*256;
            }
            jugador.Velocidad=arch.at(pos_memoria++);
            jugador.Regate=arch.at(pos_memoria++);
            jugador.Agilidad=arch.at(pos_memoria++);
            jugador.Agresividad=arch.at(pos_memoria++);

            jugador.ActTecnica=arch.at(pos_memoria++);
            jugador.ActRemate=arch.at(pos_memoria++);
            jugador.ActPase=arch.at(pos_memoria++);
            jugador.ActTiro=arch.at(pos_memoria++);
            //aqui debe haber un 0x00

            //EXTRAS
            //buscar minifoto y añadir si existe
            QString path_minifoto=ui->lineEdit->text() +  QString("/DBDAT/MINIFOTO/J95%1.DFG").arg(QString::number(jugador.puntero),5,QChar('0'));

            QFile minifoto(path_minifoto);
            if(minifoto.open(QIODevice::ReadOnly))
            {
                jugador.minifoto=minifoto.readAll();
            }

            //buscar bigfoto y añadir si existe

            QString path_bigfoto=ui->lineEdit->text() + "/DBDAT/BIGFOTO/" +
                    QString("EQ%1").arg(QString::number(equipo.EquipoIdDBC),4,QChar('0')) + "/"
                    + QString("J95%1.DFG").arg(QString::number(jugador.puntero),5,QChar('0'));

            QFile bigfoto(path_bigfoto);
            if(bigfoto.open(QIODevice::ReadOnly))
            {
                jugador.bigfoto=bigfoto.readAll();
            }

            lista_jugadores.append(jugador);
            pos_memoria++;
            if(arch.at(pos_memoria++)!=1)
            {
                isJugador=0;
            }

            //FIN DE BUCLE DE JUGADOR
        }
        // se ha recorrido todo el fichero
        //grabar a sql

        //DEBUG


        //_______________
        if(equipo.EquipoIdDBC==959900)
        {
            equipo.isBBDD_neg=0;
        }
        lista_equipos.append(equipo);
        lista_equipos.last().lista_jugadores=lista_jugadores;
        lista_equipos.last().lista_entrenadores=lista_entrenadores;

        db.add_equipo(equipo,lista_entrenadores,lista_jugadores);
        lista_entrenadores.clear();
        for (int j=0;j<lista_jugadores.size();j++)
        {
            lista_jugadores_totales.append(lista_jugadores.at(j));
        }
        lista_jugadores.clear();


        //FIN BUCLE RASTREO DE ARCHIVOS
    }

    for(num_boton_eq=0; num_boton_eq<20;num_boton_eq++)
    {
        QImage im_interface(":/img/img/dataempty.bmp");
        if(!lista_equipos.at(num_boton_eq).miniesc.isEmpty())
        { im_interface = transf_minifoto.miniesc2QImage(lista_equipos.at(num_boton_eq).miniesc);}

        list_eq_buttons.at(num_boton_eq)->setEQname(lista_equipos.at(num_boton_eq).NombreLargo);
        list_eq_buttons.at(num_boton_eq)->setIconEQ(im_interface);

    }

    punteros.clear();
    for(int i=0; i<lista_jugadores_totales.size();i++)
    {
        punteros.append(lista_jugadores_totales.at(i).puntero);
    }
    qSort(punteros.begin(),punteros.end());

    //mostrar todos los jugadores con su equipo en el tableviewjugadores

    delete model;
    delete model2;

    model = new QStandardItemModel(lista_jugadores_totales.size(),3,this);
    model2 = new QStandardItemModel(lista_equipos.size(),1,this);


    for(int i=0;i<lista_jugadores_totales.size();i++)
    {
        QModelIndex index = model->index(i,1,QModelIndex());
        model->setData(index,lista_jugadores_totales.at(i).NombreCorto);

        if(lista_jugadores_totales.at(i).minifoto.isEmpty()==false)
        {
            QImage im = transf_minifoto.minifoto2QImage(lista_jugadores_totales.at(i).minifoto);
            index = model->index(i,0,QModelIndex());
            QStandardItem *item = new QStandardItem();
            item->setData(QVariant(QPixmap::fromImage(im)), Qt::DecorationRole);
            //model->setData(index,QVariant(QPixmap::fromImage(im)));
            model->setItem(i,0,item);
            // delete item;
        }
        //buscar el equipo al que corresponde equipoiddbc
        QString EqStr;
        for(int k=0;k<lista_equipos.size();k++)
        {
            if(lista_equipos.at(k).EquipoIdDBC==lista_jugadores_totales.at(i).EquipoIdDBC)
            {EqStr=lista_equipos.at(k).NombreCorto;
                break;
            }
        }
        index = model->index(i,2,QModelIndex());
        model->setData(index,EqStr);

    }
    model->setHeaderData(0, Qt::Horizontal, tr("MiniFoto"), Qt::DisplayRole);
    model->setHeaderData(1, Qt::Horizontal, tr("Nombre"), Qt::DisplayRole);
    model->setHeaderData(2, Qt::Horizontal, tr("Equipo"), Qt::DisplayRole);
    ui->tableView_jugadores->setModel(model);
    //mostrar los equipos en tableviewequipos

    for(int i=0;i<lista_equipos.size();i++)
    {
        QModelIndex index = model2->index(i,0,QModelIndex());
        model2->setData(index,lista_equipos.at(i).NombreCorto);
    }

    model2->setHeaderData(0, Qt::Horizontal, tr("Equipo"), Qt::DisplayRole);
    ui->tableView_equipos->setModel(model2);
    ui->tableView_jugadores->setStyleSheet("QTableView::item:selected { color:white; background:black; font-weight:1000; }"
                                           "QTableCornerButton::section { background-color:#232326; }"
                                           "QHeaderView::section { color:white; background-color:#a2d324; }"
                                           "QTableView {background:white;}");

    ui->tableView_equipos->setStyleSheet("QTableView::item:selected { color:white; background:black; font-weight:1000; }"
                                         "QTableCornerButton::section { background-color:#232326; }"
                                         "QHeaderView::section { color:white; background-color:#a2d324; }"
                                         "QTableView {background:white;}");

    ui->pushButton_2->setEnabled(true);

}

void MainWindow::mouseOnEqButton(int id,char *EQ_str)
{
    ui->label_3->setText("<center> "+QString(EQ_str).toUtf8() + "</center> ");

}

void MainWindow::on_horizontalScrollBar_valueChanged(int value)
{
    quint16 num_boton_eq;
    quint16 init_eq = value*20;
    if(init_eq<lista_equipos.size())
    {
        for(num_boton_eq=0; num_boton_eq<20;num_boton_eq++)
        {
            if((init_eq+num_boton_eq)<lista_equipos.size())
            {
                QImage im_interface(":/img/img/dataempty.bmp");
                if(!lista_equipos.at(num_boton_eq+init_eq).miniesc.isEmpty())
                {im_interface = transf_minifoto.miniesc2QImage(lista_equipos.at(num_boton_eq+init_eq).miniesc);}
                list_eq_buttons.at(num_boton_eq)->setEQname(lista_equipos.at(num_boton_eq+init_eq).NombreLargo);
                list_eq_buttons.at(num_boton_eq)->setIconEQ(im_interface);
            }
            else
            {
                QImage im(":/img/img/dataempty.bmp");
                list_eq_buttons.at(num_boton_eq)->setEQname("VACIO");
                list_eq_buttons.at(num_boton_eq)->setIconEQ(im);


            }

        }
    }
}

void MainWindow::EQ_button_clicked(int id)
{
    DialogEquipos dialog_equipo;
    dialog_equipo.setModal(true);
    //identificar qué equipo es
    int num_eq=ui->horizontalScrollBar->value();
    num_eq*=20;
    num_eq+=id;
    if(num_eq<lista_equipos.size())
    {
        //ejecutar dialog
        dialog_equipo.setModal(true);
        EQUIPO eq = lista_equipos.at(num_eq);
        dialog_equipo.set_equipo(eq);
        dialog_equipo.set_punteros(punteros);
        if(QDialog::Accepted==dialog_equipo.exec())
        {
            eq =dialog_equipo.get_equipo();
            lista_equipos[num_eq]=eq;
            punteros = dialog_equipo.get_punteros();
        }
        else
        {

        }

    }



}

void MainWindow::on_tableView_equipos_doubleClicked(const QModelIndex &index)
{
    //se ha apretado dos veces a un equipo

    DialogEquipos dialog_equipo;
    dialog_equipo.setModal(true);
    //identificar qué equipo es
    for(int i=0;i<lista_equipos.size();i++)
    {

    }
    int num_eq=index.row();
    if(num_eq<lista_equipos.size())
    {
        //ejecutar dialog
        dialog_equipo.setModal(true);
        EQUIPO eq = lista_equipos.at(num_eq);
        dialog_equipo.set_equipo(eq);
        dialog_equipo.set_punteros(punteros);
        if(QDialog::Accepted==dialog_equipo.exec())
        {
            eq =dialog_equipo.get_equipo();
            lista_equipos[num_eq]=eq;
        }
        else
        {

        }
    }
}

void MainWindow::on_pushButton_2_clicked()
{
    QString fileName = QFileDialog::getExistingDirectory(this,"Carpeta para salvar estructura");
    quint8 err=0;
    for(int i=0;i<lista_equipos.size();i++)
    {
        quint8 err=db.guardar_equipo(lista_equipos.at(i),fileName);
        if(err==0)
        {
            QMessageBox::critical(this, tr("FUTBOL RELOAD"),
                                  tr("ERROR AL ESCRIBIR\n"
                                     "Archivo EQ").arg(QString::number(lista_equipos.at(i).EquipoIdDBC)),
                                  QMessageBox::Ok);
        }
    }
        QMessageBox::information(this, tr("FUTBOL RELOAD"),
                              tr("GUARDADO\n"
                                 "CAMBIOS"),
                              QMessageBox::Ok);

}

void MainWindow::on_tableView_jugadores_doubleClicked(const QModelIndex &index)
{
    QString nombre_equipo;
    QString nombre_jugador;
    JUGADOR jug;

    if (index.isValid()) {
        int row = index.row();
        QModelIndex ind = model->index(row,2,QModelIndex());
        nombre_equipo = model->data(ind).toString();
        ind = model->index(row,1,QModelIndex());
        nombre_jugador = model->data(ind).toString();
    }
    else
    {
        return;
    }
    //buscar el equipo en la lista de equipos
    quint16 num_eq;

    for(num_eq=0;num_eq<lista_equipos.size();num_eq++)
    {
        if(lista_equipos.at(num_eq).NombreCorto.indexOf(nombre_equipo,0,Qt::CaseSensitive)==0)
        {
            break;
        }
    }
    if(num_eq>=lista_equipos.size())
    {
        return;
    }

    //buscar el jugador en el equipo
    quint16 num_jugador=0;
    quint16 num_jugadores_en_equipo=lista_equipos.at(num_eq).lista_jugadores.size();
    QString nombre_jug_temp;
    for(num_jugador=0;num_jugador<num_jugadores_en_equipo;num_jugador++)
    {
        nombre_jug_temp=lista_equipos.at(num_eq).lista_jugadores.at(num_jugador).NombreCorto;
        if(nombre_jug_temp.indexOf(nombre_jugador,0,Qt::CaseSensitive)==0)
        {
            jug=lista_equipos.at(num_eq).lista_jugadores.at(num_jugador);
            break;
        }
    }
    if(jug.NombreCorto.isEmpty())
    {
        return;
    }
    Dialog_jugador dialog_jug;

    dialog_jug.set_jugador(jug);
    if(QDialog::Accepted==dialog_jug.exec())
    {
        jug =dialog_jug.get_jugador();
        lista_equipos[num_eq].lista_jugadores[num_jugador]=jug;
    }
    else
    {

    }



}


//todos los equipos donde pone en la variable
// equipos.nombre_corto_mayusculas = "SEGUNDA B"
// comparten este bloque de valores
const unsigned char cosas_bbdd_SEGUNDA_B[177] =
{
    0x00, 0x00, 0x03, 0x00,
    0x02, 0x00, 0x03, 0x00,
    0x00, 0x00, 0x04, 0x00,
    0x00, 0x00, 0x04, 0x00,
    0x00, 0x00, 0x06, 0x00,
    0x0e, 0x00, 0x03, 0x00,
    0x02, 0x00, 0x07, 0x00,
    0x09, 0x00, 0x07, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x0d, 0x00, 0x03, 0x00,
    0x02, 0x00, 0x01, 0x00,
    0x09, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x03, 0x00,
    0x06, 0x00, 0x04, 0x00,
    0x02, 0x00, 0x05, 0x00,
    0x04, 0x00, 0x05, 0x00,
    0x00, 0x00, 0x02, 0x00,
    0x07, 0x00, 0x04, 0x00,
    0x02, 0x00, 0x03, 0x00,
    0x05, 0x00, 0x03, 0x00,
    0x00, 0x00, 0x02, 0x00,
    0x09, 0x00, 0x05, 0x00,
    0x03, 0x00, 0x04, 0x00,
    0x06, 0x00, 0x04, 0x00,
    0x01, 0x00, 0x05, 0x00,
    0x0b, 0x00, 0x04, 0x00,
    0x05, 0x00, 0x07, 0x00,
    0x09, 0x00, 0x06, 0x00,
    0x01, 0x00, 0x00, 0x00,
    0x0c, 0x00, 0x04, 0x00,
    0x05, 0x00, 0x01, 0x00,
    0x09, 0x00, 0x02, 0x00,
    0x06, 0x00, 0x03, 0x00,
    0x09, 0x00, 0x05, 0x00,
    0x07, 0x00, 0x06, 0x00,
    0x0e, 0x00, 0x05, 0x00,
    0x02, 0x00, 0x02, 0x00,
    0x09, 0x00, 0x05, 0x00,
    0x06, 0x00, 0x04, 0x00,
    0x09, 0x00, 0x04, 0x00,
    0x05, 0x00, 0x00, 0x00,
    0x0a, 0x00, 0x05, 0x00,
    0x07, 0x00, 0x02, 0x00,
    0x0c, 0x00, 0x03, 0x00,
    0x00
};

QByteArray qbyte_ant;
int MainWindow::procesar_datos_equipo(EQUIPO *eq, QByteArray arch,quint32 *pos)
{
    quint32 size_cad;
    EQUIPO equipo;
    equipo=*eq;
    quint32 pos_memoria;
    for(pos_memoria=0;pos_memoria<0x2A;pos_memoria++)
    {
        equipo.cabecera.append(arch.at(pos_memoria));
    }
    //pos_memoria=36; //cadena de copyrigth y primer byte 0

    //equipo.byte1_know=arch.at(0x25);
    equipo.isBBDD_neg=arch.at(0x2A);
    if(equipo.EquipoIdDBC==959900){equipo.isBBDD_neg=1;}
    equipo.jugable=arch.at(0x2B); //cuidado con las estrellas mundiales, sólo hay jugadores
    size_cad=arch.at(0x2C);
    pos_memoria=0x2D;

    quint32 temp=0;
    for(temp=0;temp<size_cad;temp++)
    {
        equipo.NombreLargo.append(db.descodificar_caracter(arch.at(pos_memoria)));
        pos_memoria++;
    }
    size_cad=arch.at(pos_memoria++);
    for(temp=0;temp<size_cad;temp++)
    {
        equipo.NombreCorto.append(db.descodificar_caracter(arch.at(pos_memoria)));
        pos_memoria++;
    }

    size_cad=arch.at(pos_memoria++);
    for(temp=0;temp<size_cad;temp++)
    {
        equipo.NombreMayusculas.append(db.descodificar_caracter(arch.at(pos_memoria)));
        pos_memoria++;
    }

    if(equipo.isBBDD_neg==0)
    {
        equipo.Bytes_unknow3_5bytes.clear();
        for(temp=0;temp<5;temp++)
        {
            equipo.Bytes_unknow3_5bytes.append(arch.at(pos_memoria));
            pos_memoria++;
        }

        equipo.anyo_fundacion=arch.at(pos_memoria++)&0xFF;
        equipo.anyo_fundacion+=(arch.at(pos_memoria++)&0xFF)*256;

    }
    size_cad=arch.at(pos_memoria++);
    for(temp=0;temp<size_cad;temp++)
    {
        equipo.estadio.append(db.descodificar_caracter(arch.at(pos_memoria)));
        pos_memoria++;
    }

    equipo.aforo=arch.at(pos_memoria++)&0xFF;
    equipo.aforo+=(arch.at(pos_memoria++)&0xFF)*256;
    equipo.aforo+=(arch.at(pos_memoria++)&0xFF)*256*256;
    equipo.aforo+=(arch.at(pos_memoria++)&0xFF)*256*256*256;

    if(equipo.isBBDD_neg==0)
    {
        //por vaguería de no comprobar si se puede colocar directamente en memoria para little endian


        equipo.num_socios=arch.at(pos_memoria++)&0xFF; //esto no lo coge bien
        equipo.num_socios+=(arch.at(pos_memoria++)&0xFF)*256;
        equipo.num_socios+=(arch.at(pos_memoria++)&0xFF)*256*256;
        equipo.num_socios+=(arch.at(pos_memoria++)&0xFF)*256*256*256;

        size_cad=arch.at(pos_memoria++);
        for(temp=0;temp<size_cad;temp++)
        {equipo.presidente.append(db.descodificar_caracter(arch.at(pos_memoria++)));}

        equipo.presupuesto_bbdd=arch.at(pos_memoria++)&0xFF;
        equipo.presupuesto_bbdd+=(arch.at(pos_memoria++)&0xFF)*256;
        equipo.presupuesto_bbdd+=(arch.at(pos_memoria++)&0xFF)*256*256;
        equipo.presupuesto_bbdd+=(arch.at(pos_memoria++)&0xFF)*256*256*256;

        equipo.presupuesto_juego=arch.at(pos_memoria++)&0xFF; // hay que comprobar bien esto
        equipo.presupuesto_juego+=(arch.at(pos_memoria++)&0xFF)*256;
        equipo.presupuesto_juego+=(arch.at(pos_memoria++)&0xFF)*256*256;
        equipo.presupuesto_juego+=(arch.at(pos_memoria++)&0xFF)*256*256*256;

        size_cad=arch.at(pos_memoria++);
        for(temp=0;temp<size_cad;temp++)
        {equipo.patrocinador.append(db.descodificar_caracter(arch.at(pos_memoria++)));}

        size_cad=arch.at(pos_memoria++);
        for(temp=0;temp<size_cad;temp++)
        {equipo.proveedor.append(db.descodificar_caracter(arch.at(pos_memoria++)));}


        size_cad=10;
        for(temp=0;temp<size_cad;temp++){equipo.bytes2_array_de_10.append(arch.at(pos_memoria++));}

    }

    size_cad=arch.at(pos_memoria++);
    for(temp=0;temp<size_cad;temp++){equipo.nombre_corto_mayusculas.append(db.descodificar_caracter(arch.at(pos_memoria++)));}

    if(equipo.isBBDD_neg==0)
    {
        if(equipo.EquipoIdDBC>959000)
        {//pcfutbol apertura
        for(temp=0;temp<253;temp++){equipo.cosas_bbdd.append(arch.at(pos_memoria++));}
        //la posición de memoria está después de "-17" en la parte de ¿estadisticas?
        //buscar hasta que la decodificación no sea un caracter numeral ni un '-'
        while(((db.descodificar_caracter(arch.at(pos_memoria))>='0')&&(db.descodificar_caracter(arch.at(pos_memoria))<='9'))
              || (db.descodificar_caracter(arch.at(pos_memoria))==0x2D))
            {
            equipo.cosas_bbdd.append(arch.at(pos_memoria++));
            }
        for(temp=0;temp<59;temp++){equipo.cosas_bbdd.append(arch.at(pos_memoria++));}
        }
        else
        {for(temp=0;temp<306;temp++){equipo.cosas_bbdd.append(arch.at(pos_memoria++));}}
    }
    else
    {for(temp=0;temp<177;temp++){equipo.cosas_bbdd.append(arch.at(pos_memoria++));}}

    //QFile file_debug_bbdd(QString("DBG/dbg%1").arg(QString::number(equipo.EquipoIdDBC)));
    QFile file_debug_bbdd(QString("DBG/dbg_cab.bi1"));
    //if(!file_debug_bbdd.open(QIODevice::WriteOnly))
    if(!file_debug_bbdd.open(QIODevice::ReadWrite))
    {
        //error
    }
    QByteArray temp2 = arch.mid(0x24,7);
    quint8 zero=0;
    temp2.append(zero);
    file_debug_bbdd.readAll();
    file_debug_bbdd.write(temp2); //cabecera
   // file_debug_bbdd.write(equipo.cosas_bbdd);
    //comparar archivos pequeños
    if((!qbyte_ant.isEmpty())&&(QString::compare(equipo.nombre_corto_mayusculas,"SEGUNDA B",Qt::CaseSensitive)!=0))
    {
        if((qbyte_ant.size()==177)&&(equipo.cosas_bbdd.size()==177))
        {
            //comparar con archivo anterior
            QFile diff_dbg(QString("DBG/diffs.bin"));
            if(!diff_dbg.open(QIODevice::ReadWrite))
            {

            }
            QByteArray temp;
            for(int i=0; i<equipo.cosas_bbdd.size();i++)
            {
                if(equipo.cosas_bbdd.at(i)==qbyte_ant.at(i))
                {temp.append(equipo.cosas_bbdd.at(i));}
                else
                {temp.append(0xFF);}
            }
            QByteArray cosas_arch,reemplazar;
            cosas_arch = diff_dbg.readAll();
            if(!cosas_arch.isEmpty())
            {
                for(int i=0; i<temp.size();i++)
                {
                    if(temp.at(i)==cosas_arch.at(i))
                    {reemplazar.append(cosas_arch.at(i));}
                    else
                    {reemplazar.append(0xFF);}
                }
            }
            else
            {
                reemplazar=temp;
            }
            diff_dbg.seek(0);
            //añadir cabecera

            diff_dbg.write(reemplazar);
            diff_dbg.close();



        }

    }
    if(QString::compare(equipo.nombre_corto_mayusculas,"SEGUNDA B",Qt::CaseSensitive)!=0)
    {
        qbyte_ant=equipo.cosas_bbdd;
    }
    *eq=equipo;
    *pos=pos_memoria;
    return 0;
}

void MainWindow::on_lineEdit_textChanged(const QString &arg1)
{
    ui->btn_procesar->setEnabled(true);
}

void MainWindow::on_lineEdit_2_textChanged(const QString &arg1)
{
    //linedit de busqueda de jugador
   QList <quint32> IdBC;
    lista_busqueda_jugador.clear();
    lista_busqueda_equipo.clear();
    lista_busqueda_minifoto.clear();

    for(int i=0; i<lista_jugadores_totales.size(); i++)
    {
        if(lista_jugadores_totales.at(i).NombreCorto.indexOf(arg1,0,Qt::CaseInsensitive)!=(-1))
        {
           lista_busqueda_jugador.append(lista_jugadores_totales.at(i).NombreCorto);
           lista_busqueda_minifoto.append(lista_jugadores_totales.at(i).minifoto);
           IdBC.append(lista_jugadores_totales.at(i).EquipoIdDBC);
        }
    }

    //settear model con nueva lista
    delete model;

    model = new QStandardItemModel(lista_busqueda_jugador.size(),3,this);


    for(int i=0;i<lista_busqueda_jugador.size();i++)
    {
        QModelIndex index = model->index(i,1,QModelIndex());
        model->setData(index,lista_busqueda_jugador.at(i));

        if(lista_busqueda_minifoto.at(i).isEmpty()==false)
        {
            QImage im = transf_minifoto.minifoto2QImage(lista_busqueda_minifoto.at(i));
            index = model->index(i,0,QModelIndex());
            QStandardItem *item = new QStandardItem();
            item->setData(QVariant(QPixmap::fromImage(im)), Qt::DecorationRole);
            model->setItem(i,0,item);

        }
        //buscar el equipo al que corresponde equipoiddbc
        QString EqStr;
        for(int k=0;k<(lista_equipos.size());k++)
        {
            if(lista_equipos.at(k).EquipoIdDBC==IdBC.at(i))
            {
                EqStr=lista_equipos.at(k).NombreCorto;
                break;
            }
        }
        index = model->index(i,2,QModelIndex());
        model->setData(index,EqStr);

    }
    model->setHeaderData(0, Qt::Horizontal, tr("MiniFoto"), Qt::DisplayRole);
    model->setHeaderData(1, Qt::Horizontal, tr("Nombre"), Qt::DisplayRole);
    model->setHeaderData(2, Qt::Horizontal, tr("Equipo"), Qt::DisplayRole);
    ui->tableView_jugadores->setModel(model);

}
