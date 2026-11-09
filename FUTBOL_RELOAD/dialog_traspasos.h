#ifndef DIALOG_TRASPASOS_H
#define DIALOG_TRASPASOS_H

/*
 * Ventana «Traspasos»: dos plantillas lado a lado para mover jugadores de
 * un equipo a otro (conservan foto y datos). Trabaja sobre una copia: los
 * cambios sólo pasan al editor al pulsar «Aceptar», y al juego al Guardar.
 */

#include <QDialog>
#include <QList>
#include "equipo.h"

class PCFutbolTemporadaReal;

class QComboBox;
class QListWidget;
class QLabel;
class QCheckBox;

class DialogTraspasos : public QDialog
{
    Q_OBJECT
public:
    // real: jugadores de la temporada real (se traspasan dejando copia); puede ser nullptr
    DialogTraspasos(const QList<EQUIPO> &equipos, int equipoInicial = 0,
                    const PCFutbolTemporadaReal *real = nullptr, QWidget *parent = nullptr);
    QList<EQUIPO> equipos() const { return m_equipos; }
    int numeroTraspasos() const { return m_registro.size(); }

private:
    void llenarLista(int lado);
    void mover(int desde);
    QComboBox *crearCombo(int inicial);

    QList<EQUIPO> m_equipos;
    QComboBox *m_combo[2] = {nullptr, nullptr};
    QListWidget *m_lista[2] = {nullptr, nullptr};
    QLabel *m_info[2] = {nullptr, nullptr};
    QCheckBox *m_marcar = nullptr;
    QListWidget *m_log = nullptr;
    QStringList m_registro;
    const PCFutbolTemporadaReal *m_real = nullptr;
};

#endif // DIALOG_TRASPASOS_H
