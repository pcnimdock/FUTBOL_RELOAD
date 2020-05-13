#include "dialog_jugador.h"
#include "ui_dialog_jugador.h"

Dialog_jugador::Dialog_jugador(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog_jugador)
{
    ui->setupUi(this);
}

Dialog_jugador::~Dialog_jugador()
{
    delete ui;
}

int Dialog_jugador::set_jugador(JUGADOR ju)
{
    jug=ju;


    ui->le_NombreCorto->setText(jug.NombreCorto);
    ui->le_NombreLargo->setText(jug.NombreLargo);
    ui->spin_NumCamiseta->setValue(jug.NumeroCamiseta);



    ui->cb_DemarcacinPref->setCurrentIndex(jug.DemarcacionPref);
    ui->cb_DemSec1->setCurrentIndex(jug.DemarcacionesSecundarias.at(0));
    ui->cb_DemSec2->setCurrentIndex(jug.DemarcacionesSecundarias.at(1));
    ui->cb_DemSec3->setCurrentIndex(jug.DemarcacionesSecundarias.at(2));
    ui->cb_DemSec4->setCurrentIndex(jug.DemarcacionesSecundarias.at(3));
    ui->cb_DemSec5->setCurrentIndex(jug.DemarcacionesSecundarias.at(4));

    ui->check_Extranjero->setChecked(jug.Extranjero?1:0);
    ui->cb_Piel->setCurrentIndex(jug.Piel);
    ui->cb_pelo->setCurrentIndex(jug.Pelo);
    ui->cb_pos_en_campo->setCurrentIndex(jug.PosicionEnCampo);

    if(!jug.FechaDeNacimiento.isEmpty())
    {
        ui->spin_dia->setValue(jug.FechaDeNacimiento.at(0));
        ui->spin_mes->setValue(jug.FechaDeNacimiento.at(1));
        ui->spin_anyo->setValue(jug.FechaDeNacimiento.at(2));
        ui->le_EquipoProcedencia->setText(jug.EquipoProcedencia);
        ui->le_Internacional->setText(jug.Internacional);
        ui->le_Altura->setText(QString::number((jug.Altura*0.01)));
        ui->le_Peso->setText(QString::number((jug.Peso)));
        ui->tedit_Comentario->setText(jug.Comentario);
        ui->le_Precio_Juego->setText(QString::number(jug.Precio));
        ui->le_LugarDeNacimiento->setText(jug.LugarDeNacimiento);
        ui->cb_Estado_fichaje->setCurrentIndex(jug.EstadoFichaje);
    }
    else
    {
        ui->spin_dia->setEnabled(false);
        ui->spin_mes->setEnabled(false);
        ui->spin_anyo->setEnabled(false);
        ui->le_EquipoProcedencia->setEnabled(false);
        ui->le_Internacional->setEnabled(false);
        ui->le_Altura->setEnabled(false);
        ui->le_Peso->setEnabled(false);
        ui->tedit_Comentario->setEnabled(false);
        ui->le_Precio_Juego->setEnabled(false);
        ui->le_LugarDeNacimiento->setEnabled(false);
        ui->cb_Estado_fichaje->setEnabled(false);

    }


    ui->spin_Velocidad->setValue(jug.Velocidad);
    ui->spin_Regate->setValue(jug.Regate);
    ui->spin_Agilidad->setValue(jug.Agilidad);
    ui->spin_Agresividad->setValue(jug.Agresividad);
    ui->check_Extranjero->setChecked(jug.Extranjero?1:0);
    ui->check_Tecnica->setChecked(jug.ActTecnica?1:0);
    ui->check_Remate->setChecked(jug.ActRemate?1:0);
    ui->check_Pase->setChecked(jug.ActPase?1:0);
    ui->check_tiro->setChecked(jug.ActTiro?1:0);
    if(!jug.minifoto.isEmpty())
    {
        ui->btn_MiniFoto->setIconMinifoto(trans.minifoto2QImage(jug.minifoto));
    }
    else
    {
        ui->btn_MiniFoto->setText("MiniFoto");
    }
    if(!jug.bigfoto.isEmpty())
    {
        ui->btn_BigFoto->setIconBigfoto(trans.bigfoto2QImage(jug.bigfoto));
    }
    else
    {
        ui->btn_BigFoto->setText("BigFoto");
    }
    return 1;

}

JUGADOR Dialog_jugador::get_jugador()
{

    return jug;

}

