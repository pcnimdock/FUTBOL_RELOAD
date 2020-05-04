#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QImage>
#include <QPixmap>
#include <QFileDialog>
#include <qmath.h>
#include <QMessageBox>
#include "paleta_escudos_sim.h"

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

void MainWindow::cargar_imagen_reemplazo()
{

    QImage *imagen2=new QImage(ui->label_4->text());
    *imagen2=imagen2->scaled(52,65,Qt::IgnoreAspectRatio);
    QImage imagen3=imagen2->convertToFormat(QImage::Format_Indexed8,colors);
    ui->label_3->setScaledContents(true); //escalar la imagen en el qlabel
    ui->label_3->setPixmap(QPixmap::fromImage(imagen3)); //llevar el qimage al label
    delete imagen2;

}
void MainWindow::tratar_escudo_original()
{
    unsigned int i;
    qint32 paleta_miniesc[16]=
    {
        0x0000,
        0x186d14,
        0x86aa1c,
        0xaebed3,
        0xffe320,
        0x0000ff,
        0xef2000,
        0x6996be,
        0x6da6aa,
        0x004975,
        0xa2d324,
        0x005961,
        0x717171,
        0xa2a2a2,
        0xcfcfcf,
        0xffffff,

    };
    quint8 data;
    //elegir el archivo BIGFOTO
    QFile file(ui->label_2->text());
    //abrir archivo
     file.open(QIODevice::ReadOnly);
     QDataStream in(&file);

     for(i=0;i<0x18;i++)
     {
         in>>data;//mandar a la basura los bytes
     }
     colors.clear();
      qint32 color_r,color_g,color_b;
     for (i=0;i<256;i++)
     {
         color_r=paleta_escudos_sim[i][0];
         color_g=paleta_escudos_sim[i][1];
         color_b=paleta_escudos_sim[i][2];
         colors.append(qRgb(color_r,color_g,color_b));
     }
    qint16 ancho=52; //pixels ancho MINIESC
    qint16 alto=65; //pixels alto de una MINIESC
    //crear imagen vacia en formato de 8bits
    QImage  *imagen= new QImage(ancho,alto,QImage::Format_Indexed8);

    //añadir la paleta a la imagen
    imagen->setColorTable(colors);


    qint8 fila_imagen,columna_imagen;
    qint8 recorrido;
    for (recorrido=0;recorrido<4;recorrido++)
    {
        for(fila_imagen=0;fila_imagen<65;fila_imagen++)
        {
            columna_imagen=recorrido;
            while(columna_imagen<52)
            {
                in>>data;
                imagen->setPixel(columna_imagen,fila_imagen,((uint)data));
                columna_imagen=columna_imagen+4;
            }

        }
    }



    QString texto;
    QString fichero = "data.bmp";
    imagen->save(fichero ,"BMP"); //salvamos la imagen en un bmp

    ui->label->setScaledContents(true); //escalar la imagen en el qlabel
    ui->label->setPixmap(QPixmap::fromImage(*imagen)); //llevar el qimage al label

    QImage  *imagen_paleta= new QImage(ancho,alto,QImage::Format_Indexed8);

    //añadir la paleta a la imagen
    imagen_paleta->setColorTable(colors);

    for(i=0;i<alto;i++)
    {

       for(int j=0;j<ancho;j++)
       {
           imagen_paleta->setPixel(j,i,0);
       }
    }
    QString fichero2 = "plantilla_escudo.bmp";
    imagen_paleta->save(fichero2 ,"BMP"); //salvamos la imagen en un bmp



}
void MainWindow::on_btn_file_clicked()
{
    QString archivo = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    "",tr("Files (*.EMP)"));
    ui->label_2->setText(archivo);
    tratar_escudo_original();
}

void MainWindow::on_pushButton_clicked()
{
//convertir
    QByteArray data;
    qint16 color_pixel;
    qint16 valor;
    QImage *imagen2=new QImage(ui->label_4->text());
    *imagen2=imagen2->scaled(52,65,Qt::IgnoreAspectRatio);
    QImage imagen3=imagen2->convertToFormat(QImage::Format_Indexed8,colors);

    qint16 ancho=52;
    qint16 alto=65;

    quint8 a[ancho/8]; //vector de los 16 primeros bytes de linea
    quint8 b[ancho/8]; //""     ""  ""  "" segundos ""
    quint8 c[ancho/8];//""      ""  ""  "" terceros
    quint8 d[ancho/8];//""      ""  "" ""  ultimos
    data.clear();
        //se ha recorrido todos los píxeles de la imagen y ya se tiene la imagen codificada

    //terminar de tratar y sobreescribir emp
    qint8 data_temp;
    QFile file(ui->label_2->text());
    //abrir archivo
     file.open(QIODevice::ReadOnly);
     QDataStream in(&file);
     int i;
     qint8 cabecera[28];
     for(i=0;i<0x18;i++)
     {
         in>>cabecera[i];
     }
     file.close();
     QFile::remove(ui->label_2->text());

     QFile file2(ui->label_2->text());
     file2.open(QIODevice::WriteOnly);
     QDataStream out(&file2);
     for(i=0;i<0x18;i++)
     {

        out<<((qint8)cabecera[i]);
     }
     //escribir índices en el archivo
     qint8 fila_imagen,columna_imagen;
     qint8 recorrido;
     for (recorrido=0;recorrido<4;recorrido++)
     {
         for(fila_imagen=0;fila_imagen<65;fila_imagen++)
         {
             columna_imagen=recorrido;
             while(columna_imagen<52)
             {
                 out<<((qint8)imagen3.pixelIndex(columna_imagen,fila_imagen));
                 columna_imagen=columna_imagen+4;
             }

         }
     }

    // data_temp=0xFF;
    //out<< data_temp;
     data_temp=0;
    out<<((qint8)data_temp);
    out<<((qint8)data_temp);
    out<<((qint8)data_temp);
    out<<((qint8)data_temp);
    file2.close();
    QMessageBox mensaje;
    mensaje.setText("Hecho");
    mensaje.exec();

}

void MainWindow::on_pushButton_2_clicked()
{
//seleccion de escudo con el que reemplazar
    QString archivo = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    "",tr("Files (*.BMP)"));
    ui->label_4->setText(archivo);
    cargar_imagen_reemplazo();
}

void MainWindow::on_spinBox_valueChanged(int arg1)
{
tratar_escudo_original();
}
