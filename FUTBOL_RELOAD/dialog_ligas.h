#ifndef DIALOG_LIGAS_H
#define DIALOG_LIGAS_H

/*
 * Diálogo "Composición de ligas": mueve equipos entre Primera, Segunda y
 * Segunda B intercambiándolos (el número de equipos de cada división es
 * fijo en el ejecutable: 22 y 20). Ver pcfutbol_ligas.h.
 */

#include <QDialog>
#include "pcfutbol_ligas.h"

class QListWidget;
class QListWidgetItem;
class QLabel;
class QPushButton;

class DialogLigas : public QDialog
{
    Q_OBJECT
public:
    explicit DialogLigas(const QString &carpetaJuego, QWidget *parent = nullptr);
    bool cargadoOk() const { return m_ok; }

private slots:
    void onSeleccion(QListWidgetItem *item);
    void intercambiar();
    void deshacerTodo();
    void guardar();

private:
    void rellenar();
    QString textoEquipo(quint32 id, int pos) const;

    PCFutbolLigas m_ligas;
    bool m_ok = false;
    QListWidget *m_listas[3];
    QLabel *m_lblSeleccion;
    QLabel *m_lblAvisos;
    QPushButton *m_btnIntercambiar;
    QPushButton *m_btnGuardar;
    // Últimos dos equipos elegidos (de listas distintas): {lista, id}
    QList<QPair<int, quint32>> m_elegidos;
};

#endif // DIALOG_LIGAS_H
