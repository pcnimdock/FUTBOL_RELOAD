#include "mainwindow.h"
#include "dialog_sonidos.h"
#include "dialog_partida.h"
#include "pcfutbol_tactica.h"
#include "pcfutbol_audio.h"
#include "util_ventanas.h"
#include "dialog_ligas.h"
#include "dialog_traspasos.h"
#include "dialog_restaurar.h"
#include "pcfutbol_plantillas.h"
#include "pcfutbol_csv.h"
#include <QProgressDialog>
#include <QApplication>
#include "ui_mainwindow.h"
#include "pcfutbol_imagenes.h"
#include "pcfutbol_extras.h"
#include "pcfutbol_equipacion.h"
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QFileDialog>
#include <QPushButton>
#include <QLabel>
#include <QGraphicsDropShadowEffect>
#include <QPlainTextEdit>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QPalette>
#include <QColor>
#include <QMap>
#include <algorithm>



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(tr("Fútbol Reload — Editor de PC Fútbol 4.0 / 4.5"));

    // --- Iconos de los botones de acción (coherentes con style.qss) ---
    ui->pushButton->setIcon(QIcon(":/icons/folder.svg"));
    ui->btn_procesar->setIcon(QIcon(":/icons/process.svg"));
    ui->pushButton_2->setIcon(QIcon(":/icons/save.svg"));
    ui->pushButton_estrellas->setIcon(QIcon(":/icons/star.svg"));
    ui->boton_salir->setIcon(QIcon(":/icons/exit.svg"));
    ui->lineEdit_2->addAction(QIcon(":/icons/search.svg"), QLineEdit::LeadingPosition);
    ui->lineEdit_3->addAction(QIcon(":/icons/search.svg"), QLineEdit::LeadingPosition);
    ui->lineEdit_2->setPlaceholderText("Buscar jugador...");

    // --- Paleta oscura forzada para las tablas ---
    // El QSS de background-color/color a veces no se propaga de forma
    // fiable al viewport interno de QTableView (comportamiento conocido
    // de Qt), así que se fija también por código para garantizar que el
    // texto sea legible pase lo que pase con la hoja de estilos.
    {
        QPalette palTabla = ui->tableView_jugadores->palette();
        palTabla.setColor(QPalette::Base, QColor("#202325"));
        palTabla.setColor(QPalette::AlternateBase, QColor("#26292b"));
        palTabla.setColor(QPalette::Text, QColor("#eaeaea"));
        palTabla.setColor(QPalette::WindowText, QColor("#eaeaea"));
        palTabla.setColor(QPalette::Highlight, QColor("#86aa1c"));
        palTabla.setColor(QPalette::HighlightedText, QColor("#16210a"));
        ui->tableView_jugadores->setPalette(palTabla);
        ui->tableView_equipos->setPalette(palTabla);
    }
    ui->lineEdit_3->setPlaceholderText("Buscar equipo...");

    // --- Botones de exportar/importar imágenes (extraer y reinyectar
    //     todas las imágenes del juego: camisetas, escudos, fotos...) ---
    QPushButton *btnExportar = new QPushButton(QIcon(":/icons/export.svg"), "Exportar imágenes", this);
    QPushButton *btnImportar = new QPushButton(QIcon(":/icons/import.svg"), "Importar imágenes", this);
    btnExportar->setObjectName("btnExportarImagenes");
    btnImportar->setObjectName("btnImportarImagenes");

    // --- Botones de exportar/importar sonidos (SONIDOS/*.RAW <-> WAV) ---
    QPushButton *btnExportarSon = new QPushButton(QIcon(":/icons/export.svg"), "Exportar sonidos", this);
    QPushButton *btnImportarSon = new QPushButton(QIcon(":/icons/import.svg"), "Importar sonidos", this);
    btnExportarSon->setObjectName("btnExportarSonidos");
    btnImportarSon->setObjectName("btnImportarSonidos");
    QPushButton *btnSonidos = new QPushButton(QIcon(":/icons/import.svg"), "Sonidos del partido…", this);
    btnSonidos->setObjectName("btnSonidosPartido");
    btnSonidos->setToolTip("Sustituir sonidos por cualquier audio (WAV, MP3, OGG, FLAC…) convertido para el juego");

    QHBoxLayout *layoutBotonesImagenes = new QHBoxLayout();
    layoutBotonesImagenes->addWidget(btnExportar);
    layoutBotonesImagenes->addWidget(btnImportar);
    layoutBotonesImagenes->addWidget(btnExportarSon);
    layoutBotonesImagenes->addWidget(btnImportarSon);
    layoutBotonesImagenes->addWidget(btnSonidos);
    QPushButton *btnPartida = new QPushButton(QIcon(":/icons/import.svg"), "Partida guardada…", this);
    btnPartida->setObjectName("btnPartidaGuardada");
    btnPartida->setToolTip("Editar una partida de Liga Manager en curso (TACTICS/LIGA.0 … LIGA.9)");
    layoutBotonesImagenes->addWidget(btnPartida);
    connect(btnPartida, &QPushButton::clicked, this, &MainWindow::abrirPartida);

    // --- Composición de ligas (qué equipos juegan en Primera/Segunda) ---
    QPushButton *btnLigas = new QPushButton("Composición de ligas", this);
    btnLigas->setObjectName("btnComposicionLigas");
    btnLigas->setToolTip("Mover equipos entre Primera, Segunda y Segunda B");
    layoutBotonesImagenes->addWidget(btnLigas);
    connect(btnLigas, &QPushButton::clicked, this, [this]() {
        if (ui->lineEdit->text().isEmpty())
        {
            QMessageBox::warning(this, tr("FUTBOL RELOAD"),
                                 tr("Selecciona primero la carpeta del juego."));
            return;
        }
        DialogLigas dlg(ui->lineEdit->text(), this);
        dlg.exec();
    });

    // --- Plantillas: traspasos, hoja de cálculo, comprobación y deshacer ---
    QHBoxLayout *layoutPlantillas = new QHBoxLayout();
    QPushButton *btnTraspasos = new QPushButton("Traspasos…", this);
    QPushButton *btnExportarCsv = new QPushButton(QIcon(":/icons/export.svg"), "Exportar a CSV (Excel)…", this);
    QPushButton *btnImportarCsv = new QPushButton(QIcon(":/icons/import.svg"), "Importar CSV…", this);
    QPushButton *btnComprobar = new QPushButton("Comprobar plantillas", this);
    QPushButton *btnRestaurar = new QPushButton("Restaurar originales…", this);
    btnTraspasos->setObjectName("btnTraspasos");
    btnExportarCsv->setObjectName("btnExportarCsv");
    btnImportarCsv->setObjectName("btnImportarCsv");
    btnComprobar->setObjectName("btnComprobar");
    btnRestaurar->setObjectName("btnRestaurar");
    btnTraspasos->setToolTip("Mover jugadores de un equipo a otro");
    btnExportarCsv->setToolTip("Guardar jugadores y clubes en CSV para editarlos en Excel o LibreOffice");
    btnImportarCsv->setToolTip("Cargar un CSV editado (jugadores.csv o equipos.csv)");
    btnComprobar->setToolTip("Buscar lo que puede colgar el juego: alineaciones, dorsales, letras…");
    btnRestaurar->setToolTip("Devolver a su estado original los ficheros del juego que el editor ha cambiado");
    for (QPushButton *b : {btnTraspasos, btnExportarCsv, btnImportarCsv, btnComprobar, btnRestaurar})
        layoutPlantillas->addWidget(b);
    connect(btnTraspasos, &QPushButton::clicked, this, [this]() { abrirTraspasos(0); });
    connect(btnExportarCsv, &QPushButton::clicked, this, &MainWindow::exportarCsv);
    connect(btnImportarCsv, &QPushButton::clicked, this, &MainWindow::importarCsv);
    connect(btnComprobar, &QPushButton::clicked, this, [this]() { comprobarPlantillas(false); });
    connect(btnRestaurar, &QPushButton::clicked, this, &MainWindow::restaurarOriginales);

    // «Buscar equipo»: antes no hacía nada. Oculta las filas que no
    // coinciden (cada fila sigue siendo el índice del equipo en lista_equipos).
    connect(ui->lineEdit_3, &QLineEdit::textChanged, this, [this](const QString &f) {
        for (int i = 0; i < lista_equipos.size(); i++)
            ui->tableView_equipos->setRowHidden(i,
                !f.isEmpty() && !lista_equipos.at(i).NombreCorto.contains(f, Qt::CaseInsensitive)
                             && !lista_equipos.at(i).NombreLargo.contains(f, Qt::CaseInsensitive));
    });

    // Los añade al final del layout principal de la ventana central.
    if (auto *layoutPrincipal = qobject_cast<QVBoxLayout*>(centralWidget()->layout())) {
        layoutPrincipal->addLayout(layoutBotonesImagenes);
        layoutPrincipal->addLayout(layoutPlantillas);
    }

    connect(btnExportar, &QPushButton::clicked, this, &MainWindow::exportarImagenes);
    connect(btnImportar, &QPushButton::clicked, this, &MainWindow::importarImagenes);
    connect(btnExportarSon, &QPushButton::clicked, this, &MainWindow::exportarSonidos);
    connect(btnImportarSon, &QPushButton::clicked, this, &MainWindow::importarSonidos);
    connect(btnSonidos, &QPushButton::clicked, this, &MainWindow::abrirSonidos);

    // Imagen de "casilla vacía" cacheada: antes se recargaba del recurso
    // en cada refresco de la rejilla de equipos.
    imagenVacia = QImage(":/img/img/dataempty.bmp");

    // Los 20 botones de la rejilla de equipos se llaman EQ_BOTON_1..20
    // en el .ui; se localizan por nombre en vez de listarlos a mano.
    for(int i=0;i<20;i++)
    {
        mybutton *btn = findChild<mybutton*>(QString("EQ_BOTON_%1").arg(i+1));
        if(!btn) continue;
        list_eq_buttons.append(btn);
        btn->setId(i);
        btn->setEQname(QString("Empty %1").arg(QString::number(i)));
        btn->setIconEQ(imagenVacia);
        btn->setText("");
        EqButtonGroup.addButton(btn,i);
        connect(btn,SIGNAL(encima(int,char *)),this,SLOT(mouseOnEqButton(int,char *)));
    }
    connect(&EqButtonGroup, &QButtonGroup::idClicked, this, &MainWindow::EQ_button_clicked);

    for(QLabel *lbl : {ui->label, ui->label_2, ui->label_3})
    {
        auto *sombra = new QGraphicsDropShadowEffect(lbl);
        sombra->setBlurRadius(8);
        sombra->setOffset(2, 2);
        sombra->setColor(QColor(0, 0, 0, 140));
        lbl->setGraphicsEffect(sombra);
    }

    model = new QStandardItemModel(lista_jugadores_totales.size(),3,this);
    model2 = new QStandardItemModel(lista_equipos.size(),1,this);


    // Que la ventana quepa en pantallas pequeñas (contenido con barras de desplazamiento)
    UtilVentanas::hacerDesplazable(this);
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

    // --- Detección de la versión del juego (4.0 / 4.5) ---------------------
    version_pcf = detectarVersion(ui->lineEdit->text());
    db.configurarVersion(version_pcf, ui->lineEdit->text());
    const QString ver_txt = (version_pcf==40) ? "4.0" : "4.5";
    setWindowTitle(tr("Fútbol Reload — Editor de PC Fútbol %1").arg(ver_txt));
    ui->statusbar->showMessage(tr("PC Fútbol %1 detectado — la salida se escribirá en %2")
                                   .arg(ver_txt, db.carpetaSalida()));

    // --- Estrellas Mundiales ------------------------------------------------
    // El fichero es EQ959900.DBC en las copias reales de 4.0 y 4.5 (ver
    // BBDD_FUTBOL::configurarVersion). En ambos casos es el último de la lista ordenada y su
    // formato interno es IDÉNTICO (misma cabecera de 0x2C, id u16, nombre,
    // 43 bytes opacos, "SEGUNDA B", 275 bytes opacos y la lista de jugadores),
    // así que basta con leer el que exista y sacarlo de la lista de equipos.
    const QString nombre_estrellas = db.ficheroEstrellas();
    if(QString::compare(lista_archivos.last(), nombre_estrellas, Qt::CaseInsensitive)==0)
    {
        QString file_path=ui->lineEdit->text()+"/DBDAT/"+nombre_estrellas;

        QFile file_estrellas(file_path);
        if(!file_estrellas.open(QIODevice::ReadOnly))
        {
            QMessageBox::critical(this, tr("FUTBOL RELOAD"),
                                  tr("Error abriendo %1").arg(nombre_estrellas),
                                  QMessageBox::Ok);
            return;
        }
        QByteArray datos;
        datos=file_estrellas.readAll();
        file_estrellas.close();
        estrellas.ruta_pcf=ui->lineEdit->text();
        estrellas.guardar_datos_de_archivo(datos);
        lista_archivos.removeLast();
    }

    // Red de seguridad: cualquier EQ9599xx.DBC que quedara suelto (por una
    // detección de versión errónea o por una instalación mixta) NO puede
    // pasar por el parser genérico de equipos, porque no tiene ni entrenador
    // ni bloque de estadísticas y reventaría la lectura.
    for(int i=lista_archivos.size()-1;i>=0;i--)
    {
        if(lista_archivos.at(i).startsWith("EQ9599", Qt::CaseInsensitive))
            lista_archivos.removeAt(i);
    }

    // (Se eliminó aquí una pasada de depuración que reescribía ficheros
    // EQ*.DBC truncados y un arch_comp.csv en el directorio de trabajo:
    // era código de análisis que podía corromper datos del usuario.)

    // --- Determinar el número de kit (n) de cada equipo para poder leer
    //     su equipación real de PALETAS/EQUIPOS.DAT ---
    // n = posición 1-based del equipo entre los EQ*.DBC ordenados por su
    // ID numérico (los especiales 959900/959901 ya se han quitado de
    // lista_archivos más arriba, así que no hace falta excluirlos aquí).
    QStringList archivosOrdenadosPorId = lista_archivos;
    std::sort(archivosOrdenadosPorId.begin(), archivosOrdenadosPorId.end(),
               [](const QString &a, const QString &b)
    {
        QString na = a; na = na.remove(8,4).remove(0,2);
        QString nb = b; nb = nb.remove(8,4).remove(0,2);
        return na.toLong() < nb.toLong();
    });
    QMap<QString,int> numeroKitPorArchivo;
    for (int i = 0; i < archivosOrdenadosPorId.size(); i++)
    {
        numeroKitPorArchivo[archivosOrdenadosPorId.at(i)] = i + 1;
    }
    QString rutaEquiposDat = ui->lineEdit->text() + "/PALETAS/EQUIPOS.DAT";
    bool hayEquiposDat = QFile::exists(rutaEquiposDat);

    // --- Barra de progreso del procesado ------------------------------------
    QProgressDialog progreso(tr("Procesando equipos…"), tr("Cancelar"),
                             0, lista_archivos.size(), this);
    progreso.setWindowTitle(tr("FUTBOL RELOAD"));
    progreso.setWindowModality(Qt::WindowModal);
    progreso.setMinimumDuration(0);
    progreso.setAutoClose(true);
    int eq_procesados = 0;

    for (const QString &eq_file_str_it : std::as_const(lista_archivos)) {
        eq_file_str = eq_file_str_it;

        progreso.setValue(eq_procesados++);
        progreso.setLabelText(tr("Procesando %1  (%2 de %3)")
                                  .arg(eq_file_str)
                                  .arg(eq_procesados)
                                  .arg(lista_archivos.size()));
        QApplication::processEvents();
        if (progreso.wasCanceled())
        {
            lista_equipos.clear();
            lista_jugadores_totales.clear();
            ui->statusbar->showMessage(tr("Procesado cancelado."));
            return;
        }

        QString file_path;
        quint32 num_eq;
        QString num_str=eq_file_str;
        num_str=num_str.remove(8,4).remove(0,2);;
        num_eq=num_str.toLong();

        file_path = ui->lineEdit->text() + "/DBDAT/" + eq_file_str;
        QFile ficheroDbc(file_path);
        if(!ficheroDbc.open(QIODevice::ReadOnly))
        {
            QMessageBox::critical(this, tr("FUTBOL RELOAD"),
                                  tr("Error abriendo archivo\n")+eq_file_str,
                                  QMessageBox::Ok);
            return;
        }

        //se ha abierto el archivo: procesar
        QByteArray arch = ficheroDbc.readAll();
        ficheroDbc.close();
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
        // (Antes había aquí un caso especial para el equipo 959900 que saltaba
        //  a la posición fija 0x189. Era código muerto —Estrellas Mundiales se
        //  procesa con su propio parser y se elimina de lista_archivos— y
        //  además la constante sólo valía para 4.5: en 4.0 los jugadores del
        //  fichero de estrellas empiezan en 0x18A. Eliminado.)

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

                const quint32 inicioInfo = pos_memoria;
                quint8 num_cad_entr=arch.at(pos_memoria++);
                quint8 temp3=0;
                size_cad=arch.at(pos_memoria++);
                while(temp3<=num_cad_entr)
                {
                    for(temp=0;temp<size_cad;temp++){entrenador.Info.append(db.descodificar_caracter(arch.at(pos_memoria++)));}
                    size_cad=arch.at(pos_memoria++);
                    temp3++;
                }
                // (el bucle ya ha leído el primer byte del bloque siguiente)
                entrenador.InfoBruto = arch.mid(inicioInfo, pos_memoria - 1 - inicioInfo);
                entrenador.InfoLeido = entrenador.Info;
                const quint32 inicioDecl = pos_memoria - 1;
                num_cad_entr=size_cad;
                size_cad=arch.at(pos_memoria++);
                temp3=0;
                while(temp3<=num_cad_entr)
                {
                    for(temp=0;temp<size_cad;temp++){entrenador.Declaraciones.append(db.descodificar_caracter(arch.at(pos_memoria++)));}
                    size_cad=arch.at(pos_memoria++);
                    temp3++;
                }
                entrenador.DeclaracionesBruto = arch.mid(inicioDecl, pos_memoria - 1 - inicioDecl);
                entrenador.DeclaracionesLeido = entrenador.Declaraciones;
            }
            else
            {
                pos_memoria++;
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
            //tiene base de datos
            if(arch.at(pos_memoria-1)!=(1))
            {
                QMessageBox::critical(this, tr("FUTBOL RELOAD"),
                                      tr("Error al leer el archivo\n"
                                         "Se esperaba un 0x01 de jugador"),
                                      QMessageBox::Ok);
                return;
            }
        }
        else
        {
            //no tiene base de datos

            if(arch.at(pos_memoria-1)!=(1))
            {
                QMessageBox::critical(this, tr("FUTBOL RELOAD"),
                                      tr("Error al leer el archivo sin base de datos\n"
                                         "Se esperaba un 0x01 de jugador"),
                                      QMessageBox::Ok);
                return;
            }
            //pos_memoria++;

        }

        quint8 isJugador=1;
       // if(equipo.EquipoIdDBC==950405)
       // {pos_memoria--;}
       // if(equipo.EquipoIdDBC==950502)
       // {pos_memoria--;}
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

            // Posición del jugador en la pantalla ALINEACIÓN (1..N, único
            // por plantilla; suele coincidir con el dorsal). Ver jugador.h.
            jugador.PosicionEnAlineacion=arch.at(pos_memoria++);
            pos_memoria++; // byte separador 0x00


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
                const quint32 inicioComentario = pos_memoria;
                quint8 num_cadenas=arch.at(pos_memoria++);
                quint8 temp2=0;
                size_cad=arch.at(pos_memoria++);
                while(temp2<=num_cadenas)
                {
                    for(temp=0;temp<size_cad;temp++){jugador.Comentario.append(db.descodificar_caracter(arch.at(pos_memoria++)));}
                    size_cad=arch.at(pos_memoria++);
                    temp2++;
                }
                jugador.ComentarioBruto = arch.mid(inicioComentario, pos_memoria - 1 - inicioComentario);
                jugador.ComentarioLeido = jugador.Comentario;

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

        equipo.numero_kit = numeroKitPorArchivo.value(eq_file_str, -1);
        if(hayEquiposDat && equipo.numero_kit>0)
        {
            bool ok1=false, ok2=false;
            QByteArray b1 = PCFutbolEquipacion::leerBloqueEquipacion(rutaEquiposDat, equipo.numero_kit, true, &ok1);
            QByteArray b2 = PCFutbolEquipacion::leerBloqueEquipacion(rutaEquiposDat, equipo.numero_kit, false, &ok2);
            if(ok1) equipo.equipacion1_textura = b1.mid(8, PCFutbolEquipacion::TAM_TEXTURA);
            if(ok2) equipo.equipacion2_textura = b2.mid(8, PCFutbolEquipacion::TAM_TEXTURA);
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
    progreso.setValue(lista_archivos.size());

    for(num_boton_eq=0; num_boton_eq<20 && num_boton_eq<lista_equipos.size(); num_boton_eq++)
    {
        QImage im_interface = imagenVacia;
        if(!lista_equipos.at(num_boton_eq).miniesc.isEmpty())
            im_interface = transf_minifoto.miniesc2QImage(lista_equipos.at(num_boton_eq).miniesc);

        list_eq_buttons.at(num_boton_eq)->setEQname(lista_equipos.at(num_boton_eq).NombreLargo);
        list_eq_buttons.at(num_boton_eq)->setIconEQ(im_interface);
    }

    // Jugadores de los partidos de la temporada real: no pueden salir de su
    // equipo (ver pcfutbol_temporada_real.h)
    temporadaReal.cargar(ui->lineEdit->text(), lista_equipos);
    if (!temporadaReal.error().isEmpty())
        ui->statusbar->showMessage(tr("Aviso: %1").arg(temporadaReal.error()));

    refrescarTablas();
    ui->pushButton_2->setEnabled(true);

}

