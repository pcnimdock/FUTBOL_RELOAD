#include "dialog_ligas.h"
#include "util_ventanas.h"

#include <QListWidget>
#include <QPalette>
#include <QColor>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QSet>
#include <QComboBox>
#include <QGridLayout>
#include <QSignalBlocker>

DialogLigas::DialogLigas(const QString &carpetaJuego, QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Composición de ligas");
    // Botones de maximizar/minimizar: en Linux los QDialog no los traen
    setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint);
    setModal(true);
    resize(980, 640);

    QLabel *lblAyuda = new QLabel(
        "Selecciona un equipo en una división y otro en otra, y pulsa «Intercambiar». "
        "El número de equipos es fijo (22 en Primera, 20 en Segunda): sólo se puede "
        "cambiar QUIÉN juega en cada una. El orden de Primera y Segunda es la "
        "clasificación de la temporada anterior.\n"
        "Se modifican PCMANAG.DAT y PROMANAG.DAT (con copia .bak): afecta a las ligas "
        "(Arcade, Entrenador, Manager, Promanager) y al amistoso, sólo en partidas NUEVAS. "
        "La base de datos y el seguimiento de la liga real 95/96 (PCFUTBOL.DAT) no "
        "cambian: van ligados al calendario real y el juego falla si se tocan.", this);
    lblAyuda->setWordWrap(true);

    const char *titulos[3] = {"Primera División (22)", "Segunda División (20)", "Segunda B"};
    QHBoxLayout *cols = new QHBoxLayout();
    for (int i = 0; i < 3; i++)
    {
        QGroupBox *gb = new QGroupBox(QString::fromUtf8(titulos[i]), this);
        QVBoxLayout *l = new QVBoxLayout(gb);
        m_listas[i] = new QListWidget(this);
        m_listas[i]->setSelectionMode(QAbstractItemView::SingleSelection);
        // Colores fijados también por código: el QSS no siempre llega al
        // viewport interno de las listas (mismo problema que las tablas de
        // la ventana principal) y quedaban texto gris claro sobre blanco.
        {
            QPalette pal = m_listas[i]->palette();
            pal.setColor(QPalette::Base, QColor("#202325"));
            pal.setColor(QPalette::AlternateBase, QColor("#26292b"));
            pal.setColor(QPalette::Text, QColor("#eaeaea"));
            pal.setColor(QPalette::WindowText, QColor("#eaeaea"));
            pal.setColor(QPalette::Highlight, QColor("#86aa1c"));
            pal.setColor(QPalette::HighlightedText, QColor("#16210a"));
            m_listas[i]->setPalette(pal);
        }
        l->addWidget(m_listas[i]);
        cols->addWidget(gb);
        connect(m_listas[i], &QListWidget::itemClicked, this, &DialogLigas::onSeleccion);
    }

    m_lblSeleccion = new QLabel("Elige dos equipos de divisiones distintas.", this);
    m_lblAvisos = new QLabel(this);
    m_lblAvisos->setWordWrap(true);
    m_lblAvisos->setStyleSheet("color:#e0b34a;");

    m_btnIntercambiar = new QPushButton("Intercambiar", this);
    QPushButton *btnDeshacer = new QPushButton("Deshacer todo", this);
    m_btnGuardar = new QPushButton("Guardar en el juego", this);
    QPushButton *btnCerrar = new QPushButton("Cerrar", this);
    m_btnIntercambiar->setEnabled(false);

    QHBoxLayout *botones = new QHBoxLayout();
    botones->addWidget(m_btnIntercambiar);
    botones->addWidget(btnDeshacer);
    botones->addStretch();
    botones->addWidget(m_btnGuardar);
    botones->addWidget(btnCerrar);

    QVBoxLayout *root = new QVBoxLayout(this);
    root->addWidget(lblAyuda);
    root->addLayout(cols, 1);
    m_gbEuropa = new QGroupBox("Clasificados para Europa en la 1ª temporada de una partida nueva", this);
    m_gbEuropa->setToolTip("Al acabar cada temporada el juego los recalcula con la clasificación real.");
    root->addWidget(m_gbEuropa);
    root->addWidget(m_lblSeleccion);
    root->addWidget(m_lblAvisos);
    root->addLayout(botones);

    connect(m_btnIntercambiar, &QPushButton::clicked, this, &DialogLigas::intercambiar);
    connect(btnDeshacer, &QPushButton::clicked, this, &DialogLigas::deshacerTodo);
    connect(m_btnGuardar, &QPushButton::clicked, this, &DialogLigas::guardar);
    connect(btnCerrar, &QPushButton::clicked, this, &QDialog::close);

    QString error;
    m_ok = m_ligas.cargar(carpetaJuego, &error);
    if (!m_ok)
    {
        m_lblAvisos->setText(error);
        m_btnGuardar->setEnabled(false);
        UtilVentanas::hacerDesplazable(this);
        return;
    }
    construirEuropa();
    rellenar();

    // Una versión anterior del editor también modificaba PCFUTBOL.DAT, y eso
    // hace fallar el juego ("Equipo no encontrado"). Ofrecer restaurarlo.
    if (m_ligas.pcfutbolModificadoAntes())
    {
        if (QMessageBox::warning(this, "PCFUTBOL.DAT modificado",
                "PCFUTBOL.DAT es distinto de su copia PCFUTBOL.DAT.bak. Una versión anterior "
                "del editor lo modificaba al cambiar las ligas, y eso hace que el juego falle "
                "con «ERROR: Equipo no encontrado» al entrar en el amistoso o en el "
                "seguimiento de la liga.\n\n¿Restaurar PCFUTBOL.DAT desde la copia .bak?",
                QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
        {
            QString err;
            if (!m_ligas.restaurarPcfutbol(&err))
                QMessageBox::critical(this, "Error", err);
        }
    }

    // Que la ventana quepa en pantallas pequeñas (contenido con barras de desplazamiento)
    UtilVentanas::hacerDesplazable(this);
}

QString DialogLigas::textoEquipo(quint32 id, int pos) const
{
    const PCFutbolLigas::Equipo e = m_ligas.equipo(id);
    QString t = QString("%1. %2").arg(pos).arg(e.nombre.isEmpty() ? QString("EQ95%1").arg(id, 4, 10, QChar('0')) : e.nombre);
    if (!e.fichaCompleta) t += "  ·sin ficha";
    return t;
}

void DialogLigas::rellenar()
{
    const QVector<quint32> l = m_ligas.lista();
    const int fin[3] = {PCFutbolLigas::N_PRIMERA,
                        PCFutbolLigas::N_PRIMERA + PCFutbolLigas::N_SEGUNDA,
                        int(l.size())};
    QStringList avisos;
    QSet<quint32> vistosB;
    for (int div = 0, i = 0; div < 3; div++)
    {
        m_listas[div]->clear();
        int pos = 1;
        for (; i < fin[div]; i++)
        {
            const quint32 id = l.at(i);
            if (div == 2)
            {
                if (vistosB.contains(id)) continue; // Ávila/Manchego van dos veces
                vistosB.insert(id);
            }
            QListWidgetItem *it = new QListWidgetItem(textoEquipo(id, pos++), m_listas[div]);
            it->setData(Qt::UserRole, id);
            const PCFutbolLigas::Equipo e = m_ligas.equipo(id);
            it->setToolTip(QString("EQ95%1.DBC").arg(id, 4, 10, QChar('0')));
            if (div < 2 && !e.fichaCompleta)
                avisos << QString("%1 no tiene ficha completa (estadio, presidente, historial...)").arg(e.nombre);
            if (div == 0 && !e.tieneCamisa)
                avisos << QString("%1 no tiene dibujo de equipación en DBDAT/CAMISAS").arg(e.nombre);
        }
    }
    if (!avisos.isEmpty())
        avisos.prepend("Avisos (probado en el juego: un equipo sin ficha completa, el Levante, "
                       "arranca la Liga Manager en Primera, su despacho se abre y sus partidos se "
                       "juegan; sólo le faltan los datos de club. Aun así, prueba una partida "
                       "nueva antes de jugar en serio):");
    m_lblAvisos->setText(avisos.join("\n· "));
    refrescarEuropa();
    m_btnGuardar->setText(m_ligas.modificada() ? "Guardar en el juego *" : "Guardar en el juego");
    m_elegidos.clear();
    m_btnIntercambiar->setEnabled(false);
    m_lblSeleccion->setText("Elige dos equipos de divisiones distintas.");
}

void DialogLigas::onSeleccion(QListWidgetItem *item)
{
    int lista = -1;
    for (int i = 0; i < 3; i++)
        if (item->listWidget() == m_listas[i]) lista = i;
    const quint32 id = item->data(Qt::UserRole).toUInt();

    // Un solo elegido por lista; se guardan los dos últimos
    for (int k = m_elegidos.size() - 1; k >= 0; k--)
        if (m_elegidos.at(k).first == lista) m_elegidos.removeAt(k);
    m_elegidos.append(qMakePair(lista, id));
    while (m_elegidos.size() > 2)
    {
        const int quitar = m_elegidos.first().first;
        m_listas[quitar]->clearSelection();
        m_elegidos.removeFirst();
    }

    if (m_elegidos.size() == 2)
    {
        m_lblSeleccion->setText(QString("Intercambiar: %1  ↔  %2")
                                .arg(m_ligas.equipo(m_elegidos.at(0).second).nombre,
                                     m_ligas.equipo(m_elegidos.at(1).second).nombre));
        m_btnIntercambiar->setEnabled(true);
    }
    else
    {
        m_lblSeleccion->setText(QString("Elegido: %1. Elige otro equipo de otra división.")
                                .arg(m_ligas.equipo(id).nombre));
        m_btnIntercambiar->setEnabled(false);
    }
}

void DialogLigas::intercambiar()
{
    if (m_elegidos.size() != 2) return;
    m_ligas.intercambiar(m_elegidos.at(0).second, m_elegidos.at(1).second);
    rellenar();
}

void DialogLigas::deshacerTodo()
{
    m_ligas.deshacerTodo();
    rellenar();
}

void DialogLigas::guardar()
{
    if (!m_ligas.modificada())
    {
        QMessageBox::information(this, "Sin cambios", "No has cambiado ningún equipo de división.");
        return;
    }
    if (QMessageBox::question(this, "Confirmar",
            "Se van a modificar los ejecutables del juego PCMANAG.DAT y PROMANAG.DAT. "
            "La primera vez se guarda una copia .bak de cada uno.\n\n"
            "Trabaja sobre una COPIA del juego. ¿Continuar?",
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    QString error;
    QStringList informe;
    if (!m_ligas.guardar(&error, &informe))
    {
        QMessageBox::critical(this, "Error", error);
        return;
    }
    rellenar();
    QMessageBox::information(this, "Guardado",
                             "Composición de ligas guardada:\n\n" + informe.join("\n") +
                             "\n\nEmpieza una partida NUEVA para ver el cambio.");
}

// --- Clasificados para Europa ------------------------------------------------
void DialogLigas::construirEuropa()
{
    QGridLayout *g = new QGridLayout(m_gbEuropa);
    if (!m_ligas.europaEncontrada())
    {
        g->addWidget(new QLabel("No se ha encontrado la tabla de clasificados para Europa "
                                "en PCMANAG.DAT/PROMANAG.DAT.", this), 0, 0);
        return;
    }
    // Todos los equipos de la lista (Primera, Segunda, Segunda B), sin repetir
    const QVector<quint32> l = m_ligas.lista();
    QVector<quint32> ids;
    for (quint32 id : l) if (!ids.contains(id)) ids.append(id);

    m_cbEuropa.clear();
    for (int c = 0; c < 3; c++)
    {
        const QVector<quint32> actuales = m_ligas.europa(c);
        g->addWidget(new QLabel("<b>" + PCFutbolLigas::nombreCompeticion(c) + "</b>", this), 0, c);
        QVector<QComboBox *> fila;
        for (int p = 0; p < actuales.size(); p++)
        {
            QComboBox *cb = new QComboBox(this);
            for (int i = 0; i < ids.size(); i++)
            {
                const int pos = l.indexOf(ids.at(i));
                const QString div = pos < PCFutbolLigas::N_PRIMERA ? "1ª"
                                  : (pos < PCFutbolLigas::N_PRIMERA + PCFutbolLigas::N_SEGUNDA ? "2ª" : "2ªB");
                const PCFutbolLigas::Equipo e = m_ligas.equipo(ids.at(i));
                cb->addItem(QString("%1 (%2)").arg(e.nombre.isEmpty() ? QString::number(ids.at(i)) : e.nombre, div),
                            ids.at(i));
            }
            connect(cb, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, c, p, cb](int) {
                m_ligas.setEuropa(c, p, cb->currentData().toUInt());
                m_btnGuardar->setText(m_ligas.modificada() ? "Guardar en el juego *" : "Guardar en el juego");
            });
            g->addWidget(cb, 1 + p, c);
            fila.append(cb);
        }
        m_cbEuropa.append(fila);
    }
    QLabel *nota = new QLabel("Sólo se cambia QUIÉN va, no cuántos (1 a la Copa de Europa, 2 a la "
                              "Recopa, 3 a la UEFA). Probado en el juego.", this);
    nota->setWordWrap(true);
    nota->setStyleSheet("color:#9fb0b8;");
    g->addWidget(nota, 4, 0, 1, 3);
}

void DialogLigas::refrescarEuropa()
{
    for (int c = 0; c < m_cbEuropa.size(); c++)
    {
        const QVector<quint32> actuales = m_ligas.europa(c);
        for (int p = 0; p < m_cbEuropa.at(c).size() && p < actuales.size(); p++)
        {
            QComboBox *cb = m_cbEuropa.at(c).at(p);
            const QSignalBlocker bloqueo(cb);
            cb->setCurrentIndex(cb->findData(actuales.at(p)));
        }
    }
}
