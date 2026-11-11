#include "dialog_traspasos.h"
#include "pcfutbol_plantillas.h"
#include "pcfutbol_temporada_real.h"
#include "util_ventanas.h"

#include <QComboBox>
#include <QCompleter>
#include <QListWidget>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QMessageBox>

using namespace PCFutbolPlantillas;

DialogTraspasos::DialogTraspasos(const QList<EQUIPO> &equipos, int equipoInicial,
                                 const PCFutbolTemporadaReal *real, QWidget *parent)
    : QDialog(parent), m_equipos(equipos), m_real(real)
{
    setWindowTitle("Traspasos");
    setModal(true);
    resize(900, 620);

    QVBoxLayout *root = new QVBoxLayout(this);
    QLabel *ayuda = new QLabel(
        "Elige un equipo a cada lado, selecciona jugadores (Ctrl o Mayús para varios) y "
        "pulsa la flecha. El jugador conserva su foto y sus datos; si su dorsal está ocupado "
        "en el nuevo equipo se le da el primero libre, y entra el último en la alineación. "
        "Doble clic en un jugador también lo traspasa.\n"
        "Los marcados con ★ juegan partidos de la temporada real 95/96: si salen de su equipo el "
        "juego no arranca, así que al traspasarlos se deja también una copia en su equipo (como "
        "hace el juego original con los jugadores del filial).", this);
    ayuda->setWordWrap(true);
    root->addWidget(ayuda);

    QHBoxLayout *centro = new QHBoxLayout();
    for (int lado = 0; lado < 2; lado++)
    {
        QVBoxLayout *col = new QVBoxLayout();
        const int inicial = lado == 0 ? qBound(0, equipoInicial, int(m_equipos.size()) - 1)
                                      : (equipoInicial + 1 < m_equipos.size() ? equipoInicial + 1 : 0);
        m_combo[lado] = crearCombo(inicial);
        m_lista[lado] = new QListWidget(this);
        m_lista[lado]->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_info[lado] = new QLabel(this);
        m_info[lado]->setWordWrap(true);
        col->addWidget(m_combo[lado]);
        col->addWidget(m_lista[lado], 1);
        col->addWidget(m_info[lado]);
        centro->addLayout(col, 1);

        if (lado == 0)
        {
            QVBoxLayout *flechas = new QVBoxLayout();
            QPushButton *der = new QPushButton("→", this);
            QPushButton *izq = new QPushButton("←", this);
            der->setToolTip("Traspasar los seleccionados de la izquierda a la derecha");
            izq->setToolTip("Traspasar los seleccionados de la derecha a la izquierda");
            for (QPushButton *b : {der, izq}) { b->setMinimumSize(48, 36); }
            flechas->addStretch();
            flechas->addWidget(der);
            flechas->addWidget(izq);
            flechas->addStretch();
            connect(der, &QPushButton::clicked, this, [this]() { mover(0); });
            connect(izq, &QPushButton::clicked, this, [this]() { mover(1); });
            centro->addLayout(flechas);
        }
        connect(m_combo[lado], &QComboBox::currentIndexChanged, this, [this, lado]() { llenarLista(lado); });
        connect(m_lista[lado], &QListWidget::itemDoubleClicked, this, [this, lado]() { mover(lado); });
    }
    root->addLayout(centro, 1);

    m_marcar = new QCheckBox("Marcar como «Nuevo fichaje» (la etiqueta que sale junto al jugador en el juego)", this);
    m_marcar->setChecked(true);
    root->addWidget(m_marcar);

    root->addWidget(new QLabel("Traspasos hechos en esta ventana:", this));
    m_log = new QListWidget(this);
    m_log->setMaximumHeight(110);
    root->addWidget(m_log);

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    bb->button(QDialogButtonBox::Ok)->setText("Aceptar");
    bb->button(QDialogButtonBox::Cancel)->setText("Cancelar (deshacer todo)");
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(bb);

    llenarLista(0);
    llenarLista(1);
    UtilVentanas::hacerDesplazable(this);
}

QComboBox *DialogTraspasos::crearCombo(int inicial)
{
    QComboBox *cb = new QComboBox(this);
    cb->setEditable(true);                      // se puede escribir para buscar
    cb->setInsertPolicy(QComboBox::NoInsert);
    for (int i = 0; i < m_equipos.size(); i++)
        cb->addItem(m_equipos.at(i).NombreCorto, i);
    cb->completer()->setFilterMode(Qt::MatchContains);
    cb->completer()->setCompletionMode(QCompleter::PopupCompletion);
    cb->completer()->setCaseSensitivity(Qt::CaseInsensitive);
    cb->setCurrentIndex(inicial);
    return cb;
}

