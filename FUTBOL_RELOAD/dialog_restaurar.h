#ifndef DIALOG_RESTAURAR_H
#define DIALOG_RESTAURAR_H

/*
 * Ventana «Restaurar originales»: lista todas las copias .bak que han dejado
 * las partes del editor que escriben directamente en el juego (equipaciones,
 * dibujo de la ficha, composición de ligas, sonidos) y permite volver al
 * original. Las plantillas (botón Guardar) no hacen falta: se escriben en
 * una carpeta aparte (FUTBOL45_NEW / FUTBOL4_NEW).
 */

#include <QDialog>
#include <QStringList>

class QTreeWidget;
class QLabel;

class DialogRestaurar : public QDialog
{
    Q_OBJECT
public:
    explicit DialogRestaurar(const QString &carpetaJuego, QWidget *parent = nullptr);

    // Copias .bak encontradas bajo la carpeta (ruta del .bak)
    static QStringList buscarCopias(const QString &carpetaJuego);

private:
    void rellenar();
    void restaurar();

    QString m_carpeta;
    QTreeWidget *m_arbol = nullptr;
    QLabel *m_resumen = nullptr;
};

#endif // DIALOG_RESTAURAR_H
