#include "dialog_partida.h"
#include "dialog_tactica.h"
#include "util_ventanas.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {
enum Col { ColDorsal, ColNombre, ColPos, ColVE, ColRE, ColAG, ColCA, ColEN, ColValor, NCol };
const char *POS[] = {"POR", "DEF", "MED", "DEL"};
}

DialogPartida::DialogPartida(const QString &ruta, const PCFutbolPartida &partida,
                             const QString &carpetaJuego, QWidget *parent)
    : QDialog(parent), m_ruta(ruta), m_carpetaJuego(carpetaJuego), m_p(partida)
{
    setWindowTitle("Partida guardada — " + QFileInfo(ruta).fileName());
    resize(1050, 680);
    QVBoxLayout *root = new QVBoxLayout(this);

    QLabel *cab = new QLabel(QString(
        "<b>%1</b> · jornada %2 · grabada el %3<br>"
        "Los cambios se ven en el juego al <b>cargar esta partida</b>. La primera vez que guardes se "
        "hace copia .bak. Solo se cambian valores en su sitio: el resto del fichero queda igual.")
        .arg(m_p.jugadorHumano().toHtmlEscaped()).arg(m_p.jornada()).arg(m_p.fecha()), this);
    cab->setWordWrap(true);
    root->addWidget(cab);

    QHBoxLayout *centro = new QHBoxLayout();
    QVBoxLayout *izq = new QVBoxLayout();
    m_filtro = new QLineEdit(this);
    m_filtro->setPlaceholderText("Buscar equipo…");
    m_filtro->setClearButtonEnabled(true);
    m_lista = new QListWidget(this);
    m_lista->setMaximumWidth(230);
    izq->addWidget(m_filtro);
    izq->addWidget(m_lista, 1);
    centro->addLayout(izq);

    QVBoxLayout *der = new QVBoxLayout();
    m_titulo = new QLabel(this);
    QFont ft = m_titulo->font();
    ft.setPointSizeF(ft.pointSizeF() * 1.3);
    ft.setBold(true);
    m_titulo->setFont(ft);
    der->addWidget(m_titulo);

    QGroupBox *club = new QGroupBox("Club", this);
    QFormLayout *fl = new QFormLayout(club);
    m_caja = new QDoubleSpinBox(this);
    m_caja->setRange(-99e9, 99e9);
    m_caja->setDecimals(0);
    m_caja->setSingleStep(10e6);
    m_caja->setGroupSeparatorShown(true);
    m_caja->setSuffix(" pts");
    m_caja->setToolTip("Dinero en caja (Finanzas → Caja → «Caja»)");
    fl->addRow("Caja:", m_caja);
    QHBoxLayout *entr = new QHBoxLayout();
    const char *nom[3] = {"Defensa", "Mediocampo", "Delantera"};
    for (int i = 0; i < 3; i++)
    {
        m_entr[i] = new QSpinBox(this);
        m_entr[i]->setRange(PCFutbolPartida::MIN_ENTRENAMIENTO, PCFutbolPartida::MAX_ENTRENAMIENTO);
        m_entr[i]->setToolTip(QString("Entrenamiento específico (barras de la pantalla de tácticas). "
                                      "De %1 a %2 cada una y entre las tres como mucho %3, como en el juego.")
                                  .arg(PCFutbolPartida::MIN_ENTRENAMIENTO).arg(PCFutbolPartida::MAX_ENTRENAMIENTO)
                                  .arg(PCFutbolPartida::SUMA_MAX_ENTRENAMIENTO));
        entr->addWidget(new QLabel(nom[i], this));
        entr->addWidget(m_entr[i]);
    }
    entr->addStretch();
    QPushButton *tac = new QPushButton("Táctica…", this);
    connect(tac, &QPushButton::clicked, this, &DialogPartida::editarTactica);
    entr->addWidget(tac);
    fl->addRow("Entrenamiento:", entr);
    der->addWidget(club);

    m_tabla = new QTableWidget(0, NCol, this);
    m_tabla->setHorizontalHeaderLabels({"Nº", "Jugador", "Pos.", "VE", "RE", "AG", "CA", "EN", "Valor (pts)"});
    m_tabla->verticalHeader()->hide();
    m_tabla->horizontalHeader()->setSectionResizeMode(ColNombre, QHeaderView::Stretch);
    m_tabla->setToolTip("VE velocidad, RE resistencia, AG agresividad, CA calidad (0–99), EN energía (0–99). "
                        "La media (ME) la calcula el juego.");
    der->addWidget(m_tabla, 1);
    centro->addLayout(der, 1);
    root->addLayout(centro, 1);

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Close, this);
    bb->button(QDialogButtonBox::Save)->setText("Guardar partida");
    bb->button(QDialogButtonBox::Close)->setText("Cerrar");
    connect(bb, &QDialogButtonBox::accepted, this, &DialogPartida::guardar);
    connect(bb, &QDialogButtonBox::rejected, this, &DialogPartida::reject);
    root->addWidget(bb);

    connect(m_filtro, &QLineEdit::textChanged, this, [this](const QString &t) {
        for (int i = 0; i < m_lista->count(); i++)
            m_lista->item(i)->setHidden(!t.isEmpty() && !m_lista->item(i)->text().contains(t, Qt::CaseInsensitive));
    });
    connect(m_lista, &QListWidget::currentRowChanged, this, [this](int r) {
        recogerTabla();
        mostrarEquipo(r);
    });
    connect(m_caja, &QDoubleSpinBox::valueChanged, this, [this](double v) {
        if (!m_cargando && m_actual >= 0) m_p.equipos()[m_actual].caja = v;
    });
    for (int i = 0; i < 3; i++)
        connect(m_entr[i], &QSpinBox::valueChanged, this, [this, i](int v) {
            if (m_cargando || m_actual < 0) return;
            // entre las tres, como mucho 75: se recorta la que se está cambiando
            int otras = 0;
            for (int k = 0; k < 3; k++) if (k != i) otras += m_entr[k]->value();
            const int maximo = PCFutbolPartida::SUMA_MAX_ENTRENAMIENTO - otras;
            if (v > maximo)
            {
                QSignalBlocker b(m_entr[i]);
                v = qMax(PCFutbolPartida::MIN_ENTRENAMIENTO, maximo);
                m_entr[i]->setValue(v);
            }
            m_p.equipos()[m_actual].entrenamiento[i] = v;
        });

    llenarEquipos();
    // El equipo del jugador humano primero seleccionado, si se reconoce
    m_lista->setCurrentRow(0);
    UtilVentanas::hacerDesplazable(this);
}

