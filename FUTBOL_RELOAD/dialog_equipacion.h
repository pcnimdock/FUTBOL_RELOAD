#ifndef DIALOG_EQUIPACION_H
#define DIALOG_EQUIPACION_H

/*
 * Editor de equipación (camiseta) — modo "paint".
 *
 * Mantiene la misma API pública que la versión anterior (setRutaEquiposDat,
 * setNumeroEquipo, mostrarTexturaActual) para no romper dialogequipos.cpp,
 * pero por dentro ahora es un editor de píxeles:
 *
 *   - Lienzo con el sprite REAL de 20x20 (mapa calibrado contra el juego).
 *     Clic izq = pintar · clic der = cuentagotas · arrastrar = pintar seguido.
 *   - Paleta: familias de color (rampas) o los 256 colores reales del juego.
 *   - Patrones predefinidos: liso / rayas verticales / franjas horizontales.
 *   - Relleno por zonas: cuello, ribete, mangas, hombros, torso, faldón.
 *   - Deshacer (Ctrl+Z), restaurar original y guardado con copia .bak.
 */

#include <QDialog>
#include <QWidget>
#include <QByteArray>
#include <QList>
#include <QStringList>
#include <QVector>
#include "pcfutbol_equipacion.h"

class QLineEdit;
class QSpinBox;
class QComboBox;
class QLabel;
class QCheckBox;

// ---------------------------------------------------------------------------
// Lienzo de pintura: el sprite de camiseta de 20x20, ampliado
// ---------------------------------------------------------------------------
class LienzoCamiseta : public QWidget
{
    Q_OBJECT
public:
    explicit LienzoCamiseta(QWidget *parent = nullptr);

    void setTextura(const QByteArray &textura168);
    QByteArray textura() const { return m_textura; }

    void setColorActual(int idxPaleta) { m_colorActual = idxPaleta; }
    void setProtegerSombra(bool v) { m_protegerSombra = v; }
    void setMostrarRejilla(bool v) { m_rejilla = v; update(); }
    void setZoom(int z);

    QSize sizeHint() const override;

signals:
    void texturaCambiada();
    void trazoIniciado();            // antes de pintar (para el historial de deshacer)
    void colorCogido(int idxPaleta); // cuentagotas
    void pixelSobrevolado(int sx, int sy, int posTextura);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void leaveEvent(QEvent *) override;

private:
    bool pixelDesdeRaton(const QPoint &p, int *sx, int *sy) const;

    QByteArray m_textura;
    int  m_colorActual = 0x80;
    int  m_zoom = 16;
    bool m_protegerSombra = true;
    bool m_rejilla = true;
    int  m_hoverX = -1, m_hoverY = -1;
};

// ---------------------------------------------------------------------------
// Paleta de colores del juego
// ---------------------------------------------------------------------------
class PaletaCamiseta : public QWidget
{
    Q_OBJECT
public:
    explicit PaletaCamiseta(QWidget *parent = nullptr);

    void setModoCompleto(bool v);
    int colorSeleccionado() const { return m_seleccionado; }
    void seleccionar(int idxPaleta);

    QSize sizeHint() const override;

signals:
    void colorElegido(int idxPaleta);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;

private:
    int celdaEn(const QPoint &p) const;

    bool m_completo = false;
    int  m_seleccionado = 0x80;
    QVector<QVector<int> > m_filas;
    QStringList m_nombres;
};

// ---------------------------------------------------------------------------
// Diálogo
// ---------------------------------------------------------------------------
class DialogEquipacion : public QDialog
{
    Q_OBJECT
public:
    explicit DialogEquipacion(QWidget *parent = nullptr);

    // --- API que ya usaba dialogequipos.cpp (sin cambios) ---
    void setRutaEquiposDat(const QString &ruta);
    void setNumeroEquipo(int n);
    void mostrarTexturaActual(bool primera, const QByteArray &textura168);

    // --- Nuevo: recuperar lo editado para refrescar el EQUIPO en memoria ---
    QByteArray texturaEditada(bool primera) const;
    bool seHaGuardado() const { return m_guardado; }

private slots:
    void cambiarEquipacion(int idx);
    void aplicarPatron();
    void rellenarZona();
    void deshacer();
    void restaurarOriginal();
    void leerDelFichero();
    void buscarRutaEquiposDat();
    void guardarEnDisco();
    void onTrazoIniciado();
    void onTexturaCambiada();
    void onPixelSobrevolado(int sx, int sy, int pos);
    void onColorElegido(int idxPaleta);

private:
    void empujarUndo();
    void refrescar();
    QString nombreZona(int posTextura) const;

    QByteArray m_tex[2];      // 0 = 1ª equipación, 1 = 2ª
    QByteArray m_texOrig[2];
    int m_actual = 0;
    bool m_guardado = false;
    QList<QByteArray> m_undo[2];
    QList<PCFutbolEquipacion::ColorEquipacion> colores;

    LienzoCamiseta *m_lienzo;
    PaletaCamiseta *m_paleta;
    QLineEdit *le_ruta;
    QSpinBox  *sp_numero;
    QLabel    *m_lblColor;
    QLabel    *m_lblInfo;
    QComboBox *m_cbEquipacion;
    QComboBox *m_cbPatron;
    QComboBox *m_cbColorA;
    QComboBox *m_cbColorB;
    QComboBox *m_cbZona;
    QSpinBox  *m_spAncho;
    QSpinBox  *m_spZoom;
    QCheckBox *m_chkSombra;
    QCheckBox *m_chkRejilla;
    QCheckBox *m_chkPaletaCompleta;
};

#endif // DIALOG_EQUIPACION_H
