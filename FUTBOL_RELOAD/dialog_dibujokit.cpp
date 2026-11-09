#include "dialog_dibujokit.h"
#include "util_ventanas.h"

#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QColorDialog>
#include <QPixmap>
#include <QIcon>
#include <QPainter>

static const int ESCALA = 6;

DialogDibujoKit::DialogDibujoKit(const QString &carpetaJuego, const QString &id,
                                 const QString &nombreEquipo, QWidget *parent)
    : QDialog(parent), m_carpeta(carpetaJuego), m_id(id)
{
    setWindowTitle("Dibujo en la ficha del equipo — " + nombreEquipo);
    setModal(true);

    QVBoxLayout *root = new QVBoxLayout(this);
    QLabel *ayuda = new QLabel(
        "Es el dibujo de camiseta y pantalón que sale en la Base de datos, en la ficha del "
        "equipo, junto al botón «Plantilla» (fichero DBDAT/CAMISAS). Sólo tiene 16 colores: "
        "los de la paleta de la ficha. Puedes sustituir un color por otro, o exportarlo a PNG, "
        "retocarlo con cualquier programa de dibujo e importarlo.", this);
    ayuda->setWordWrap(true);
    root->addWidget(ayuda);

    m_lblImagen = new QLabel(this);
    m_lblImagen->setAlignment(Qt::AlignCenter);
    m_lblImagen->setStyleSheet("background:#a3a3a3; padding:8px;");
    root->addWidget(m_lblImagen);

    m_lblOrigen = new QLabel(this);
    m_lblOrigen->setWordWrap(true);
    m_lblOrigen->setStyleSheet("color:#9fb0b8;");
    root->addWidget(m_lblOrigen);

    // --- Paleta: 16 colores, los de equipo (1, 2, 8) se pueden cambiar ------
    QGroupBox *gbPal = new QGroupBox("Paleta de la ficha (pulsa un color de equipo para cambiarlo)", this);
    QGridLayout *gl = new QGridLayout(gbPal);
    for (int i = 0; i < 16; i++)
    {
        QPushButton *b = new QPushButton(this);
        b->setMinimumWidth(110);
        b->setIconSize(QSize(22, 16));
        connect(b, &QPushButton::clicked, this, [this, i]() { elegirColorEquipo(i); });
        m_botonesColor.append(b);
        gl->addWidget(b, i / 4, i % 4);
    }
    QLabel *aviso = new QLabel(
        "⚠ Los colores de equipo (1, 2 y 8) son los de TODA la ficha: cambiarlos cambia también "
        "el escudo grande y las cintas. Se guardan en la foto del estadio (BIGCAMP) y en el "
        "escudo grande (BIGESC).", this);
    aviso->setWordWrap(true);
    aviso->setStyleSheet("color:#e0b34a;");
    gl->addWidget(aviso, 4, 0, 1, 4);
    root->addWidget(gbPal);

    // --- Sustituir color -----------------------------------------------------
    QHBoxLayout *fs = new QHBoxLayout();
    m_cbDe = new QComboBox(this);
    m_cbA = new QComboBox(this);
    QPushButton *btnSust = new QPushButton("Sustituir", this);
    fs->addWidget(new QLabel("Sustituir el color", this));
    fs->addWidget(m_cbDe);
    fs->addWidget(new QLabel("por", this));
    fs->addWidget(m_cbA);
    fs->addWidget(btnSust);
    fs->addStretch();
    root->addLayout(fs);

    // --- Botones -------------------------------------------------------------
    QHBoxLayout *fb = new QHBoxLayout();
    QPushButton *btnExp = new QPushButton("Exportar PNG…", this);
    QPushButton *btnImp = new QPushButton("Importar PNG…", this);
    QPushButton *btnDesc = new QPushButton("Descartar cambios", this);
    QPushButton *btnGuardar = new QPushButton("Guardar en el juego", this);
    QPushButton *btnCerrar = new QPushButton("Cerrar", this);
    fb->addWidget(btnExp);
    fb->addWidget(btnImp);
    fb->addWidget(btnDesc);
    fb->addStretch();
    fb->addWidget(btnGuardar);
    fb->addWidget(btnCerrar);
    root->addLayout(fb);

    connect(btnSust, &QPushButton::clicked, this, &DialogDibujoKit::sustituir);
    connect(btnExp, &QPushButton::clicked, this, &DialogDibujoKit::exportarPng);
    connect(btnImp, &QPushButton::clicked, this, &DialogDibujoKit::importarPng);
    connect(btnDesc, &QPushButton::clicked, this, &DialogDibujoKit::descartar);
    connect(btnGuardar, &QPushButton::clicked, this, &DialogDibujoKit::guardar);
    connect(btnCerrar, &QPushButton::clicked, this, &QDialog::close);

    QString error;
    if (!m_kit.cargar(m_carpeta, m_id, &error))
    {
        m_lblImagen->setText(error);
        for (QWidget *w : std::initializer_list<QWidget *>{btnSust, btnExp, btnImp, btnDesc, btnGuardar})
            w->setEnabled(false);
    }
    refrescar();
    UtilVentanas::hacerDesplazable(this);
}

