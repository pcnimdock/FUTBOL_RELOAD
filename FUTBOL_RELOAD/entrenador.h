#ifndef ENTRENADOR_H
#define ENTRENADOR_H
#include <QByteArray>
#include <QString>

class ENTRENADOR
{
public:
    ENTRENADOR();
    quint32 EquipoIdDBC;
    quint16 puntero;
    QString NombreCorto;
    QString NombreLargo;
    QString Info;
    QString Declaraciones;
    QByteArray minifoto;
    QByteArray bigfoto;

    void clear();
};

#endif // ENTRENADOR_H