void DialogPartida::llenarEquipos()
{
    m_lista->clear();
    for (const auto &e : m_p.equipos())
    {
        QString n = e.nombreCorto;
        if (e.id == 9900) n = "Sin equipo (Estrellas / Segunda B)";
        m_lista->addItem(n);
    }
}

void DialogPartida::mostrarEquipo(int i)
{
    if (i < 0 || i >= m_p.equipos().size()) return;
    m_cargando = true;
    m_actual = i;
    const auto &e = m_p.equipos()[i];
    m_titulo->setText(QString("%1 — %2 · entrenador: %3").arg(e.nombreCorto, e.estadio, e.entrenador));
    m_caja->setValue(e.caja);
    for (int k = 0; k < 3; k++) m_entr[k]->setValue(e.entrenamiento[k]);

    QList<int> orden;
    for (int r = 0; r < e.jugadores.size(); r++) orden << r;
    std::stable_sort(orden.begin(), orden.end(), [&e](int a, int b) {
        return e.jugadores[a].alineacion < e.jugadores[b].alineacion;
    });
    m_tabla->setRowCount(e.jugadores.size());
    for (int r = 0; r < orden.size(); r++)
    {
        const auto &j = e.jugadores[orden[r]];
        auto fijo = [](const QString &t) {
            auto *it = new QTableWidgetItem(t);
            it->setFlags(it->flags() & ~Qt::ItemIsEditable);
            return it;
        };
        auto *num = fijo(QString::number(j.alineacion));
        num->setData(Qt::UserRole, orden[r]);
        num->setToolTip(QString("Puesto en la alineación (dorsal %1)").arg(j.dorsal));
        m_tabla->setItem(r, ColDorsal, num);
        auto *nombre = fijo(j.nombreCorto);
        nombre->setToolTip(j.nombreLargo);
        m_tabla->setItem(r, ColNombre, nombre);
        m_tabla->setItem(r, ColPos, fijo(j.posicion >= 0 && j.posicion < 4 ? POS[j.posicion] : "?"));
        for (int k = 0; k < 4; k++)
        {
            auto *it = new QTableWidgetItem();
            it->setData(Qt::EditRole, j.media[k]);
            m_tabla->setItem(r, ColVE + k, it);
        }
        auto *en = new QTableWidgetItem();
        en->setData(Qt::EditRole, j.energia);
        m_tabla->setItem(r, ColEN, en);
        auto *va = new QTableWidgetItem();
        va->setData(Qt::EditRole, qint64(j.valor));
        m_tabla->setItem(r, ColValor, va);
    }
    m_tabla->resizeColumnsToContents();
    m_tabla->horizontalHeader()->setSectionResizeMode(ColNombre, QHeaderView::Stretch);
    m_cargando = false;
}