static QIcon muestra(QRgb c)
{
    QPixmap pm(22, 16);
    pm.fill(QColor(c));
    QPainter p(&pm);
    p.setPen(Qt::gray);
    p.drawRect(0, 0, 21, 15);
    p.end();
    // Mismo aspecto aunque el botón esté desactivado (colores fijos): si no,
    // Qt agrisa la muestra y el negro o el rojo parecen grises.
    QIcon icono(pm);
    icono.addPixmap(pm, QIcon::Disabled);
    return icono;
}

void DialogDibujoKit::refrescar()
{
    if (!m_kit.cargado())
    {
        for (QPushButton *b : m_botonesColor) b->setEnabled(false);
        return;
    }
    m_lblImagen->setPixmap(QPixmap::fromImage(m_kit.imagen(ESCALA)));
    m_lblOrigen->setText("Paleta tomada de " + m_kit.origenPaleta() +
                         (m_kit.modificado() ? "   ·   <b>cambios sin guardar</b>" : ""));

    const QVector<QRgb> pal = m_kit.paleta();
    const QVector<int> n = m_kit.recuento();
    const int deSel = m_cbDe->currentIndex(), aSel = m_cbA->currentIndex();
    m_cbDe->clear();
    m_cbA->clear();
    for (int i = 0; i < 16; i++)
    {
        QString txt = QString("%1").arg(i);
        if (PCFutbolDibujoKit::esColorDeEquipo(i)) txt += " equipo";
        if (i >= 9 && i <= 11) txt += " (foto)";
        txt += QString(" · %1 px").arg(n.at(i));
        m_botonesColor[i]->setIcon(muestra(pal.at(i)));
        m_botonesColor[i]->setText(txt);
        m_botonesColor[i]->setEnabled(PCFutbolDibujoKit::esColorDeEquipo(i));
        m_botonesColor[i]->setToolTip(PCFutbolDibujoKit::esColorDeEquipo(i)
            ? "Color propio del equipo: pulsa para cambiarlo"
            : "Color fijo del interfaz del juego (igual en todos los equipos)");
        m_cbDe->addItem(muestra(pal.at(i)), QString("%1 (%2 px)").arg(i).arg(n.at(i)));
        m_cbA->addItem(muestra(pal.at(i)), QString::number(i));
    }
    if (deSel >= 0) m_cbDe->setCurrentIndex(deSel);
    if (aSel >= 0) m_cbA->setCurrentIndex(aSel);
}

void DialogDibujoKit::sustituir()
{
    const int de = m_cbDe->currentIndex(), a = m_cbA->currentIndex();
    if (de < 0 || a < 0 || de == a) return;
    m_kit.sustituir(de, a);
    refrescar();
}

void DialogDibujoKit::exportarPng()
{
    const QString ruta = QFileDialog::getSaveFileName(this, "Exportar dibujo",
                                                      "EQ" + m_id + ".png", "PNG (*.png)");
    if (ruta.isEmpty()) return;
    if (!m_kit.imagen(1).save(ruta, "PNG"))
        QMessageBox::critical(this, "Error", "No se pudo guardar " + ruta);
}

void DialogDibujoKit::importarPng()
{
    const QString ruta = QFileDialog::getOpenFileName(this, "Importar dibujo", QString(),
                                                      "Imágenes (*.png *.bmp *.gif)");
    if (ruta.isEmpty()) return;
    QImage img(ruta);
    if (img.isNull())
    {
        QMessageBox::critical(this, "Error", "No se pudo leer " + ruta);
        return;
    }
    if (img.width() != m_kit.ancho() || img.height() != m_kit.alto())
    {
        if (QMessageBox::question(this, "Tamaño distinto",
                QString("La imagen mide %1×%2 y el dibujo %3×%4. Se escalará. ¿Continuar?")
                    .arg(img.width()).arg(img.height()).arg(m_kit.ancho()).arg(m_kit.alto()),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
            return;
    }
    m_kit.importar(img);
    refrescar();
}

void DialogDibujoKit::descartar()
{
    QString error;
    m_kit.cargar(m_carpeta, m_id, &error);
    refrescar();
}

void DialogDibujoKit::elegirColorEquipo(int idx)
{
    if (!PCFutbolDibujoKit::esColorDeEquipo(idx)) return;
    const QColor c = QColorDialog::getColor(QColor(m_kit.paleta().at(idx)), this,
                                            QString("Color de equipo %1").arg(idx));
    if (!c.isValid()) return;
    m_kit.setColorEquipo(idx, c);
    refrescar();
}

void DialogDibujoKit::guardar()
{
    if (!m_kit.modificado())
    {
        QMessageBox::information(this, "Sin cambios", "No hay cambios que guardar.");
        return;
    }
    QString error;
    QStringList informe;
    if (!m_kit.guardar(&error, &informe))
    {
        QMessageBox::critical(this, "Error", error);
        return;
    }
    refrescar();
    QMessageBox::information(this, "Guardado",
                             informe.join("\n") + "\n\n(La primera vez se guarda una copia .bak.)");
}
