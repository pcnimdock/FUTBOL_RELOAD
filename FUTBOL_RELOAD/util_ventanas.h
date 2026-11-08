#ifndef UTIL_VENTANAS_H
#define UTIL_VENTANAS_H

/*
 * Utilidades para que las ventanas quepan en pantallas pequeñas.
 *
 * Problema: con pantallas bajas (p. ej. 1360x654 en la máquina virtual)
 * varias ventanas pedían más altura de la que hay. GNOME no deja maximizar
 * ni redimensionar una ventana cuyo tamaño mínimo no cabe en pantalla.
 *
 * Solución: el contenido va dentro de un QScrollArea. La ventana puede
 * hacerse tan pequeña como se quiera (lo que no cabe se desplaza con las
 * barras) y se abre ajustada al espacio disponible.
 */

#include <QWidget>
#include <QMainWindow>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLayout>
#include <QScreen>
#include <QGuiApplication>
#include <QFrame>

namespace UtilVentanas {

inline QScrollArea *crearScroll(QWidget *contenido)
{
    QScrollArea *sa = new QScrollArea;
    sa->setObjectName("scrollVentana");
    sa->setWidgetResizable(true);
    sa->setFrameShape(QFrame::NoFrame);
    contenido->setObjectName(contenido->objectName().isEmpty() ? QString("contenidoVentana")
                                                               : contenido->objectName());
    sa->setWidget(contenido);
    // Fondo transparente para que se vea el de la ventana (style.qss)
    sa->setStyleSheet(QString("QScrollArea#scrollVentana { background: transparent; }"
                              "QScrollArea#scrollVentana > QWidget > QWidget#%1 { background: transparent; }")
                      .arg(contenido->objectName()));
    sa->viewport()->setAutoFillBackground(false);
    contenido->setAutoFillBackground(false);
    return sa;
}

// Tamaño inicial: el que pide el contenido, sin pasar del espacio disponible
inline void ajustarAPantalla(QWidget *ventana, const QSize &deseado)
{
    QScreen *pant = ventana->screen() ? ventana->screen() : QGuiApplication::primaryScreen();
    if (!pant) return;
    const QRect disp = pant->availableGeometry();
    // margen para la barra de título y los bordes del gestor de ventanas
    const int w = qMin(deseado.width() + 24, disp.width() - 16);
    const int h = qMin(deseado.height() + 24, disp.height() - 48);
    ventana->resize(qMax(w, 320), qMax(h, 240));
}

// Para diálogos: mete su layout principal dentro de un área desplazable.
// Llamar al FINAL del constructor, cuando el layout ya está montado.
inline void hacerDesplazable(QWidget *ventana)
{
    QLayout *l = ventana->layout();
    if (!l) return;
    QSize deseado = ventana->sizeHint().expandedTo(ventana->minimumSizeHint());
    if (ventana->testAttribute(Qt::WA_Resized))   // respeta un resize() previo
        deseado = deseado.expandedTo(ventana->size());

    QWidget *contenido = new QWidget;
    contenido->setLayout(l);          // Qt "roba" el layout y re-parenta los widgets
    QScrollArea *sa = crearScroll(contenido);

    QVBoxLayout *nuevo = new QVBoxLayout(ventana);
    nuevo->setContentsMargins(0, 0, 0, 0);
    nuevo->addWidget(sa);

    ventana->setMinimumSize(0, 0);
    ajustarAPantalla(ventana, deseado);
}

// Para la ventana principal: el widget central pasa a un área desplazable.
inline void hacerDesplazable(QMainWindow *ventana)
{
    QWidget *central = ventana->centralWidget();
    if (!central) return;
    const QSize deseado = ventana->sizeHint().expandedTo(ventana->minimumSizeHint());
    ventana->takeCentralWidget();
    ventana->setCentralWidget(crearScroll(central));
    ventana->setMinimumSize(0, 0);
    ajustarAPantalla(ventana, deseado);
}

} // namespace UtilVentanas

#endif // UTIL_VENTANAS_H
