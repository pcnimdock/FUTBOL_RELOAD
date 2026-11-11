#include "dialog_partida.h"
#include "dialog_tactica.h"
#include "util_ventanas.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include "pcfutbol_plantillas.h"
#include <QHeaderView>
#include <QInputDialog>
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
                             const QString &carpetaJuego, QWidget *parent, const QList<EQUIPO> *bd)
    : QDialog(parent), m_ruta(ruta), m_carpetaJuego(carpetaJuego), m_bd(bd), m_p(partida)
{
    setWindowTitle("Partida guardada — " + QFileInfo(ruta).fileName());
    resize(1050, 680);
    QVBoxLayout *root = new QVBoxLayout(this);

    QLabel *cab = new QLabel(QString(
        "<b>%4</b> — <b>%1</b> · jornada %2 · grabada el %3<br>"
        "Los cambios se ven en el juego al <b>cargar esta partida</b>. La primera vez que guardes se "
        "hace copia .bak. Solo se cambian valores en su sitio: el resto del fichero queda igual.")
        .arg(m_p.jugadorHumano().toHtmlEscaped()).arg(m_p.jornada()).arg(m_p.fecha())
        .arg(m_p.esPromanager() ? "Promanager" : "Liga Manager"), this);
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
    m_tabla->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tabla->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tabla->horizontalHeader()->setSectionResizeMode(ColNombre, QHeaderView::Stretch);
    m_tabla->setToolTip("VE velocidad, RE resistencia, AG agresividad, CA calidad (0–99), EN energía (0–99). "
                        "La media (ME) la calcula el juego.");
    der->addWidget(m_tabla, 1);
    QHBoxLayout *acciones = new QHBoxLayout();
    QPushButton *ren = new QPushButton("Cambiar nombre…", this);
    ren->setToolTip("Nombre corto y largo del jugador seleccionado");
    connect(ren, &QPushButton::clicked, this, &DialogPartida::renombrar);
    QPushButton *tra = new QPushButton("Traspasar a…", this);
    tra->setToolTip("Mover el jugador seleccionado a otro equipo de la liga (conserva todos sus datos)");
    connect(tra, &QPushButton::clicked, this, &DialogPartida::traspasar);
    QPushButton *baja = new QPushButton("Quitar lesión y sanción", this);

    baja->setToolTip("Deja al jugador seleccionado sin lesión, sin sanción y sin amarillas acumuladas");
    connect(baja, &QPushButton::clicked, this, [this]() { quitarBajas(false); });
    QPushButton *bajas = new QPushButton("… a todo el equipo", this);

    bajas->setToolTip("Lo mismo para toda la plantilla");
    connect(bajas, &QPushButton::clicked, this, [this]() { quitarBajas(true); });
    QPushButton *nuevo = new QPushButton("Añadir jugador…", this);
    nuevo->setToolTip("Añadir un jugador nuevo a la plantilla (o copiarlo de la base de datos del editor)");
    connect(nuevo, &QPushButton::clicked, this, &DialogPartida::anadirJugador);
    acciones->addWidget(nuevo);
    acciones->addWidget(ren);
    acciones->addWidget(tra);
    acciones->addWidget(baja);
    acciones->addWidget(bajas);
    acciones->addStretch();
    der->addLayout(acciones);
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
    for (int k = 0; k < 3; k++)
    {
        m_entr[k]->setEnabled(e.offEntrenamiento >= 0);
        m_entr[k]->setValue(e.entrenamiento[k]);
        if (e.offEntrenamiento < 0) m_entr[k]->setToolTip("Las partidas de Promanager no guardan entrenamiento");
    }

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
        QString texto = j.nombreCorto;
        if (j.lesionado) texto += QString("  ✚ lesionado (%1)").arg(qMax(1, j.jornadasBaja));
        else if (j.sancionado) texto += QString("  ▮ sancionado (%1)").arg(qMax(1, j.jornadasBaja));
        auto *nombre = fijo(texto);
        nombre->setToolTip(j.nombreLargo);
        if (j.lesionado || j.sancionado) nombre->setForeground(QColor(0xe0, 0x5a, 0x4a));
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

int DialogPartida::jugadorSeleccionado() const
{
    const int r = m_tabla->currentRow();
    if (r < 0 || !m_tabla->item(r, ColDorsal)) return -1;
    return m_tabla->item(r, ColDorsal)->data(Qt::UserRole).toInt();
}

void DialogPartida::renombrar()
{
    const int j = jugadorSeleccionado();
    if (m_actual < 0 || j < 0)
    {
        QMessageBox::information(this, "Partida", "Selecciona primero un jugador.");
        return;
    }
    recogerTabla();
    const auto &jug = m_p.equipos()[m_actual].jugadores[j];
    QDialog d(this);
    d.setWindowTitle("Cambiar nombre");
    QFormLayout *f = new QFormLayout(&d);
    QLineEdit *corto = new QLineEdit(jug.nombreCorto, &d);
    corto->setMaxLength(16);
    QLineEdit *largo = new QLineEdit(jug.nombreLargo, &d);
    largo->setMaxLength(40);
    f->addRow("Nombre corto (el de las listas):", corto);
    f->addRow("Nombre largo:", largo);
    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &d);
    connect(bb, &QDialogButtonBox::accepted, &d, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &d, &QDialog::reject);
    f->addRow(bb);
    if (d.exec() != QDialog::Accepted) return;
    if (corto->text() == jug.nombreCorto && largo->text() == jug.nombreLargo) return;
    QString err;
    if (!m_p.renombrarJugador(m_actual, j, corto->text(), largo->text(), &err))
    {
        QMessageBox::warning(this, "Cambiar nombre", err);
        return;
    }
    mostrarEquipo(m_actual);
}

