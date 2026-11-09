#ifndef DIALOG_DIBUJOKIT_H
#define DIALOG_DIBUJOKIT_H

/*
 * Ventana «Dibujo en la ficha del equipo»: ver y editar
 * DBDAT/CAMISAS/EQ95xxxx.DFG con los colores reales del juego, y cambiar
 * los colores propios del equipo (1, 2 y 8) de la ficha. Ver
 * pcfutbol_dibujokit.h para el formato y de dónde sale la paleta.
 */

#include <QDialog>
#include <QVector>
#include "pcfutbol_dibujokit.h"

class QLabel;
class QComboBox;
class QPushButton;

class DialogDibujoKit : public QDialog
{
    Q_OBJECT
public:
    DialogDibujoKit(const QString &carpetaJuego, const QString &id,
                    const QString &nombreEquipo, QWidget *parent = nullptr);
    bool cargado() const { return m_kit.cargado(); }

private slots:
    void sustituir();
    void exportarPng();
    void importarPng();
    void descartar();
    void guardar();
    void elegirColorEquipo(int idx);

private:
    void refrescar();

    QString m_carpeta, m_id;
    PCFutbolDibujoKit m_kit;
    QLabel *m_lblImagen = nullptr;
    QLabel *m_lblOrigen = nullptr;
    QVector<QPushButton *> m_botonesColor;
    QComboBox *m_cbDe = nullptr;
    QComboBox *m_cbA = nullptr;
};

#endif // DIALOG_DIBUJOKIT_H
