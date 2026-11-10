#include "dialog_tactica.h"
#include "util_ventanas.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QVBoxLayout>
#include <cmath>

using namespace PCFutbolTactica;

// ===========================================================================
// Campo
// ===========================================================================
CampoTactica::CampoTactica(QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void CampoTactica::setTactica(const Tactica &t)
{
    m_t = t;
    update();
}

void CampoTactica::setSeleccionado(int k)
{
    if (k < 0 || k >= JUGADORES || k == m_sel) return;
    m_sel = k;
    update();
}

double CampoTactica::celda() const
{
    return qMax(8.0, qMin((width() - 8) / double(COLUMNAS), (height() - 8) / double(FILAS)));
}

QRectF CampoTactica::campo() const
{
    const double c = celda();
    const QSizeF s(c * COLUMNAS, c * FILAS);
    return QRectF(QPointF((width() - s.width()) / 2, (height() - s.height()) / 2), s);
}

QPointF CampoTactica::centro(int x, int y) const
{
    const QRectF r = campo();
    const double c = celda();
    return QPointF(r.left() + (x + 0.5) * c, r.top() + (y + 0.5) * c);
}

QPoint CampoTactica::celdaEn(const QPointF &p) const
{
    const QRectF r = campo();
    const double c = celda();
    return QPoint(qBound(0, int((p.x() - r.left()) / c), COLUMNAS - 1),
                  qBound(0, int((p.y() - r.top()) / c), FILAS - 1));
}

QRectF CampoTactica::rectZona(const Jugador &j) const
{
    const QRectF r = campo();
    const double c = celda();
    return QRectF(r.left() + j.zx * c, r.top() + j.zy * c, j.zancho * c, j.zalto * c);
}

void CampoTactica::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF r = campo();
    const double c = celda();

    // césped a cuadros, como en el juego
    for (int x = 0; x < COLUMNAS; x++)
        for (int y = 0; y < FILAS; y++)
            p.fillRect(QRectF(r.left() + x * c, r.top() + y * c, c + 0.5, c + 0.5),
                       (x + y) % 2 ? QColor(0x2e, 0x8b, 0x2e) : QColor(0x38, 0x9e, 0x38));

    // líneas
    QPen blanco(QColor(255, 255, 255, 200), qMax(1.0, c / 18));
    p.setPen(blanco);
    p.setBrush(Qt::NoBrush);
    p.drawRect(r);
    p.drawLine(QPointF(r.center().x(), r.top()), QPointF(r.center().x(), r.bottom()));
    p.drawEllipse(r.center(), c * 1.3, c * 1.3);
    p.drawRect(QRectF(r.left(), r.top() + 2 * c, 2.2 * c, 5 * c));
    p.drawRect(QRectF(r.right() - 2.2 * c, r.top() + 2 * c, 2.2 * c, 5 * c));
    p.drawRect(QRectF(r.left(), r.top() + 3.2 * c, 0.8 * c, 2.6 * c));
    p.drawRect(QRectF(r.right() - 0.8 * c, r.top() + 3.2 * c, 0.8 * c, 2.6 * c));

    // zonas
    for (int k = 0; k < JUGADORES; k++)
    {
        if (!m_todas && k != m_sel) continue;
        const QRectF z = rectZona(m_t.j[k]).adjusted(1, 1, -1, -1);
        if (k == m_sel)
        {
            p.setBrush(QColor(255, 235, 59, 50));
            p.setPen(QPen(QColor(255, 235, 59), 2, Qt::DashLine));
        }
        else
        {
            p.setBrush(QColor(255, 255, 255, 18));
            p.setPen(QPen(QColor(255, 255, 255, 90), 1, Qt::DotLine));
        }
        p.drawRect(z);
        if (k == m_sel)
        {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(255, 235, 59));
            const double h = qMax(6.0, c / 4);
            for (const QPointF &q : {z.topLeft(), z.topRight(), z.bottomLeft(), z.bottomRight()})
                p.drawRect(QRectF(q.x() - h / 2, q.y() - h / 2, h, h));
        }
    }

    // jugadores: línea defensa → ataque, círculo y flecha
    QFont f = font();
    f.setBold(true);
    f.setPixelSize(int(qMax(8.0, c * 0.42)));
    p.setFont(f);
    auto dibujar = [&](int k) {
        const Jugador &j = m_t.j[k];
        const bool sel = k == m_sel;
        const QPointF d = centro(j.dx, j.dy), a = centro(j.ax, j.ay);
        const double rad = c * 0.36;
        if (d != a)
        {
            p.setPen(QPen(QColor(255, 255, 255, sel ? 230 : 110), sel ? 2 : 1));
            p.drawLine(d, a);
        }
        // flecha (ataque)
        QPainterPath fl;
        fl.moveTo(a.x() + rad * 1.1, a.y());
        fl.lineTo(a.x() - rad * 0.8, a.y() - rad);
        fl.lineTo(a.x() - rad * 0.8, a.y() + rad);
        fl.closeSubpath();
        p.setPen(QPen(sel ? Qt::white : QColor(0x5a, 0x45, 0x00), sel ? 2.5 : 1));
        p.setBrush(QColor(0xf6, 0xe0, 0x4b));
        p.drawPath(fl);
        p.setPen(Qt::black);
        p.drawText(QRectF(a.x() - rad, a.y() - rad, rad * 1.6, rad * 2), Qt::AlignCenter, QString::number(k + 1));
        // círculo (defensa)
        p.setPen(QPen(sel ? Qt::white : QColor(0x60, 0x30, 0x00), sel ? 2.5 : 1));
        p.setBrush(QColor(0xf0, 0x9a, 0x2a));
        p.drawEllipse(d, rad, rad);
        p.setPen(Qt::black);
        p.drawText(QRectF(d.x() - rad, d.y() - rad, rad * 2, rad * 2), Qt::AlignCenter, QString::number(k + 1));
    };
    for (int k = 0; k < JUGADORES; k++)
        if (k != m_sel) dibujar(k);
    dibujar(m_sel);                         // el seleccionado, por encima
}

