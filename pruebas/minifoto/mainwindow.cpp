#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QImage>
#include <QPixmap>
#include <QFileDialog>
#include <qmath.h>
#include <QMessageBox>

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
    *imagen2=imagen2->scaled(24,24,Qt::IgnoreAspectRatio);
    QImage imagen3=imagen2->convertToFormat(QImage::Format_Indexed8,colors);
    ui->label_3->setScaledContents(true); //escalar la imagen en el qlabel
    ui->label_3->setPixmap(QPixmap::fromImage(imagen3)); //llevar el qimage al label
    delete imagen2;

}
void MainWindow::tratar_escudo_original()
{
    qint32 paleta_minifoto[16]=
    {
        0x0000,
        0x186d14,
        0x86aa1c,
        0xa6b6cb,
        0xf7db18,
        0x0010e3,
        0xf70000,
        0x618eb6,
        0x592400,
        0x9a6124,
        0xdb9224,
        0xffd786,
        0x696969,
        0x9a9a9a,
        0xc7c7c7,
        0xf7f7f7,
    };
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
    unsigned int i;
    qint32 color_r,color_g,color_b;

    quint8 data;
    //elegir el archivo BIGFOTO
    QFile file(ui->label_2->text());
    //abrir archivo
     file.open(QIODevice::ReadOnly);
     QDataStream in(&file);
//     for(i=0;i<8;i++)
//     {
//        in>>data;//mandar a la basura los primeros 8 bytes
//     }
     colors.clear();
    //     for(i=0;i<1;i++)
//     {
//         //leer la paleta de colores en el archivo
//         in>>color[0];
//         in>>color[1];
//         in>>color[2];
//         //añadir color a la paleta
//       //  colors.append(qRgb((qint16)color[0],(qint16)color[1],(qint16)color[2]));
//     }
 //    for(i=0;i<8;i++)
 //    {
         //tirar a la basura los siguientes 8 bytes
 //        in>>color[0];
 //    }

     for (int i=0;i<16;i++)
     {
         color_r=(paleta_minifoto[i]>>16)&0xFF;
         color_g=(paleta_minifoto[i]>>8)&0xFF;
         color_b=(paleta_minifoto[i])&0xFF;
         colors.append(qRgb(color_r,color_g,color_b));
     }
//     qint16 color[3];
//     for(i=0;i<16;i++)
//         {
//             //leer la paleta de colores en el archivo
//             in>>color[0];
//             in>>color[1];
//             in>>color[2];
//             //añadir color a la paleta
//             colors.append(qRgb((qint16)color[0],(qint16)color[1],(qint16)color[2]));
//         }

    qint16 ancho=24;
    qint16 alto=24;

    //qint16 ancho=48; //pixels ancho MINIESC
    //qint16 alto=64; //pixels alto de una MINIESC
    //crear imagen vacia en formato de 8bits
    QImage  *imagen= new QImage(ancho,alto,QImage::Format_Indexed8);

    //añadir la paleta a la imagen
    imagen->setColorTable(colors);



    quint8 a[17]; //vector de los 16 primeros bytes de linea
    quint8 b[17]; //""     ""  ""  "" segundos ""
    quint8 c[17];//""      ""  ""  "" terceros
    quint8 d[17];//""      ""  "" ""  ultimos

    for(i=0;i<58;i++)
    {
    in>>a[0];//quitar cabecera
    }
    for(int z=0;z<alto;z++)
    {
        //El dfg se divide en 4 bloques
        //la combinacion de los 4 bloques contiene el indice de colores
        //Leemos la fila completa
        //en 4 partes
        int j=0;
        while(j<ancho/8)
        {
            in>>a[j];
            j++;
        }
        j=0;
        while(j<ancho/8)
        {    in>>b[j];j++;
        }
        j=0;
        while(j<ancho/8)
        {    in>>c[j];j++;
        }
        j=0;
        while(j<ancho/8)
        {    in>>d[j];j++;
        }
        j=0;
        while(j<ancho/8)//el número de bytes capturados, 6 para miniesc
        {
            //hay que imprimir 48 pixels por fila

            //obtenemos el color de 8 píxeles

            quint16 ui_dato[8];
            i=0;
            while(i<8)
            {
                //tratamiento de los 8 bits
                ui_dato[7-i]=(((a[j]>>i)&0x01)<<0 )+ (((b[j]>>i)&0x01)<<1) + (((c[j]>>i)&0x01)<<2) +
                        (((d[j]>>i)&0x01)<<3);
            i++;
            }
            //ya tenemos los colores de 8 píxeles
            //aplicamos en qimage
            i=0;
            while(i<8)
            {
                imagen->setPixel(i+8*j,z,(ui_dato[i]));
            i++;
            }

        j++;
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
    uint index_color_paleta=0;
    for(i=0;i<alto;i++)
    {
        index_color_paleta=i/4;
       for(int j=0;j<ancho;j++)
       {
           imagen_paleta->setPixel(j,i,index_color_paleta);
       }
    }
    QString fichero2 = "paleta_minifoto.bmp";
    imagen_paleta->save(fichero2 ,"BMP"); //salvamos la imagen en un bmp



}
void MainWindow::on_btn_file_clicked()
{
    QString archivo = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    "",tr("Files (*.DFG)"));
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
    *imagen2=imagen2->scaled(24,24,Qt::IgnoreAspectRatio);
    QImage imagen3=imagen2->convertToFormat(QImage::Format_Indexed8,colors);

    qint16 ancho=24;
    qint16 alto=24;

    quint8 a[ancho/8]; //vector de los 16 primeros bytes de linea
    quint8 b[ancho/8]; //""     ""  ""  "" segundos ""
    quint8 c[ancho/8];//""      ""  ""  "" terceros
    quint8 d[ancho/8];//""      ""  "" ""  ultimos
    data.clear();
    for(int i=0;i<alto;i++)
    {
        //recorrer todas la filas
        for(int z=0; z<ancho/8;z++) //inicializar todo a 0
        {a[z]=b[z]=c[z]=d[z]=0;}
        int j;
        for(j=0;j<ancho;j++)
        {
            //recorrer todos los píxeles de la fila
            color_pixel=imagen3.pixelIndex(j,i);
            a[j/8]|=(color_pixel&0x01)<<(7-j%8);
            b[j/8]|=((color_pixel>>1)&0x01)<<(7-j%8);
            c[j/8]|=((color_pixel>>2)&0x01)<<(7-j%8);
            d[j/8]|=((color_pixel>>3)&0x01)<<(7-j%8);


        }
        //se ha recorrido toda la fila, guardar en qbytearray para posterior tratamiento
        for(j=0;j<ancho/8;j++)
        {
        data.append(a[j]);
        }
        for(j=0;j<ancho/8;j++)
        {
        data.append(b[j]);
        }
        for(j=0;j<ancho/8;j++)
        {
        data.append(c[j]);
        }
        for(j=0;j<ancho/8;j++)
        {
        data.append(d[j]);
        }
    }
    //se ha recorrido todos los píxeles de la imagen y ya se tiene la imagen codificada

    //terminar de tratar y sobreescribir dfg
    qint8 data_temp;
    QFile file(ui->label_2->text());
    //abrir archivo
     file.open(QIODevice::ReadOnly);
     QDataStream in(&file);
     int i;
     qint8 cabecera[60];
     for(i=0;i<58;i++)
     {
         in>>cabecera[i];
     }
     file.close();
     QFile::remove(ui->label_2->text());

     QFile file2(ui->label_2->text());
     file2.open(QIODevice::WriteOnly);
     QDataStream out(&file2);
     for(i=0;i<58;i++)
     {

        out<<cabecera[i];//mandar a la basura los primeros 8 bytes
     }
     for(i=0;i<data.size();i++)
     {
         data_temp=data.at(i);
         out<<data_temp;
     }
     data_temp=0xFF;
    out<< data_temp;

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

}