void DialogPartida::traspasar()
{
    const int j = jugadorSeleccionado();
    if (m_actual < 0 || j < 0)
    {
        QMessageBox::information(this, "Partida", "Selecciona primero un jugador.");
        return;
    }
    recogerTabla();
    QStringList nombres;
    QList<int> indices;
    for (int k = 0; k < m_p.equipos().size(); k++)
    {
        const auto &e = m_p.equipos()[k];
        if (k == m_actual || e.id == 9900) continue;
        nombres << QString("%1 (%2 jugadores)").arg(e.nombreCorto).arg(e.jugadores.size());
        indices << k;
    }
    bool ok = false;
    const QString jug = m_p.equipos()[m_actual].jugadores[j].nombreCorto;
    const QString elegido = QInputDialog::getItem(this, "Traspasar", "Traspasar a " + jug + " a:",
                                                  nombres, 0, false, &ok);
    if (!ok) return;
    const int destino = indices.value(nombres.indexOf(elegido), -1);
    QString info, err;
    if (!m_p.traspasar(m_actual, j, destino, &info, &err))
    {
        QMessageBox::warning(this, "Traspasar", err);
        return;
    }
    mostrarEquipo(m_actual);
    QMessageBox::information(this, "Traspasar", info + ".\n\nSe guarda al pulsar «Guardar partida».");
}

void DialogPartida::quitarBajas(bool todos)
{
    if (m_actual < 0) return;
    recogerTabla();
    if (todos)
    {
        for (int i = 0; i < m_p.equipos()[m_actual].jugadores.size(); i++) m_p.quitarBajas(m_actual, i);
    }
    else
    {
        const int j = jugadorSeleccionado();
        if (j < 0)
        {
            QMessageBox::information(this, "Partida", "Selecciona primero un jugador.");
            return;
        }
        m_p.quitarBajas(m_actual, j);
    }
    mostrarEquipo(m_actual);
}