void CampoTactica::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) return;
    const QPointF pos = e->position();
    const double c = celda();
    const double rad = c * 0.45;
    m_arr = Nada;

    // 1) esquinas de la zona del seleccionado
    const QRectF z = rectZona(m_t.j[m_sel]);
    const QPointF esquinas[4] = {z.topLeft(), z.topRight(), z.bottomLeft(), z.bottomRight()};
    for (int i = 0; i < 4; i++)
        if (QLineF(pos, esquinas[i]).length() < qMax(8.0, c / 3))
        {
            m_arr = Esquina;
            m_esquina = i;
            m_antes = m_t.j[m_sel];
            return;
        }

    // 2) círculos y flechas (primero el seleccionado)
    QList<int> orden;
    orden << m_sel;
    for (int k = JUGADORES - 1; k >= 0; k--)
        if (k != m_sel) orden << k;
    for (int k : orden)
    {
        const Jugador &j = m_t.j[k];
        Arrastre tipo = Nada;
        if (QLineF(pos, centro(j.dx, j.dy)).length() < rad) tipo = Defensa;
        else if (QLineF(pos, centro(j.ax, j.ay)).length() < rad) tipo = Ataque;
        if (tipo != Nada)
        {
            if (k != m_sel)
            {
                m_sel = k;
                emit seleccion(k);
            }
            m_arr = tipo;
            m_antes = m_t.j[k];
            update();
            return;
        }
    }

    // 3) dentro de la zona del seleccionado: mover la zona entera
    if (z.contains(pos))
    {
        m_arr = MoverZona;
        m_inicio = celdaEn(pos);
        m_antes = m_t.j[m_sel];
    }
}

