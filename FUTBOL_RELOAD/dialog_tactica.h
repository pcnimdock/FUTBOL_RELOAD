#ifndef DIALOG_TACTICA_H
#define DIALOG_TACTICA_H

/*
 * Ventana «Táctica»: la táctica con la que empieza el equipo en una liga
 * nueva (ver pcfutbol_tactica.h). Campo de 15 × 9 como el del juego:
 * círculo = posición al defender, flecha = al atacar, recuadro = zona del
 * jugador seleccionado. Todo se arrastra con el ratón.
 */

#include <QDialog>
#include <QWidget>
#include "pcfutbol_tactica.h"

class QListWidget;
class QLabel;
class QCheckBox;

class CampoTactica : public QWidget
{
    Q_OBJECT
public:
    explicit CampoTactica(QWidget *parent = nullptr);
    void setTactica(const PCFutbolTactica::Tactica &t);
    PCFutbolTactica::Tactica tactica() const { return m_t; }
    void setSeleccionado(int k);
    int seleccionado() const { return m_sel; }
    void setVerZonas(bool v) { m_todas = v; update(); }
    QSize sizeHint() const override { return QSize(600, 360); }
    QSize minimumSizeHint() const override { return QSize(300, 180); }

signals:
    void cambiado();
    void seleccion(int k);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    enum Arrastre { Nada, Defensa, Ataque, Esquina, MoverZona };
    QRectF campo() const;
    double celda() const;
    QPointF centro(int x, int y) const;
    QPoint celdaEn(const QPointF &p) const;
    QRectF rectZona(const PCFutbolTactica::Jugador &j) const;

    PCFutbolTactica::Tactica m_t;
    int m_sel = 0;
    bool m_todas = false;
    Arrastre m_arr = Nada;
    int m_esquina = 0;              // 0 arriba-izq, 1 arriba-der, 2 abajo-izq, 3 abajo-der
    QPoint m_inicio;                // celda donde empezó a mover la zona
    PCFutbolTactica::Jugador m_antes;
};

class DialogTactica : public QDialog
{
    Q_OBJECT
public:
    // nombres: los 11 titulares en orden de alineación (para la lista)
    DialogTactica(const PCFutbolTactica::Tactica &t, const QStringList &nombres,
                  const QString &equipo, const QString &carpetaJuego, QWidget *parent = nullptr);
    PCFutbolTactica::Tactica tactica() const;

private:
    void actualizarInfo();
    void cargarDelJuego();
    void grabarEnElJuego();

    CampoTactica *m_campo = nullptr;
    QListWidget *m_lista = nullptr;
    QLabel *m_info = nullptr;
    PCFutbolTactica::Tactica m_original;
    QString m_equipo, m_carpetaJuego;
};

#endif // DIALOG_TACTICA_H
