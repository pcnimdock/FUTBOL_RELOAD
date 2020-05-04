#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QByteArray>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

//quint8 caracters[] ={     'Ç','ü','é','â','ä','à','å','ç','ê','ë',
//                          'è','ï','î','ì','Ä','Å','É','æ','Æ','ô',
//                          'ö','ò','û','ù','ÿ','Ö','Ü','¢','£','¥',
//                          '.','.','á','í','ó','ú','ñ','Ñ','ª','º',
//                          '¿','.','¬','½','¼','¡','«','»'};

const quint8 caracteres[130]={  0xC7 ,0xFC ,0xE9 ,0xE2 ,0xE4 ,0xE0 ,0xE5 ,0xE7 ,0xEA ,0xEB ,
                       0xE8 ,0xEF ,0xEE ,0xEC ,0xC4 ,0xC5 ,0xC9 ,0xE6 ,0xC6 ,0xF4 ,
                       0xF6 ,0xF2 ,0xFB ,0xF9 ,0xFF ,0xD6 ,0xDC ,0xA2 ,0xA3 ,0xA5 ,
                       0x00 ,0x00 ,0xE1 ,0xED ,0xF3 ,0xFA ,0xF1 ,0xD1 ,0xAA ,0xBA ,
                       0xBF ,0x81 ,0xAC ,0xBD ,0xBC ,0xA1 ,0xAB ,0xBB ,0x00 ,0x00 ,
                       0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,
                       0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,
                       0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,
                       0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,
                       0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,
                       0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,
                       0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,
                       0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00
                       };
void MainWindow::on_pushButton_clicked()
{
    QString resultado2_string="/home/fran/resultado2.bin";


    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Image"), "/home/fran/.wine/drive_c/FUTBOL45/DBDAT", tr("Image Files (*.dbc)"));
    file = new QFile(fileName);
    if(!file->open(QIODevice::ReadWrite))
    {return;}

    QByteArray arr;
    QByteArray arr2;
    arr2.clear();
    arr.clear();
    arr.append(file->readAll());


    quint32 i;

    //hacer un xor con 0x61 a todos los valores
    qint32 size_arr=arr.size();
    for(i=0;i<size_arr;i++)
    {
        quint8 val=arr.at(i);
        val^=0x61;
        if(val&0x80)
        {
            val&=(~0x80);
            val=caracteres[val];
        }
        arr2.append(val);
    }

    arr.clear();
    file2 = new QFile(resultado2_string);
    if(!file2->open(QIODevice::ReadWrite))
    {return;}

    file2->write(arr2);
      delete file;
    delete file2;
    //delete file;

}