void CampoTactica::mouseMoveEvent(QMouseEvent *e)
{
    const QPointF pos = e->position();
    if (m_arr == Nada)
    {
        // cursor según lo que haya debajo
        const QRectF z = rectZona(m_t.j[m_sel]);
        const double c = celda();
        Qt::CursorShape cur = Qt::ArrowCursor;
        for (const QPointF &q : {z.topLeft(), z.bottomRight()})
            if (QLineF(pos, q).length() < qMax(8.0, c / 3)) cur = Qt::SizeFDiagCursor;
        for (const QPointF &q : {z.topRight(), z.bottomLeft()})
            if (QLineF(pos, q).length() < qMax(8.0, c / 3)) cur = Qt::SizeBDiagCursor;
        if (cur == Qt::ArrowCursor)
            for (const Jugador &j : m_t.j)
                if (QLineF(pos, centro(j.dx, j.dy)).length() < c * 0.45 ||
                    QLineF(pos, centro(j.ax, j.ay)).length() < c * 0.45)
                    cur = Qt::OpenHandCursor;
        if (cur == Qt::ArrowCursor && z.contains(pos)) cur = Qt::SizeAllCursor;
        setCursor(cur);
        return;
    }

    Jugador j = m_antes;
    const QPoint q = celdaEn(pos);
    if (m_arr == Defensa || m_arr == Ataque)
    {
        (m_arr == Defensa ? j.dx : j.ax) = q.x();
        (m_arr == Defensa ? j.dy : j.ay) = q.y();
        // si sale de su zona, la zona crece hasta incluirla
        const int x0 = qMin(j.zx, q.x()), y0 = qMin(j.zy, q.y());
        const int x1 = qMax(j.zx + j.zancho - 1, q.x()), y1 = qMax(j.zy + j.zalto - 1, q.y());
        j.zx = x0; j.zy = y0; j.zancho = x1 - x0 + 1; j.zalto = y1 - y0 + 1;
    }
    else if (m_arr == Esquina)
    {
        // la esquina opuesta queda fija; la zona debe contener defensa y ataque
        int x0 = m_antes.zx, y0 = m_antes.zy;
        int x1 = m_antes.zx + m_antes.zancho - 1, y1 = m_antes.zy + m_antes.zalto - 1;
        const QPointF c = pos;
        const QRectF r = campo();
        const double t = celda();
        // la esquina se mueve a la línea de la cuadrícula más cercana
        const int gx = qBound(0, int(std::lround((c.x() - r.left()) / t)), COLUMNAS);
        const int gy = qBound(0, int(std::lround((c.y() - r.top()) / t)), FILAS);
        const int mnx = qMin(j.dx, j.ax), mxx = qMax(j.dx, j.ax);
        const int mny = qMin(j.dy, j.ay), mxy = qMax(j.dy, j.ay);
        if (m_esquina == 0 || m_esquina == 2) x0 = qMin(gx, mnx); else x1 = qMax(gx - 1, mxx);
        if (m_esquina == 0 || m_esquina == 1) y0 = qMin(gy, mny); else y1 = qMax(gy - 1, mxy);
        j.zx = x0; j.zy = y0; j.zancho = x1 - x0 + 1; j.zalto = y1 - y0 + 1;
    }
    else if (m_arr == MoverZona)
    {
        int ddx = q.x() - m_inicio.x(), ddy = q.y() - m_inicio.y();
        ddx = qBound(-m_antes.zx, ddx, COLUMNAS - (m_antes.zx + m_antes.zancho));
        ddy = qBound(-m_antes.zy, ddy, FILAS - (m_antes.zy + m_antes.zalto));
        j.zx += ddx; j.zy += ddy; j.dx += ddx; j.dy += ddy; j.ax += ddx; j.ay += ddy;
    }
    Tactica t = m_t;
    t.j[m_sel] = j;
    corregir(t);
    if (t != m_t)
    {
        m_t = t;
        update();
        emit cambiado();
    }
}

void CampoTactica::mouseReleaseEvent(QMouseEvent *)
{
    m_arr = Nada;
}

// ===========================================================================
// Ventana
// ===========================================================================
DialogTactica::DialogTactica(const Tactica &t, const QStringList &nombres, const QString &equipo,
                             const QString &carpetaJuego, QWidget *parent)
    : QDialog(parent), m_original(t), m_equipo(equipo), m_carpetaJuego(carpetaJuego)
{
    setWindowTitle("Táctica — " + equipo);
    resize(980, 600);
    QVBoxLayout *root = new QVBoxLayout(this);

    QLabel *ayuda = new QLabel(
        "Táctica con la que empieza el equipo en una <b>liga nueva</b> (las partidas guardadas llevan "
        "la suya). <b>Círculo</b> = dónde se coloca al defender, <b>flecha</b> = al atacar, "
        "<b>recuadro</b> = zona por la que se mueve. Arrastra círculos y flechas; las esquinas del "
        "recuadro cambian la zona y dentro del recuadro se mueve entera. El número es el puesto en "
        "la alineación.", this);
    ayuda->setWordWrap(true);
    root->addWidget(ayuda);

    QHBoxLayout *forms = new QHBoxLayout();
    forms->addWidget(new QLabel("Formación del juego:", this));
    const QStringList nf = nombresFormaciones();
    for (int i = 0; i < nf.size(); i++)
    {
        QPushButton *b = new QPushButton(nf[i], this);
        b->setToolTip("Pone las posiciones de la formación " + nf[i] + " del juego");
        connect(b, &QPushButton::clicked, this, [this, i]() {
            m_campo->setTactica(formacion(i));
            actualizarInfo();
        });
        forms->addWidget(b);
    }
    forms->addStretch();
    QPushButton *deshacer = new QPushButton("Deshacer cambios", this);
    connect(deshacer, &QPushButton::clicked, this, [this]() {
        m_campo->setTactica(m_original);
        actualizarInfo();
    });
    forms->addWidget(deshacer);
    root->addLayout(forms);

    QHBoxLayout *centro = new QHBoxLayout();
    m_lista = new QListWidget(this);
    for (int k = 0; k < JUGADORES; k++)
        m_lista->addItem(QString("%1  %2").arg(k + 1, 2).arg(k < nombres.size() ? nombres[k] : QString("—")));
    m_lista->setMaximumWidth(220);
    centro->addWidget(m_lista);
    m_campo = new CampoTactica(this);
    m_campo->setTactica(t);
    centro->addWidget(m_campo, 1);
    root->addLayout(centro, 1);

    QHBoxLayout *abajo = new QHBoxLayout();
    m_info = new QLabel(this);
    abajo->addWidget(m_info, 1);
    QCheckBox *zonas = new QCheckBox("Ver todas las zonas", this);
    connect(zonas, &QCheckBox::toggled, m_campo, &CampoTactica::setVerZonas);
    abajo->addWidget(zonas);
    QPushButton *cargar = new QPushButton("Cargar TACTIC.nnn…", this);
    cargar->setToolTip("Cargar una táctica grabada en el juego con «Grabar táctica» (carpeta TACTICS)");
    connect(cargar, &QPushButton::clicked, this, &DialogTactica::cargarDelJuego);
    abajo->addWidget(cargar);
    QPushButton *grabar = new QPushButton("Grabar en TACTICS…", this);
    grabar->setToolTip("Guardarla como TACTIC.nnn para poder cargarla en el juego con «Cargar táctica»");
    connect(grabar, &QPushButton::clicked, this, &DialogTactica::grabarEnElJuego);
    abajo->addWidget(grabar);
    root->addLayout(abajo);

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    bb->button(QDialogButtonBox::Ok)->setText("Aceptar");
    bb->button(QDialogButtonBox::Cancel)->setText("Cancelar");
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(bb);

    connect(m_lista, &QListWidget::currentRowChanged, this, [this](int k) {
        m_campo->setSeleccionado(k);
        actualizarInfo();
    });
    connect(m_campo, &CampoTactica::seleccion, this, [this](int k) {
        m_lista->setCurrentRow(k);
        actualizarInfo();
    });
    connect(m_campo, &CampoTactica::cambiado, this, &DialogTactica::actualizarInfo);
    m_lista->setCurrentRow(0);
    actualizarInfo();
    UtilVentanas::hacerDesplazable(this);
}

