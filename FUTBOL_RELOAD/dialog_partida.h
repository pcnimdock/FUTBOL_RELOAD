#ifndef DIALOG_PARTIDA_H
#define DIALOG_PARTIDA_H

/*
 * Ventana «Partida guardada»: editar una partida de Liga Manager en curso
 * (TACTICS/LIGA.n): caja y entrenamiento de cada equipo, su táctica, y las
 * medias, energía y valor de cada jugador. Ver pcfutbol_partida.h.
 */

#include <QDialog>
#include "pcfutbol_partida.h"
#include "equipo.h"

class QListWidget;
class QTableWidget;
class QLabel;
class QDoubleSpinBox;
class QSpinBox;
class QLineEdit;

class DialogPartida : public QDialog
{
    Q_OBJECT
public:
    // bd: base de datos cargada en el editor (para copiar jugadores); puede ser nullptr
    DialogPartida(const QString &ruta, const PCFutbolPartida &partida,
                  const QString &carpetaJuego, QWidget *parent = nullptr,
                  const QList<EQUIPO> *bd = nullptr);

private:
    void llenarEquipos();
    void mostrarEquipo(int i);
    void recogerTabla();
    void editarTactica();
    int jugadorSeleccionado() const;      // índice en equipos()[m_actual].jugadores, o -1
    void renombrar();
    void traspasar();
    void quitarBajas(bool todos);
    void anadirJugador();
    void guardar();
    void reject() override;

    QString m_ruta, m_carpetaJuego;
    const QList<EQUIPO> *m_bd = nullptr;
    PCFutbolPartida m_p;
    int m_actual = -1;
    bool m_cargando = false;

    QLineEdit *m_filtro = nullptr;
    QListWidget *m_lista = nullptr;
    QLabel *m_titulo = nullptr;
    QDoubleSpinBox *m_caja = nullptr;
    QDoubleSpinBox *m_entrada = nullptr, *m_valla = nullptr, *m_prima1 = nullptr, *m_prima2 = nullptr;
    QLabel *m_lPrima1 = nullptr, *m_lPrima2 = nullptr;
    QSpinBox *m_entr[3] = {nullptr, nullptr, nullptr};
    QTableWidget *m_tabla = nullptr;
};

#endif // DIALOG_PARTIDA_H
