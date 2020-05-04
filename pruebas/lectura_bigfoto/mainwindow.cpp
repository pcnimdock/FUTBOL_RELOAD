#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QImage>
#include <QPixmap>
#include <QFileDialog>
#include <qmath.h>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{
    unsigned int i;
    QVector<QRgb> colors; //iniciar paleta de colores
    quint8 data;
    //elegir el archivo BIGFOTO
    QFile file(ui->label_2->text());
    //abrir archivo
     file.open(QIODevice::ReadOnly);

     qint8 color[3];

     QDataStream in(&file);
     for(i=0;i<2;i++)
     {
         in>>data;//leer dos datos que se repiten en todas los BIFOTO
                //y tirarlos a la basura
     }
     colors.clear();

     for(i=0;i<16;i++)
     {
         //leer la paleta de colores en el archivo
         in>>color[0];
         in>>color[1];
         in>>color[2];
         //añadir color a la paleta
         colors.append(qRgb((qint16)color[0],(qint16)color[1],(qint16)color[2]));
     }
     for(i=0;i<8;i++)
     {
         //tirar a la basura los siguientes 8 bytes
         in>>color[0];
     }

    qint16 ancho=128; //pixels ancho de una BIGFOTO
    qint16 alto=182; //pixels alto de una BIGFOTO
    //crear imagen vacia en formato de 8bits
    QImage  *imagen= new QImage(ancho,alto,QImage::Format_Indexed8);

    //añadir la paleta a la imagen
    imagen->setColorTable(colors);

    int p=0;

    quint8 a[17]; //vector de los 16 primeros bytes de linea
    quint8 b[17]; //""     ""  ""  "" segundos ""
    quint8 c[17];//""      ""  ""  "" terceros
    quint8 d[17];//""      ""  "" ""  ultimos

    for(int z=0;z<alto;z++)
    {
        //El dfg se divide en 4 bloques
        //la combinacion de los 4 bloques contiene el indice de colores
        //Leemos la fila completa
        //en 4 partes
        for(int j=0;j<16;j++)
        {
            in>>a[j];
        }
        for(int j=0;j<16;j++)
        {
            in>>b[j];
        }
        for(int j=0;j<16;j++)
        {
            in>>c[j];
        }
        for(int j=0;j<16;j++)
        {
            in>>d[j];
        }

        for(int j=0;j<16;j++)
        {
            //tratamos la fila completa
            quint16 ui_dato[8];
            for(i=0;i<8;i++)
            {
                //tratamiento de los 8 bits
                ui_dato[7-i]=(((a[j]>>i)&0x01)<<0 )+ (((b[j]>>i)&0x01)<<1) + (((c[j]>>i)&0x01)<<2) +
                        (((d[j]>>i)&0x01)<<3);
            }
            //aplicamos en qimage
            for(i=0;i<8;i++)
            {
                imagen->setPixel(i+8*j,z,(ui_dato[i]));

            }


        }


    }

    QString texto;
    QString fichero = "data.bmp";
    imagen->save(fichero ,"BMP"); //salvamos la imagen en un bmp

    ui->label->setScaledContents(true); //escalar la imagen en el qlabel
    ui->label->setPixmap(QPixmap::fromImage(*imagen)); //llevar el qimage al label
}

void MainWindow::on_btn_file_clicked()
{
    QString archivo = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    "",tr("Files (*.DFG)"));
    ui->label_2->setText(archivo);
}