void MainWindow::mouseOnEqButton(int id,char *EQ_str)
{
    ui->label_3->setText("<center> "+QString(EQ_str).toUtf8() + "</center> ");

}

void MainWindow::on_horizontalScrollBar_valueChanged(int value)
{
    const int init_eq = value*20;
    if(init_eq>=lista_equipos.size())
        return;

    for(int num_boton_eq=0; num_boton_eq<list_eq_buttons.size(); num_boton_eq++)
    {
        const int idx = init_eq + num_boton_eq;
        if(idx < lista_equipos.size())
        {
            QImage im_interface = imagenVacia;
            if(!lista_equipos.at(idx).miniesc.isEmpty())
                im_interface = transf_minifoto.miniesc2QImage(lista_equipos.at(idx).miniesc);
            list_eq_buttons.at(num_boton_eq)->setEQname(lista_equipos.at(idx).NombreLargo);
            list_eq_buttons.at(num_boton_eq)->setIconEQ(im_interface);
        }
        else
        {
            list_eq_buttons.at(num_boton_eq)->setEQname(tr("VACIO"));
            list_eq_buttons.at(num_boton_eq)->setIconEQ(imagenVacia);
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
        dialog_equipo.setRutaJuego(ui->lineEdit->text());
        dialog_equipo.setTemporadaReal(&temporadaReal);
        if(QDialog::Accepted==dialog_equipo.exec())
        {
            eq =dialog_equipo.get_equipo();
            lista_equipos[num_eq]=eq;
            punteros = dialog_equipo.get_punteros();
            refrescarTablas();
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
    int num_eq=index.row();
    if(num_eq<lista_equipos.size())
    {
        //ejecutar dialog
        dialog_equipo.setModal(true);
        EQUIPO eq = lista_equipos.at(num_eq);
        dialog_equipo.set_equipo(eq);
        dialog_equipo.set_punteros(punteros);
        dialog_equipo.setRutaJuego(ui->lineEdit->text());
        dialog_equipo.setTemporadaReal(&temporadaReal);
        if(QDialog::Accepted==dialog_equipo.exec())
        {
            eq =dialog_equipo.get_equipo();
            lista_equipos[num_eq]=eq;
            punteros = dialog_equipo.get_punteros();
            refrescarTablas();
        }
        else
        {

        }
    }
}

void MainWindow::on_pushButton_2_clicked()
{
    if (!comprobarPlantillas(true))
        return;
    QString fileName = QFileDialog::getExistingDirectory(this,
                                                         tr("Carpeta para salvar estructura"));
    if(fileName.isEmpty())
        return;

    QStringList fallos;
    for(int i=0;i<lista_equipos.size();i++)
    {
        if(db.guardar_equipo(lista_equipos.at(i),fileName)==0)
        {
            fallos << QString("EQ%1.DBC")
                        .arg(QString::number(lista_equipos.at(i).EquipoIdDBC),6,QChar('0'));
        }
    }

    if(db.guardar_estrellas(estrellas,fileName)==0)
        fallos << tr("Estrellas mundiales");

    if(fallos.isEmpty())
    {
        QMessageBox::information(this, tr("FUTBOL RELOAD"),
                                 tr("Cambios guardados correctamente en\n%1/%2")
                                     .arg(fileName, db.carpetaSalida()));
    }
    else
    {
        QMessageBox::critical(this, tr("FUTBOL RELOAD"),
                              tr("No se pudieron escribir %1 archivo(s):\n%2")
                                  .arg(fallos.size())
                                  .arg(fallos.join("\n")));
    }

}

void MainWindow::on_tableView_jugadores_doubleClicked(const QModelIndex &index)
{
    // Cada fila guarda (equipo, jugador) en Qt::UserRole; antes se buscaba
    // por nombre y se podía abrir otro jugador con el mismo comienzo.
    if (!index.isValid()) return;
    const QModelIndex ind = model->index(index.row(), 1, QModelIndex());
    const int num_eq = ind.data(Qt::UserRole).toInt();
    const int num_jugador = ind.data(Qt::UserRole + 1).toInt();
    if (num_eq < 0 || num_eq >= lista_equipos.size()) return;
    if (num_jugador < 0 || num_jugador >= lista_equipos.at(num_eq).lista_jugadores.size()) return;

    Dialog_jugador dialog_jug;
    dialog_jug.set_jugador(lista_equipos.at(num_eq).lista_jugadores.at(num_jugador));
    if (QDialog::Accepted == dialog_jug.exec())
    {
        lista_equipos[num_eq].lista_jugadores[num_jugador] = dialog_jug.get_jugador();
        refrescarTablas();
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
int MainWindow::procesar_datos_equipo(EQUIPO *eq, const QByteArray &arch, quint32 *pos)
{
    quint32 size_cad;
    EQUIPO equipo;
    equipo=*eq;
    quint32 pos_memoria=0;
    for(pos_memoria=0;pos_memoria<0x2B;pos_memoria++)
    {
        equipo.cabecera.append(arch.at(pos_memoria));
    }
    equipo.tipo_dbc=arch.at(0x25);
    equipo.isBBDD_neg=arch.at(0x2A);
    equipo.pais=arch.at(0x2B);
    equipo.pais_string=equipo.get_nacion(equipo.pais);

    if(equipo.EquipoIdDBC==959901){equipo.isBBDD_neg=1;}
    equipo.jugable=arch.at(0x2A); //cuidado con las estrellas mundiales, sólo hay jugadores

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

    //leer las estadisticas
    quint32 offset=0;


    if(equipo.isBBDD_neg==0)
    {
        // --- BLOQUE DE ESTADÍSTICAS / PALMARÉS ------------------------------
        // Estructura EXACTA (verificada byte a byte contra los 234 DBC de
        // PC Fútbol 4.0 y compatible con 4.5):
        //
        //   [202 bytes fijos]  pares u16 LE por competición + campos varios
        //   [1 byte  ]         longitud L de la etiqueta de temporadas
        //   [L bytes ]         etiqueta codificada (" 2- 3- ... -18-19" en los
        //                      equipos con historial largo; "-" o "--" en los
        //                      equipos modestos)
        //   [47 bytes]         puesto por temporada (1 byte cada uno)
        //
        // La versión anterior del editor localizaba el final del bloque
        // buscando el texto "-17"/"-18"/"-19". Ese atajo FALLA en PC Fútbol
        // 4.0: 22 de los 43 equipos con base de datos (Albacete, Logroñés,
        // Eibar…) tienen etiquetas cortas del tipo "--" y el editor abortaba
        // con "No se encontró la marca de temporadas". Con la estructura real
        // no hace falta ninguna heurística y funciona en las dos versiones.
        const quint32 CAB_STATS  = 202;   // parte fija
        const quint32 COLA_STATS = 47;    // puestos por temporada

        if(pos_memoria + CAB_STATS >= (quint32)arch.size())
        {
            QMessageBox::warning(this, tr("FUTBOL RELOAD"),
                                 tr("Bloque de estadísticas truncado en el equipo %1.")
                                     .arg(equipo.EquipoIdDBC));
            return 1;
        }
        const quint32 len_etiqueta = (quint8)arch.at(pos_memoria + CAB_STATS);
        offset = CAB_STATS + 1 + len_etiqueta + COLA_STATS;

        if(pos_memoria + offset >= (quint32)arch.size())
        {
            QMessageBox::warning(this, tr("FUTBOL RELOAD"),
                                 tr("Bloque de estadísticas incoherente en el equipo %1.")
                                     .arg(equipo.EquipoIdDBC));
            return 1;
        }

        for(temp=0;temp<offset;temp++){
            equipo.cosas_bbdd.append(arch.at(pos_memoria));
            pos_memoria++;
        }

        // Comprobación barata: justo detrás debe venir el 0x02 del entrenador.
        if((quint8)arch.at(pos_memoria)!=0x02)
        {
            QMessageBox::warning(this, tr("FUTBOL RELOAD"),
                                 tr("Tras las estadísticas del equipo %1 no aparece\n"
                                    "la marca 0x02 de entrenador.")
                                     .arg(equipo.EquipoIdDBC));
            return 1;
        }
    }
    else
    {for(temp=0;temp<177;temp++){equipo.cosas_bbdd.append(arch.at(pos_memoria++));}}


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
    // búsqueda de jugador
    llenarTablaJugadores(arg1);
}

void MainWindow::on_pushButton_estrellas_clicked()
{
  dialog_estrellas_mundiales dialog_estrellas;

  dialog_estrellas.setModal(true);
  //identificar qué equipo es
  EQUIPO eq;
  eq.lista_jugadores=estrellas.list_jugador;
  eq.EquipoIdDBC=estrellas.estrellas_mundiales.EquipoIdDBC;
  eq.NombreCorto=estrellas.estrellas_mundiales.NombreCorto;
  eq.nombre_corto_mayusculas=estrellas.estrellas_mundiales.nombre_corto_mayusculas;
  dialog_estrellas.set_equipo(eq);
  dialog_estrellas.set_punteros(punteros);
  if(QDialog::Accepted==dialog_estrellas.exec())
  {
      eq =dialog_estrellas.get_equipo();
      estrellas.list_jugador=eq.lista_jugadores;
      estrellas.estrellas_mundiales.NombreCorto=eq.NombreCorto;
      estrellas.estrellas_mundiales.nombre_corto_mayusculas=eq.nombre_corto_mayusculas;
      punteros = dialog_estrellas.get_punteros();
  }

}

void MainWindow::exportarImagenes()
{
    // Si ya hay una ruta del juego escrita arriba, se usa esa y no se vuelve a
    // preguntar. Sólo se pide si está vacía o no parece una carpeta del juego.
    QString carpetaJuego = rutaJuegoActual();
    if (carpetaJuego.isEmpty())
    {
        carpetaJuego = QFileDialog::getExistingDirectory(
            this, "Selecciona la carpeta del juego (donde está PCFUTBOL.DAT)");
        if (carpetaJuego.isEmpty()) return;
    }

    QString carpetaSalida = QFileDialog::getExistingDirectory(
        this, "Selecciona (o crea) la carpeta donde guardar las imágenes");
    if (carpetaSalida.isEmpty()) return;

    QProgressDialog progreso(tr("Exportando imágenes…"), tr("Cancelar"), 0, 100, this);
    progreso.setWindowTitle(tr("Exportar imágenes"));
    progreso.setWindowModality(Qt::WindowModal);
    progreso.setMinimumDuration(0);
    progreso.setValue(0);

    auto avance = [&progreso](int hechos, int total, const QString &detalle) -> bool {
        if (total > 0)
        {
            progreso.setMaximum(total);
            progreso.setValue(hechos);
        }
        progreso.setLabelText(detalle);
        QApplication::processEvents();
        return !progreso.wasCanceled();
    };

    auto r = PCFutbolImagenes::exportarImagenes(carpetaJuego, carpetaSalida, avance);
    // Formatos añadidos: sprites .EMP, fondo por tiles (.BLO/.BIN) y textos .GRC
    PCFutbolExtras::Resultado x;
    if (!progreso.wasCanceled())
    {
        progreso.reset();
        progreso.setLabelText(tr("Exportando sprites, fondos y textos…"));
        progreso.setValue(0);
        PCFutbolExtras::exportarTodo(carpetaJuego, carpetaSalida, x, avance);
    }
    progreso.close();

    QString resumen = QString(
        "Ficheros escaneados: %1\n"
        "Ficheros con imagen reconocida: %2\n"
        "Imágenes extraídas: %3\n"
        "Frames de sprites (.EMP): %4\n"
        "Fondos del campo (.BLO/.BIN): %5\n"
        "Textos de interfaz (.GRC): %6\n\n"
        "Manifest: %7\n"
        "Los textos están en la subcarpeta TEXTOS (uno por fichero .GRC).")
        .arg(r.ficherosEscaneados)
        .arg(r.ficherosConImagen)
        .arg(r.imagenesExtraidas)
        .arg(x.spritesExportados)
        .arg(x.fondosExportados)
        .arg(x.textosExportados)
        .arg(r.manifestPath);

    const QStringList avisos = r.errores + x.errores;
    if (!avisos.isEmpty())
        resumen += "\n\nAvisos:\n" + avisos.join("\n");

    QMessageBox::information(this, "Exportación completada", resumen);
}

void MainWindow::importarImagenes()
{
    // Misma idea que en exportar: si la ruta del juego ya está puesta arriba,
    // se reutiliza como carpeta ORIGINAL sin volver a preguntar.
    QString carpetaJuego = rutaJuegoActual();
    if (carpetaJuego.isEmpty())
    {
        carpetaJuego = QFileDialog::getExistingDirectory(
            this, "Selecciona la carpeta ORIGINAL del juego (sin modificar)");
        if (carpetaJuego.isEmpty()) return;
    }

    QString carpetaImagenes = QFileDialog::getExistingDirectory(
        this, "Selecciona la carpeta de imágenes (con manifest.csv, editadas o no)");
    if (carpetaImagenes.isEmpty()) return;

    QString carpetaNueva = QFileDialog::getExistingDirectory(
        this, "Selecciona (vacía) dónde crear la copia nueva y modificada del juego");
    if (carpetaNueva.isEmpty()) return;

    QProgressDialog progreso(tr("Copiando el juego original…"), tr("Cancelar"), 0, 100, this);
    progreso.setWindowTitle(tr("Importar imágenes"));
    progreso.setWindowModality(Qt::WindowModal);
    progreso.setMinimumDuration(0);
    progreso.setValue(0);
    QApplication::processEvents();

    auto avance = [&progreso](int hechos, int total, const QString &detalle) -> bool {
        if (total > 0)
        {
            progreso.setMaximum(total);
            progreso.setValue(hechos);
        }
        progreso.setLabelText(detalle);
        QApplication::processEvents();
        return !progreso.wasCanceled();
    };

    auto r = PCFutbolImagenes::importarImagenes(carpetaJuego, carpetaImagenes,
                                                carpetaNueva, avance);
    // Se reinyecta sobre la copia YA creada por importarImagenes()
    PCFutbolExtras::Resultado x;
    if ((r.errores.isEmpty() || !r.carpetaNueva.isEmpty()) && !progreso.wasCanceled())
    {
        progreso.reset();
        progreso.setLabelText(tr("Reinyectando sprites, fondos y textos…"));
        progreso.setValue(0);
        PCFutbolExtras::importarTodo(carpetaNueva, carpetaImagenes, x, avance);
    }
    progreso.close();

    QString resumen = QString(
        "Ficheros reescritos: %1\n"
        "Imágenes reinyectadas: %2\n"
        "Frames de sprites (.EMP): %3\n"
        "Fondos del campo (.BLO/.BIN): %4\n"
        "Textos de interfaz (.GRC): %5\n\n"
        "Copia nueva del juego en:\n%6")
        .arg(r.ficherosReescritos)
        .arg(r.imagenesReinyectadas)
        .arg(x.spritesImportados)
        .arg(x.fondosImportados)
        .arg(x.textosImportados)
        .arg(r.carpetaNueva);

    const QStringList avisos = r.errores + x.errores;
    if (!avisos.isEmpty())
        resumen += "\n\nAvisos:\n" + avisos.join("\n");

    QMessageBox::information(this, "Importación completada", resumen);
}

// ===========================================================================
// SONIDOS
// ===========================================================================
void MainWindow::exportarSonidos()
{
    QString carpetaJuego = rutaJuegoActual();   // reutiliza la ruta ya elegida
    if (carpetaJuego.isEmpty())
    {
        carpetaJuego = QFileDialog::getExistingDirectory(
            this, "Selecciona la carpeta del juego (la que contiene SONIDOS)");
        if (carpetaJuego.isEmpty()) return;
    }

    QString carpetaSalida = QFileDialog::getExistingDirectory(
        this, "Selecciona (o crea) la carpeta donde guardar los WAV");
    if (carpetaSalida.isEmpty()) return;

    setCursor(Qt::WaitCursor);
    PCFutbolExtras::Resultado r;
    PCFutbolExtras::exportarSonidos(carpetaJuego, carpetaSalida, r);
    unsetCursor();

    QString resumen = QString(
        "Sonidos exportados: %1\n\n"
        "Están en la subcarpeta SONIDOS de:\n%2\n\n"
        "Son WAV mono de 8 bits a 11.025 Hz, con las muestras originales\n"
        "intactas. Puedes editarlos con cualquier programa de audio.")
        .arg(r.sonidosExportados)
        .arg(carpetaSalida);

    if (!r.errores.isEmpty())
        resumen += "\n\nAvisos:\n" + r.errores.join("\n");

    if (r.sonidosExportados == 0 && r.errores.isEmpty())
        resumen = "No se ha encontrado ningún .RAW en esa carpeta.";

    QMessageBox::information(this, "Exportación de sonidos", resumen);
}

void MainWindow::abrirPartida()
{
    const QString juego = rutaJuegoActual();
    const QString tactics = PCFutbolTactica::carpetaTactics(juego);
    const QString ruta = QFileDialog::getOpenFileName(this, "Partida de Liga Manager",
        tactics.isEmpty() ? juego : tactics, "Partidas de Liga Manager (LIGA.* liga.*);;Todos (*)");
    if (ruta.isEmpty()) return;
    PCFutbolPartida p;
    QString err;
    if (!p.cargar(ruta, &err))
    {
        QMessageBox::warning(this, "Partida guardada", err);
        return;
    }
    DialogPartida d(ruta, p, juego, this);
    d.exec();
}

void MainWindow::abrirSonidos()
{
    QString carpetaJuego = rutaJuegoActual();
    if (carpetaJuego.isEmpty() || PCFutbolAudio::carpetaSonidos(carpetaJuego).isEmpty())
    {
        carpetaJuego = QFileDialog::getExistingDirectory(
            this, "Selecciona la carpeta del juego (la que contiene SONIDOS)");
        if (carpetaJuego.isEmpty()) return;
    }
    DialogSonidos d(carpetaJuego, this);
    if (!d.valido())
    {
        QMessageBox::warning(this, "Sonidos", "No hay carpeta SONIDOS en:\n" + carpetaJuego);
        return;
    }
    d.exec();
}

void MainWindow::importarSonidos()
{
    QString carpetaJuego = QFileDialog::getExistingDirectory(
        this, "Selecciona la carpeta del juego SOBRE LA QUE ESCRIBIR (¡se modifica!)");
    if (carpetaJuego.isEmpty()) return;

    if (QMessageBox::question(this, "Confirmar",
            "Se van a sobrescribir los ficheros .RAW de:\n" + carpetaJuego +
            "\n\nLa primera vez se guarda una copia .bak de cada fichero (se puede deshacer con «Restaurar originales»).\n\n¿Continuar?",
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    QString carpetaSonidos = QFileDialog::getExistingDirectory(
        this, "Selecciona la carpeta con los audios (o su subcarpeta SONIDOS): GOL.wav, FONDO.mp3, …");
    if (carpetaSonidos.isEmpty()) return;

    setCursor(Qt::WaitCursor);
    PCFutbolExtras::Resultado r;
    PCFutbolExtras::importarSonidos(carpetaJuego, carpetaSonidos, r);
    unsetCursor();

    QString resumen = QString("Sonidos reinyectados: %1").arg(r.sonidosImportados);
    if (!r.detalles.isEmpty())
        resumen += "\n\n" + r.detalles.join("\n");
    if (!r.errores.isEmpty())
        resumen += "\n\nAvisos:\n" + r.errores.join("\n");

    QMessageBox::information(this, "Importación de sonidos", resumen);
}


// ---------------------------------------------------------------------------
// Detección de la versión de PC Fútbol instalada en la carpeta indicada.
//
// Comprobado con las dos copias reales (zips de 4.0 y 4.5):
//   - COMFILE.DAT dice "v3.0" en la 4.5, así que no sirve.
//   - Las DOS versiones llaman EQ959900.DBC a las Estrellas Mundiales.
//   - Las DOS tienen futbol4.exe (en la 4.5, en minúsculas).
//   - Sólo la 4.5 trae INFOFUT.EXE, ACTUALIZ.EXE y pcFut4.exe.
// Los nombres se comparan sin distinguir mayúsculas (en Linux sí importan).
// Si nada concluye, se asume 4.5.
// ---------------------------------------------------------------------------
int MainWindow::detectarVersion(const QString &rutaJuego)
{
    const QStringList ficheros = QDir(rutaJuego).entryList(QDir::Files);
    auto hay = [&ficheros](const QString &nombre) {
        return ficheros.contains(nombre, Qt::CaseInsensitive);
    };
    if(hay("INFOFUT.EXE") || hay("ACTUALIZ.EXE") || hay("PCFUT4.EXE") || hay("FUTBOL45.EXE"))
        return 45;
    if(hay("FUTBOL4.EXE") || hay("CRACK.EXE"))
        return 40;
    return 45;
}


// ---------------------------------------------------------------------------
// Devuelve la carpeta del juego que el usuario ya ha elegido arriba (lineEdit),
// siempre que exista y tenga pinta de serlo (contiene DBDAT). Si no, devuelve
// una cadena vacía y el llamante pedirá la carpeta con un diálogo.
// ---------------------------------------------------------------------------
QString MainWindow::rutaJuegoActual() const
{
    const QString ruta = ui->lineEdit->text().trimmed();
    if (ruta.isEmpty()) return QString();
    if (!QDir(ruta).exists())  return QString();
    if (!QDir(ruta + "/DBDAT").exists()) return QString();
    return ruta;
}


// ---------------------------------------------------------------------------
// Tablas de la ventana principal. Se rehacen desde lista_equipos después de
// cualquier cambio (ficha, traspasos, CSV) para que no muestren datos viejos.
// ---------------------------------------------------------------------------
void MainWindow::refrescarTablas()
{
    lista_jugadores_totales.clear();
    for (const EQUIPO &eq : lista_equipos)
        lista_jugadores_totales.append(eq.lista_jugadores);

    punteros.clear();
    for (const JUGADOR &j : lista_jugadores_totales) punteros.append(j.puntero);
    for (const JUGADOR &j : estrellas.list_jugador) punteros.append(j.puntero);
    std::sort(punteros.begin(), punteros.end());

    llenarTablaJugadores(ui->lineEdit_2->text());

    QStandardItemModel *viejo = model2;
    model2 = new QStandardItemModel(lista_equipos.size(), 1, this);
    for (int i = 0; i < lista_equipos.size(); i++)
        model2->setData(model2->index(i, 0), lista_equipos.at(i).NombreCorto);
    model2->setHeaderData(0, Qt::Horizontal, tr("Equipo"), Qt::DisplayRole);
    ui->tableView_equipos->setModel(model2);
    delete viejo;
    for (int i = 0; i < lista_equipos.size(); i++)        // mantener el filtro de equipos
        ui->tableView_equipos->setRowHidden(i, !ui->lineEdit_3->text().isEmpty() &&
            !lista_equipos.at(i).NombreCorto.contains(ui->lineEdit_3->text(), Qt::CaseInsensitive) &&
            !lista_equipos.at(i).NombreLargo.contains(ui->lineEdit_3->text(), Qt::CaseInsensitive));

    on_horizontalScrollBar_valueChanged(ui->horizontalScrollBar->value());
}

void MainWindow::llenarTablaJugadores(const QString &filtro)
{
    QStandardItemModel *viejo = model;
    model = new QStandardItemModel(0, 3, this);
    int fila = 0;
    for (int e = 0; e < lista_equipos.size(); e++)
    {
        const EQUIPO &eq = lista_equipos.at(e);
        for (int k = 0; k < eq.lista_jugadores.size(); k++)
        {
            const JUGADOR &j = eq.lista_jugadores.at(k);
            if (!filtro.isEmpty() && !j.NombreCorto.contains(filtro, Qt::CaseInsensitive))
                continue;
            QStandardItem *foto = new QStandardItem();
            if (!j.minifoto.isEmpty())
                foto->setData(QVariant(QPixmap::fromImage(transf_minifoto.minifoto2QImage(j.minifoto))),
                              Qt::DecorationRole);
            QStandardItem *nombre = new QStandardItem(j.NombreCorto);
            nombre->setData(e, Qt::UserRole);
            nombre->setData(k, Qt::UserRole + 1);
            model->setItem(fila, 0, foto);
            model->setItem(fila, 1, nombre);
            model->setItem(fila, 2, new QStandardItem(eq.NombreCorto));
            fila++;
        }
    }
    model->setHeaderData(0, Qt::Horizontal, tr("MiniFoto"), Qt::DisplayRole);
    model->setHeaderData(1, Qt::Horizontal, tr("Nombre"), Qt::DisplayRole);
    model->setHeaderData(2, Qt::Horizontal, tr("Equipo"), Qt::DisplayRole);
    ui->tableView_jugadores->setModel(model);
    delete viejo;
}

bool MainWindow::hayDatosCargados()
{
    if (!lista_equipos.isEmpty()) return true;
    QMessageBox::warning(this, tr("FUTBOL RELOAD"),
                         tr("Primero elige la carpeta del juego y pulsa «Procesar»."));
    return false;
}

void MainWindow::abrirTraspasos(int equipoInicial)
{
    if (!hayDatosCargados()) return;
    DialogTraspasos dlg(lista_equipos, equipoInicial, &temporadaReal, this);
    if (dlg.exec() == QDialog::Accepted && dlg.numeroTraspasos() > 0)
    {
        lista_equipos = dlg.equipos();
        refrescarTablas();
        ui->statusbar->showMessage(tr("%1 traspasos hechos. Recuerda pulsar Guardar.")
                                       .arg(dlg.numeroTraspasos()));
    }
}

// ---------------------------------------------------------------------------
// Comprobación de plantillas. Antes de guardar devuelve false si el usuario
// cancela; con problemas graves sólo se guarda si se corrigen o si el
// usuario insiste.
// ---------------------------------------------------------------------------
bool MainWindow::comprobarPlantillas(bool antesDeGuardar)
{
    if (!hayDatosCargados()) return false;
    for (;;)
    {
        const QList<PCFutbolPlantillas::Problema> probs =
            PCFutbolPlantillas::comprobar(lista_equipos, &temporadaReal);
        QStringList graves, avisos;
        bool corregible = false;
        for (const auto &p : probs)
        {
            (p.grave ? graves : avisos) << p.texto;
            corregible |= p.corregible;
        }
        if (probs.isEmpty())
        {
            if (!antesDeGuardar)
                QMessageBox::information(this, tr("Comprobar plantillas"),
                    tr("Todo correcto: %1 equipos revisados, nada que pueda colgar el juego.")
                        .arg(lista_equipos.size()));
            return true;
        }
        // Antes de guardar, los avisos leves solos no interrumpen
        if (antesDeGuardar && graves.isEmpty() && !corregible)
            return true;

        QDialog dlg(this);
        dlg.setWindowTitle(tr("Comprobar plantillas"));
        dlg.resize(760, 520);
        QVBoxLayout *lay = new QVBoxLayout(&dlg);
        QLabel *cab = new QLabel(&dlg);
        cab->setWordWrap(true);
        cab->setText(graves.isEmpty()
            ? tr("No hay nada grave. %1 avisos (cosas que no están en el juego original o que "
                 "pueden verse raras):").arg(avisos.size())
            : tr("<b>%1 problemas graves</b> (pueden colgar el juego o estropear el fichero) "
                 "y %2 avisos:").arg(graves.size()).arg(avisos.size()));
        lay->addWidget(cab);
        QPlainTextEdit *txt = new QPlainTextEdit(&dlg);
        txt->setReadOnly(true);
        QStringList lineas;
        for (const QString &g : graves) lineas << "✖ " + g;
        for (const QString &a : avisos) lineas << "⚠ " + a;
        txt->setPlainText(lineas.join("\n"));
        lay->addWidget(txt, 1);
        QDialogButtonBox *bb = new QDialogButtonBox(&dlg);
        QPushButton *bCorregir = corregible
            ? bb->addButton(tr("Corregir automáticamente"), QDialogButtonBox::ActionRole) : nullptr;
        QPushButton *bSeguir = antesDeGuardar
            ? bb->addButton(graves.isEmpty() ? tr("Guardar así") : tr("Guardar igualmente"),
                            QDialogButtonBox::AcceptRole) : nullptr;
        bb->addButton(antesDeGuardar ? tr("Cancelar") : tr("Cerrar"), QDialogButtonBox::RejectRole);
        QAbstractButton *pulsado = nullptr;
        connect(bb, &QDialogButtonBox::clicked, &dlg, [&](QAbstractButton *b) { pulsado = b; dlg.done(0); });
        lay->addWidget(bb);
        dlg.exec();

        if (pulsado && pulsado == bCorregir)
        {
            const QStringList hecho = PCFutbolPlantillas::corregirAutomaticamente(lista_equipos);
            refrescarTablas();
            QMessageBox mb(QMessageBox::Information, tr("Corregido"),
                           tr("%1 correcciones hechas.").arg(hecho.size()), QMessageBox::Ok, this);
            mb.setDetailedText(hecho.join("\n"));
            mb.exec();
            continue;       // volver a comprobar
        }
        return pulsado && pulsado == bSeguir;
    }
}

void MainWindow::exportarCsv()
{
    if (!hayDatosCargados()) return;
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Carpeta donde guardar los CSV"));
    if (dir.isEmpty()) return;
    const QString fj = dir + "/jugadores.csv", fe = dir + "/equipos.csv", fl = dir + "/LEEME_CSV.txt";
    if ((QFile::exists(fj) || QFile::exists(fe)) &&
        QMessageBox::question(this, tr("Exportar CSV"),
            tr("Ya hay jugadores.csv o equipos.csv en esa carpeta. ¿Sobrescribirlos?"),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;
    QString err;
    if (!PCFutbolCsv::exportarJugadores(lista_equipos, fj, &err) ||
        !PCFutbolCsv::exportarEquipos(lista_equipos, fe, &err) ||
        !PCFutbolCsv::escribirLeeme(fl, &err))
    {
        QMessageBox::critical(this, tr("Exportar CSV"), err);
        return;
    }
    QMessageBox::information(this, tr("Exportar CSV"),
        tr("Guardados:\n%1  (%2 jugadores)\n%3  (%4 clubes)\n%5  (explicación de columnas y códigos)\n\n"
           "Ábrelos con Excel o LibreOffice, cámbialos y cárgalos con «Importar CSV…».")
            .arg(fj).arg(lista_jugadores_totales.size()).arg(fe).arg(lista_equipos.size()).arg(fl));
}

void MainWindow::importarCsv()
{
    if (!hayDatosCargados()) return;
    const QString ruta = QFileDialog::getOpenFileName(this, tr("Importar CSV"), QString(),
                                                      tr("CSV (*.csv *.txt);;Todos (*)"));
    if (ruta.isEmpty()) return;
    PCFutbolCsv::Tabla t;
    QString err;
    if (!PCFutbolCsv::leer(ruta, &t, &err))
    {
        QMessageBox::critical(this, tr("Importar CSV"), err);
        return;
    }

    QList<EQUIPO> copia = lista_equipos;
    PCFutbolCsv::Resultado r;
    if (t.tipo == PCFutbolCsv::Jugadores)
    {
        bool borrar = false;
        const QStringList ausentes = PCFutbolCsv::jugadoresAusentes(lista_equipos, t);
        if (!ausentes.isEmpty())
        {
            QMessageBox mb(QMessageBox::Question, tr("Importar CSV"),
                tr("%1 jugadores del editor no están en el CSV (por ejemplo: %2).\n\n"
                   "¿Darlos de baja (quitarlos de su equipo)?\n"
                   "«No» los deja como están.")
                    .arg(ausentes.size()).arg(ausentes.mid(0, 3).join(", ")),
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, this);
            mb.setDefaultButton(QMessageBox::No);
            mb.setDetailedText(ausentes.join("\n"));
            const int resp = mb.exec();
            if (resp == QMessageBox::Cancel) return;
            borrar = resp == QMessageBox::Yes;
        }
        QList<quint16> otros;
        for (const JUGADOR &j : estrellas.list_jugador) otros << j.puntero;
        r = PCFutbolCsv::importarJugadores(copia, otros, t, borrar, &temporadaReal);
    }
    else
        r = PCFutbolCsv::importarEquipos(copia, t);

    QString resumen = (t.tipo == PCFutbolCsv::Jugadores)
        ? tr("Jugadores modificados: %1\nTraspasos: %2\nAltas: %3\nBajas: %4")
              .arg(r.modificados).arg(r.traspasos).arg(r.nuevos).arg(r.borrados)
        : tr("Clubes modificados: %1").arg(r.modificados);
    resumen += tr("\n\n(%1 filas leídas, %2, separador «%3»)")
                   .arg(t.filas.size()).arg(t.codificacion, t.separador == '\t' ? QString("tab") : QString(t.separador));
    if (!r.avisos.isEmpty())
        resumen += tr("\n\n%1 avisos (datos ajustados o filas ignoradas): ver detalles.").arg(r.avisos.size());
    const bool hayCambios = r.modificados + r.traspasos + r.nuevos + r.borrados > 0;
    QMessageBox mb(QMessageBox::Question, tr("Importar CSV"),
                   resumen + (hayCambios ? tr("\n\n¿Aplicar estos cambios en el editor?\n"
                                             "(Al juego no se escribe nada hasta pulsar Guardar.)")
                                         : tr("\n\nNo hay cambios que aplicar.")),
                   hayCambios ? (QMessageBox::Yes | QMessageBox::No) : QMessageBox::Ok, this);
    QStringList det = r.detalles;
    if (!r.avisos.isEmpty()) det << "" << tr("AVISOS:") << r.avisos;
    mb.setDetailedText(det.join("\n"));
    if (mb.exec() != QMessageBox::Yes) return;

    lista_equipos = copia;
    refrescarTablas();
    ui->statusbar->showMessage(tr("CSV importado. Recuerda pulsar Guardar."));
}

void MainWindow::restaurarOriginales()
{
    QString carpeta = rutaJuegoActual();
    if (carpeta.isEmpty())
    {
        carpeta = QFileDialog::getExistingDirectory(this, tr("Carpeta del juego"));
        if (carpeta.isEmpty()) return;
    }
    DialogRestaurar dlg(carpeta, this);
    dlg.exec();
}