void DialogTraspasos::llenarLista(int lado)
{
    QListWidget *l = m_lista[lado];
    l->clear();
    const int e = m_combo[lado]->currentData().toInt();
    if (e < 0 || e >= m_equipos.size()) return;
    const EQUIPO &eq = m_equipos.at(e);

    QList<int> orden;
    for (int i = 0; i < eq.lista_jugadores.size(); i++) orden << i;
    std::stable_sort(orden.begin(), orden.end(), [&eq](int a, int b) {
        return eq.lista_jugadores.at(a).PosicionEnAlineacion < eq.lista_jugadores.at(b).PosicionEnAlineacion;
    });
    const QStringList pos = nombresPosicion();
    int porteros = 0;
    for (int i : orden)
    {
        const JUGADOR &j = eq.lista_jugadores.at(i);
        if (j.PosicionEnCampo == 0) porteros++;
        const bool atado = m_real && m_real->juegaPartidosReales(eq.EquipoIdDBC, j.puntero);
        QListWidgetItem *it = new QListWidgetItem(
            QString("%1  %2  ·  %3%4").arg(j.NumeroCamiseta, 2).arg(j.NombreCorto,
                j.PosicionEnCampo < pos.size() ? pos.at(j.PosicionEnCampo) : QString("?"),
                atado ? QString("  ★") : QString()), l);
        it->setData(Qt::UserRole, i);
        it->setToolTip(j.NombreLargo);
    }
    const int n = eq.lista_jugadores.size();
    QString info = QString("%1 jugadores, %2 porteros").arg(n).arg(porteros);
    if (eq.isBBDD_neg) info += " · ficha reducida (sin fecha, altura, peso, precio…)";
    if (n < MIN_JUGADORES_VISTO)
        info += QString("<br><span style='color:#e0b34a'>⚠ menos de %1 (ningún equipo original tiene menos)</span>")
                    .arg(MIN_JUGADORES_VISTO);
    if (n > MAX_JUGADORES_JUEGO)
        info += QString("<br><span style='color:#e05a4a'>✖ más de %1: el juego no arranca</span>")
                    .arg(MAX_JUGADORES_JUEGO);
    if (n > 0 && porteros == 0)
        info += "<br><span style='color:#e0b34a'>⚠ sin portero</span>";
    m_info[lado]->setText(info);
}

void DialogTraspasos::mover(int desde)
{
    const int hacia = 1 - desde;
    const int eo = m_combo[desde]->currentData().toInt();
    const int ed = m_combo[hacia]->currentData().toInt();
    if (eo == ed)
    {
        QMessageBox::information(this, "Traspasos", "Los dos lados tienen el mismo equipo.");
        return;
    }
    QList<int> idx;
    for (QListWidgetItem *it : m_lista[desde]->selectedItems()) idx << it->data(Qt::UserRole).toInt();
    if (idx.isEmpty()) return;
    // de mayor a menor para que los índices sigan valiendo al quitar
    std::sort(idx.begin(), idx.end(), std::greater<int>());
    QStringList atados;
    for (int i : idx)
    {
        const JUGADOR &j = m_equipos.at(eo).lista_jugadores.at(i);
        if (m_real && m_real->juegaPartidosReales(m_equipos.at(eo).EquipoIdDBC, j.puntero))
            atados << j.NombreCorto;
    }
    if (!atados.isEmpty() &&
        QMessageBox::question(this, "Temporada real",
            QString("%1 juega%2 partidos de la temporada real 95/96 con el %3. Si sale%2 del equipo, "
                    "el juego no arranca («Jugador no encontrado»).\n\n"
                    "Se traspasará%2 dejando también una copia en el %3, como hace el juego original "
                    "con los jugadores del filial. ¿Continuar?")
                .arg(atados.join(", "), atados.size() > 1 ? "n" : "", m_equipos.at(eo).NombreCorto),
            QMessageBox::Yes | QMessageBox::Cancel) != QMessageBox::Yes)
        return;
    QStringList hechos;
    for (int i : idx)
    {
        const JUGADOR &jj = m_equipos.at(eo).lista_jugadores.at(i);
        const bool copia = m_real && m_real->juegaPartidosReales(m_equipos.at(eo).EquipoIdDBC, jj.puntero);
        const QString t = traspasar(m_equipos, eo, i, ed, m_marcar->isChecked(), copia);
        if (!t.isEmpty()) hechos.prepend(t);
    }
    for (const QString &t : hechos)
    {
        m_registro << t;
        m_log->addItem(t);
    }
    m_log->scrollToBottom();
    llenarLista(0);
    llenarLista(1);
}