Tactica DialogTactica::tactica() const
{
    return m_campo->tactica();
}

void DialogTactica::actualizarInfo()
{
    const int k = m_campo->seleccionado();
    const Jugador &j = m_campo->tactica().j[k];
    QString t = QString("Jugador %1 — defensa: columna %2, fila %3 · ataque: columna %4, fila %5 · "
                        "zona: %6×%7 desde (%8, %9)")
                    .arg(k + 1).arg(j.dx).arg(j.dy).arg(j.ax).arg(j.ay)
                    .arg(j.zancho).arg(j.zalto).arg(j.zx).arg(j.zy);
    if (m_campo->tactica() != m_original) t += "  <span style='color:#e0b34a'>(cambiada)</span>";
    m_info->setText(t);
}

void DialogTactica::cargarDelJuego()
{
    QString carpeta = carpetaTactics(m_carpetaJuego);
    const QString f = QFileDialog::getOpenFileName(this, "Cargar táctica del juego", carpeta,
                                                   "Tácticas del juego (TACTIC.* tactic.*);;Todos (*)");
    if (f.isEmpty()) return;
    Tactica t;
    QString nombre, manager, err;
    if (!leerFichero(f, &t, &nombre, &manager, &err))
    {
        QMessageBox::warning(this, "Táctica", QFileInfo(f).fileName() + ": " + err);
        return;
    }
    corregir(t);
    m_campo->setTactica(t);
    actualizarInfo();
    QMessageBox::information(this, "Táctica", QString("Cargada «%1»%2.").arg(nombre,
        manager.trimmed().isEmpty() ? QString() : " (de " + manager.trimmed() + ")"));
}

void DialogTactica::grabarEnElJuego()
{
    QString carpeta = carpetaTactics(m_carpetaJuego);
    if (carpeta.isEmpty())
    {
        carpeta = QFileDialog::getExistingDirectory(this, "Carpeta TACTICS del juego", m_carpetaJuego);
        if (carpeta.isEmpty()) return;
    }
    const QString ruta = ficheroLibre(carpeta);
    if (ruta.isEmpty())
    {
        QMessageBox::warning(this, "Táctica", "Ya hay 200 tácticas en " + carpeta + " (TACTIC.000 a 199).");
        return;
    }
    bool ok = false;
    const QString nombre = QInputDialog::getText(this, "Grabar táctica", "Nombre (hasta 23 letras):",
                                                 QLineEdit::Normal, m_equipo.left(23), &ok).left(23);
    if (!ok) return;
    QString err;
    if (!grabarFichero(ruta, m_campo->tactica(), nombre, "Editor", &err))
    {
        QMessageBox::warning(this, "Táctica", err);
        return;
    }
    QMessageBox::information(this, "Táctica",
        QString("Grabada como %1.\nEn el juego: Entrenador → Tácticas → «Cargar táctica».").arg(QFileInfo(ruta).fileName()));
}