void DialogPartida::recogerTabla()
{
    if (m_actual < 0 || m_actual >= m_p.equipos().size()) return;
    auto &e = m_p.equipos()[m_actual];
    for (int r = 0; r < m_tabla->rowCount(); r++)
    {
        const int idx = m_tabla->item(r, ColDorsal)->data(Qt::UserRole).toInt();
        if (idx < 0 || idx >= e.jugadores.size()) continue;
        auto &j = e.jugadores[idx];
        for (int k = 0; k < 4; k++)
            j.media[k] = qBound(0, m_tabla->item(r, ColVE + k)->data(Qt::EditRole).toInt(), PCFutbolPartida::MAX_MEDIA);
        j.energia = qBound(0, m_tabla->item(r, ColEN)->data(Qt::EditRole).toInt(), PCFutbolPartida::MAX_ENERGIA);
        j.valor = qMax<qint64>(0, m_tabla->item(r, ColValor)->data(Qt::EditRole).toLongLong());
    }
}

void DialogPartida::editarTactica()
{
    if (m_actual < 0) return;
    recogerTabla();
    auto &e = m_p.equipos()[m_actual];
    if (!e.tieneTactica)
    {
        QMessageBox::information(this, "Táctica", "No se reconoce la táctica de este equipo en la partida.");
        return;
    }
    // El jugador k de la táctica es el que ocupa el puesto k de la alineación
    QStringList nombres;
    for (int k = 1; k <= PCFutbolTactica::JUGADORES; k++)
    {
        QString n = "—";
        for (const auto &j : e.jugadores)
            if (j.alineacion == k)
                n = QString("%1 · %2").arg(j.nombreCorto, j.posicion >= 0 && j.posicion < 4 ? POS[j.posicion] : "?");
        nombres << n;
    }
    DialogTactica d(e.tactica, nombres, e.nombreCorto + " (" + e.nombreTactica + ")", m_carpetaJuego, this);
    if (d.exec() == QDialog::Accepted) e.tactica = d.tactica();
}

void DialogPartida::guardar()
{
    recogerTabla();
    if (!m_p.cambiada())
    {
        QMessageBox::information(this, "Partida", "No hay cambios.");
        return;
    }
    QString err;
    if (!m_p.guardar(m_ruta, &err))
    {
        QMessageBox::warning(this, "Partida", err);
        return;
    }
    QMessageBox::information(this, "Partida",
        "Partida guardada. Para verla en el juego, cárgala desde Liga Manager → «Cargar Liga».");
}

void DialogPartida::reject()
{
    recogerTabla();
    if (m_p.cambiada() &&
        QMessageBox::question(this, "Partida", "Hay cambios sin guardar. ¿Cerrar sin guardarlos?",
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;
    QDialog::reject();
}