void Dialog_jugador::on_btn_guardar_clicked()
{
    //guardar todos los campos
    jug.NombreCorto=ui->le_NombreCorto->text();
    jug.NombreLargo=ui->le_NombreLargo->text();
    jug.NumeroCamiseta=ui->spin_NumCamiseta->value();

    jug.DemarcacionPref=ui->cb_DemarcacinPref->currentIndex();
    jug.DemarcacionesSecundarias[0]=ui->cb_DemSec1->currentIndex();
    jug.DemarcacionesSecundarias[1]=ui->cb_DemSec2->currentIndex();
    jug.DemarcacionesSecundarias[2]=ui->cb_DemSec3->currentIndex();
    jug.DemarcacionesSecundarias[3]=ui->cb_DemSec4->currentIndex();
    jug.DemarcacionesSecundarias[4]=ui->cb_DemSec5->currentIndex();

    jug.Extranjero=ui->check_Extranjero->isChecked();
    jug.Piel=ui->cb_Piel->currentIndex();
    jug.Pelo=ui->cb_pelo->currentIndex();
    jug.PosicionEnCampo=ui->cb_pos_en_campo->currentIndex();

    if(!jug.FechaDeNacimiento.isEmpty())
    {
        jug.FechaDeNacimiento[0]=ui->spin_dia->value();
        jug.FechaDeNacimiento[1]=ui->spin_mes->value();
        jug.FechaDeNacimiento[2]=ui->spin_anyo->value();
        jug.EquipoProcedencia=ui->le_EquipoProcedencia->text();
        jug.Internacional=ui->le_Internacional->text();
        jug.Altura= ui->le_Altura->text().toFloat()*100;
        jug.Peso = ui->le_Peso->text().toInt();
        jug.Comentario = ui->tedit_Comentario->toPlainText();
        jug.Precio = ui->le_Precio_Juego->text().toUInt();
        jug.LugarDeNacimiento = ui->le_LugarDeNacimiento->text();
        jug.EstadoFichaje = ui->cb_Estado_fichaje->currentIndex();
    }
    else
    {
        ui->spin_dia->setEnabled(false);
        ui->spin_mes->setEnabled(false);
        ui->spin_anyo->setEnabled(false);
        ui->le_EquipoProcedencia->setEnabled(false);
        ui->le_Internacional->setEnabled(false);
        ui->le_Altura->setEnabled(false);
        ui->le_Peso->setEnabled(false);
        ui->tedit_Comentario->setEnabled(false);
        ui->le_Precio_Juego->setEnabled(false);
        ui->le_LugarDeNacimiento->setEnabled(false);
        ui->cb_Estado_fichaje->setEnabled(false);

    }


    jug.Velocidad = ui->spin_Velocidad->value();
    jug.Regate = ui->spin_Regate->value();
    jug.Agilidad = ui->spin_Agilidad->value();
    jug.Agresividad = ui->spin_Agresividad->value();
    jug.Extranjero = ui->check_Extranjero->isChecked();
    jug.ActTecnica = ui->check_Tecnica->isChecked();
    jug.ActRemate = ui->check_Remate->isChecked();
    jug.ActPase = ui->check_Pase->isChecked();
    jug.ActTiro = ui->check_tiro->isChecked();
    if(ui->btn_MiniFoto->isIcon())
    {
        //ui->btn_MiniFoto->setIconMinifoto(trans.minifoto2QImage(jug.minifoto));
        QImage im;
        im = ui->btn_MiniFoto->getIconEQ();
        jug.minifoto = trans.devolver_minifoto_reemplazo(im);
    }

    if(ui->btn_BigFoto->isIcon())
    {
        QImage im;
        im = ui->btn_BigFoto->getIconEQ();
        jug.bigfoto = trans.devolver_foto_bigfoto(im);
    }
}

void Dialog_jugador::on_btn_BigFoto_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,tr("Archivo sustitución de BigFoto"),"./" ,tr("Imagenes (*.bmp *.BMP)"));
    QImage im = QImage(fileName);
    ui->btn_BigFoto->setIconBigfoto(im);
}

void Dialog_jugador::on_btn_MiniFoto_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,tr("Archivo sustitución de MiniFoto"),"./" ,tr("Imagenes (*.bmp *.BMP)"));
    QImage im = QImage(fileName);
    ui->btn_MiniFoto->setIconMinifoto(im);
}