void DialogPartida::anadirJugador()
{
    if (m_actual < 0) return;
    recogerTabla();
    if (m_p.equipos()[m_actual].id == 9900)
    {
        QMessageBox::information(this, "Añadir jugador", "No se añaden jugadores a las Estrellas Mundiales.");
        return;
    }
    QDialog d(this);
    d.setWindowTitle("Añadir jugador a " + m_p.equipos()[m_actual].nombreCorto);
    QFormLayout *f = new QFormLayout(&d);
    QLineEdit *corto = new QLineEdit(&d);
    corto->setMaxLength(16);
    QLineEdit *largo = new QLineEdit(&d);
    largo->setMaxLength(40);
    QComboBox *pos = new QComboBox(&d);
    pos->addItems(PCFutbolPlantillas::nombresPosicion());
    QComboBox *dem = new QComboBox(&d);
    dem->addItems(PCFutbolPlantillas::nombresDemarcacion());
    QCheckBox *ext = new QCheckBox("Extranjero", &d);
    QComboBox *piel = new QComboBox(&d);
    piel->addItems(PCFutbolPlantillas::nombresPiel());
    QComboBox *pelo = new QComboBox(&d);
    pelo->addItems(PCFutbolPlantillas::nombresPelo());
    QSpinBox *med[4];
    QHBoxLayout *fm = new QHBoxLayout();
    const char *nm[4] = {"VE", "RE", "AG", "CA"};
    for (int i = 0; i < 4; i++)
    {
        med[i] = new QSpinBox(&d);
        med[i]->setRange(0, PCFutbolPartida::MAX_MEDIA);
        med[i]->setValue(60);
        fm->addWidget(new QLabel(nm[i], &d));
        fm->addWidget(med[i]);
    }
    QCheckBox *apt[4];
    QHBoxLayout *fa = new QHBoxLayout();
    const char *na[4] = {"Técnica", "Remate", "Pase", "Tiro"};
    for (int i = 0; i < 4; i++) { apt[i] = new QCheckBox(na[i], &d); fa->addWidget(apt[i]); }
    QSpinBox *precio = new QSpinBox(&d);
    precio->setRange(0, 9999);
    precio->setValue(10);
    precio->setSuffix(" millones");
    QSpinBox *dorsal = new QSpinBox(&d);
    dorsal->setRange(0, 99);
    dorsal->setSpecialValueText("el primero libre");
    QSpinBox *en = new QSpinBox(&d);
    en->setRange(0, PCFutbolPartida::MAX_ENERGIA);
    en->setValue(99);

    // Copiar de la base de datos del editor
    quint16 punteroBd = 0;
    if (m_bd && !m_bd->isEmpty())
    {
        QPushButton *copiar = new QPushButton("Copiar de la base de datos…", &d);
        f->addRow(copiar);
        connect(copiar, &QPushButton::clicked, &d, [&]() {
            QStringList items;
            QList<QPair<int, int>> idx;
            for (int e = 0; e < m_bd->size(); e++)
                for (int k = 0; k < m_bd->at(e).lista_jugadores.size(); k++)
                {
                    items << QString("%1 — %2").arg(m_bd->at(e).lista_jugadores.at(k).NombreCorto, m_bd->at(e).NombreCorto);
                    idx << qMakePair(e, k);
                }
            bool ok = false;
            const QString it = QInputDialog::getItem(&d, "Copiar de la base de datos", "Jugador:", items, 0, true, &ok);
            const int i = items.indexOf(it);
            if (!ok || i < 0) return;
            const JUGADOR &j = m_bd->at(idx[i].first).lista_jugadores.at(idx[i].second);
            corto->setText(j.NombreCorto.left(16));
            largo->setText(j.NombreLargo.left(40));
            pos->setCurrentIndex(qBound(0, int(j.PosicionEnCampo), 3));
            dem->setCurrentIndex(qBound(0, int(j.DemarcacionPref), dem->count() - 1));
            ext->setChecked(j.Extranjero);
            piel->setCurrentIndex(qBound(0, int(j.Piel), piel->count() - 1));
            pelo->setCurrentIndex(qBound(0, int(j.Pelo), pelo->count() - 1));
            const int m[4] = {j.Velocidad, j.Regate, j.Agilidad, j.Agresividad};
            for (int k = 0; k < 4; k++) med[k]->setValue(m[k]);
            const int a[4] = {j.ActTecnica, j.ActRemate, j.ActPase, j.ActTiro};
            for (int k = 0; k < 4; k++) apt[k]->setChecked(a[k]);
            precio->setValue(j.Precio);
            dorsal->setValue(j.NumeroCamiseta);
            punteroBd = j.puntero;      // su identidad (foto), si está libre en la partida
        });
    }
    f->addRow("Nombre corto:", corto);
    f->addRow("Nombre largo:", largo);
    f->addRow("Posición:", pos);
    f->addRow("Demarcación:", dem);
    f->addRow(ext);
    f->addRow("Piel:", piel);
    f->addRow("Pelo:", pelo);
    f->addRow("Medias:", fm);
    f->addRow("Aptitudes:", fa);
    f->addRow("Precio:", precio);
    f->addRow("Dorsal:", dorsal);
    f->addRow("Energía:", en);
    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &d);
    connect(bb, &QDialogButtonBox::accepted, &d, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &d, &QDialog::reject);
    f->addRow(bb);
    if (d.exec() != QDialog::Accepted) return;

    PCFutbolPartida::Jugador j;
    j.puntero = punteroBd;
    j.nombreCorto = corto->text().trimmed();
    j.nombreLargo = largo->text().trimmed().isEmpty() ? j.nombreCorto : largo->text().trimmed();
    j.posicion = pos->currentIndex();
    j.demarcacion = dem->currentIndex();
    j.extranjero = ext->isChecked();
    j.piel = piel->currentIndex();
    j.pelo = pelo->currentIndex();
    for (int k = 0; k < 4; k++) { j.media[k] = med[k]->value(); j.aptitud[k] = apt[k]->isChecked(); }
    j.valor = precio->value() * 1e6;
    j.dorsal = dorsal->value();
    j.energia = en->value();
    QString info, err;
    if (!m_p.anadirJugador(m_actual, j, &info, &err))
    {
        QMessageBox::warning(this, "Añadir jugador", err);
        return;
    }
    mostrarEquipo(m_actual);
    m_lista->item(m_actual)->setText(m_p.equipos()[m_actual].nombreCorto);
    QMessageBox::information(this, "Añadir jugador", info + ".\n\nSe guarda al pulsar «Guardar partida».");
}
