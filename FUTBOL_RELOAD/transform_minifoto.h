#ifndef TRANSFORM_MINIFOTO_H
#define TRANSFORM_MINIFOTO_H
#include <QImage>
#include <QPixmap>
#include <qmath.h>

#define MINI_ESC 1
#define MINI_FOTO 2

class TRANSFORM_MINIFOTO
{
public:
    TRANSFORM_MINIFOTO();
    QImage foto2QImage(QByteArray minifoto_dfg,quint8 MINI_ESC_FOTO);
    QByteArray devolver_foto_reemplazo(QImage foto,quint8 MINI_ESC_FOTO);

    QImage miniesc2QImage(QByteArray miniesc_dfg);
    QByteArray devolver_miniesc_reemplazo(QImage foto);

    QImage minifoto2QImage(QByteArray miniesc_dfg);
    QByteArray devolver_minifoto_reemplazo(QImage foto);

    QImage fotosim2QImage(QByteArray escudo_sim);
    QByteArray devolver_fotosim_reemplazo(QImage escudo_sim);

    QChar descodificar_caracter(quint8 val);
    quint8 codificar_caracter(QChar val);

    QImage bigfoto2QImage(QByteArray bigfoto);
    QByteArray devolver_foto_bigfoto(QImage im);

    QVector<QRgb> colors; //iniciar paleta de colores
    QVector<QRgb> colors_bigfoto;
    QByteArray cabecera;
    QByteArray cabecera_miniesc;
    QByteArray cabecera_minifoto;
    QByteArray cabecera_escudo_sim;
    QByteArray cabecera_bigfoto;

    QByteArray cabecera_minifoto_standar;

};

#endif // TRANSFORM_MINIFOTO_H
