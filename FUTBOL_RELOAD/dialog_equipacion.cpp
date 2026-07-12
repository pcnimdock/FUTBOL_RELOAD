#include "dialog_equipacion.h"
#include "paleta_kits.h"

#include <QPainter>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QShortcut>
#include <QPixmap>
#include <QIcon>
#include <QFile>
#include <QFileInfo>

static inline QColor colorPaleta(int idx)
{
    if (idx < 0 || idx > 255)
    {
        return QColor(0, 0, 0);
    }
    return QColor(paleta_kits[idx][0], paleta_kits[idx][1], paleta_kits[idx][2]);
}

// ===========================================================================
// LienzoCamiseta
// ===========================================================================
LienzoCamiseta::LienzoCamiseta(QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
    m_textura = QByteArray(PCFutbolEquipacion::TAM_TEXTURA, char(0x7B));
    PCFutbolEquipacion::restaurarSombra(m_textura);
    setFixedSize(sizeHint());
}

QSize LienzoCamiseta::sizeHint() const
{
    return QSize(PCFutbolEquipacion::SPRITE_W * m_zoom,
                 PCFutbolEquipacion::SPRITE_H * m_zoom);
}

void LienzoCamiseta::setZoom(int z)
{
    m_zoom = qBound(6, z, 32);
    setFixedSize(sizeHint());
    updateGeometry();
    update();
}

void LienzoCamiseta::setTextura(const QByteArray &textura168)
{
    m_textura = textura168;
    m_textura.resize(PCFutbolEquipacion::TAM_TEXTURA);
    update();
}

bool LienzoCamiseta::pixelDesdeRaton(const QPoint &p, int *sx, int *sy) const
{
    const int x = p.x() / m_zoom;
    const int y = p.y() / m_zoom;
    if (x < 0 || y < 0 || x >= PCFutbolEquipacion::SPRITE_W || y >= PCFutbolEquipacion::SPRITE_H)
    {
        return false;
    }
    *sx = x;
    *sy = y;
    return true;
}

void LienzoCamiseta::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    const int Z = m_zoom;

    // Fondo tipo tablero (zona transparente del sprite)
    for (int y = 0; y < PCFutbolEquipacion::SPRITE_H; y++)
    {
        for (int x = 0; x < PCFutbolEquipacion::SPRITE_W; x++)
        {
            const bool par = ((x + y) % 2) == 0;
            p.fillRect(x * Z, y * Z, Z, Z, par ? QColor(0x35, 0x38, 0x3a) : QColor(0x2b, 0x2e, 0x30));
        }
    }

    const int posHover = (m_hoverX >= 0)
            ? PCFutbolEquipacion::posicionTextura(m_hoverX, m_hoverY) : -1;

    for (int y = 0; y < PCFutbolEquipacion::SPRITE_H; y++)
    {
        for (int x = 0; x < PCFutbolEquipacion::SPRITE_W; x++)
        {
            const int pos = PCFutbolEquipacion::posicionTextura(x, y);
            if (pos < 0)
            {
                continue;
            }
            const quint8 idx = static_cast<quint8>(m_textura.at(pos));
            p.fillRect(x * Z, y * Z, Z, Z, colorPaleta(idx));

            // Resalta los píxeles que comparten el mismo byte de textura
            if (posHover >= 0 && pos == posHover)
            {
                p.fillRect(x * Z, y * Z, Z, Z, QColor(0x86, 0xaa, 0x1c, 70));
            }
        }
    }

    if (m_rejilla && Z >= 8)
    {
        p.setPen(QColor(0, 0, 0, 70));
        for (int x = 0; x <= PCFutbolEquipacion::SPRITE_W; x++)
        {
            p.drawLine(x * Z, 0, x * Z, PCFutbolEquipacion::SPRITE_H * Z);
        }
        for (int y = 0; y <= PCFutbolEquipacion::SPRITE_H; y++)
        {
            p.drawLine(0, y * Z, PCFutbolEquipacion::SPRITE_W * Z, y * Z);
        }
    }

    if (m_hoverX >= 0 && posHover >= 0)
    {
        p.setPen(QPen(QColor(0x86, 0xaa, 0x1c), 2));
        p.drawRect(m_hoverX * Z + 1, m_hoverY * Z + 1, Z - 2, Z - 2);
    }
}

void LienzoCamiseta::mousePressEvent(QMouseEvent *ev)
{
    int sx = 0, sy = 0;
    if (!pixelDesdeRaton(ev->pos(), &sx, &sy))
    {
        return;
    }

    if (ev->button() == Qt::RightButton)
    {
        const int c = PCFutbolEquipacion::colorEnPixel(m_textura, sx, sy);
        if (c >= 0)
        {
            m_colorActual = c;
            emit colorCogido(c);
        }
        return;
    }

    if (ev->button() == Qt::LeftButton)
    {
        emit trazoIniciado();
        if (PCFutbolEquipacion::pintarPixel(m_textura, sx, sy,
                                            static_cast<quint8>(m_colorActual),
                                            m_protegerSombra))
        {
            update();
            emit texturaCambiada();
        }
    }
}

void LienzoCamiseta::mouseMoveEvent(QMouseEvent *ev)
{
    int sx = 0, sy = 0;
    if (!pixelDesdeRaton(ev->pos(), &sx, &sy))
    {
        return;
    }

    if (sx != m_hoverX || sy != m_hoverY)
    {
        m_hoverX = sx;
        m_hoverY = sy;
        emit pixelSobrevolado(sx, sy, PCFutbolEquipacion::posicionTextura(sx, sy));
        update();
    }

    if (ev->buttons() & Qt::LeftButton)
    {
        if (PCFutbolEquipacion::pintarPixel(m_textura, sx, sy,
                                            static_cast<quint8>(m_colorActual),
                                            m_protegerSombra))
        {
            update();
            emit texturaCambiada();
        }
    }
}

void LienzoCamiseta::leaveEvent(QEvent *)
{
    m_hoverX = -1;
    m_hoverY = -1;
    update();
}

// ===========================================================================
// PaletaCamiseta
// ===========================================================================
static const int CELDA_PALETA = 18;

PaletaCamiseta::PaletaCamiseta(QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
    const QList<PCFutbolEquipacion::ColorEquipacion> cols =
            PCFutbolEquipacion::coloresDisponibles();
    for (const PCFutbolEquipacion::ColorEquipacion &c : cols)
    {
        QVector<int> fila;
        for (quint8 v : c.rampa)
        {
            fila.append(v);
        }
        m_filas.append(fila);
        m_nombres.append(c.nombre);
    }
    setFixedSize(sizeHint());
}

QSize PaletaCamiseta::sizeHint() const
{
    if (m_completo)
    {
        return QSize(16 * CELDA_PALETA + 1, 16 * CELDA_PALETA + 1);
    }
    return QSize(8 * CELDA_PALETA + 1, m_filas.size() * CELDA_PALETA + 1);
}

void PaletaCamiseta::setModoCompleto(bool v)
{
    m_completo = v;
    setFixedSize(sizeHint());
    updateGeometry();
    update();
}

void PaletaCamiseta::seleccionar(int idxPaleta)
{
    m_seleccionado = idxPaleta;
    update();
}

int PaletaCamiseta::celdaEn(const QPoint &p) const
{
    const int cx = p.x() / CELDA_PALETA;
    const int cy = p.y() / CELDA_PALETA;
    if (cx < 0 || cy < 0)
    {
        return -1;
    }
    if (m_completo)
    {
        if (cx >= 16 || cy >= 16)
        {
            return -1;
        }
        return cy * 16 + cx;
    }
    if (cy >= m_filas.size() || cx >= m_filas.at(cy).size())
    {
        return -1;
    }
    return m_filas.at(cy).at(cx);
}

void PaletaCamiseta::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(0x20, 0x23, 0x25));

    const int filas = m_completo ? 16 : m_filas.size();
    for (int f = 0; f < filas; f++)
    {
        const int cols = m_completo ? 16 : m_filas.at(f).size();
        for (int c = 0; c < cols; c++)
        {
            const int idx = m_completo ? (f * 16 + c) : m_filas.at(f).at(c);
            const QRect r(c * CELDA_PALETA, f * CELDA_PALETA, CELDA_PALETA, CELDA_PALETA);
            p.fillRect(r.adjusted(1, 1, -1, -1), colorPaleta(idx));
            if (idx == m_seleccionado)
            {
                p.setPen(QPen(QColor(0x86, 0xaa, 0x1c), 2));
                p.drawRect(r.adjusted(1, 1, -2, -2));
            }
        }
    }
}

void PaletaCamiseta::mousePressEvent(QMouseEvent *ev)
{
    const int idx = celdaEn(ev->pos());
    if (idx >= 0)
    {
        m_seleccionado = idx;
        update();
        emit colorElegido(idx);
    }
}

void PaletaCamiseta::mouseMoveEvent(QMouseEvent *ev)
{
    const int idx = celdaEn(ev->pos());
    if (idx >= 0)
    {
        setToolTip(QString("Índice 0x%1 (%2)")
                   .arg(idx, 2, 16, QChar('0')).arg(idx).toUpper());
    }
    else
    {
        setToolTip(QString());
    }
}

// ===========================================================================
// DialogEquipacion
// ===========================================================================
DialogEquipacion::DialogEquipacion(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Editar equipación (camiseta)");
    setModal(true);
    colores = PCFutbolEquipacion::coloresDisponibles();

    for (int i = 0; i < 2; i++)
    {
        m_tex[i] = QByteArray(PCFutbolEquipacion::TAM_TEXTURA, char(0x7B));
        PCFutbolEquipacion::restaurarSombra(m_tex[i]);
        m_texOrig[i] = m_tex[i];
    }

    // --- Ruta y número de equipo -------------------------------------------
    QGroupBox *gbRuta = new QGroupBox("Ubicación en el juego", this);
    QFormLayout *formRuta = new QFormLayout();

    QHBoxLayout *filaRuta = new QHBoxLayout();
    le_ruta = new QLineEdit(this);
    le_ruta->setPlaceholderText("Ruta a PALETAS/EQUIPOS.DAT...");
    QPushButton *btnBuscarRuta = new QPushButton("Buscar...", this);
    filaRuta->addWidget(le_ruta);
    filaRuta->addWidget(btnBuscarRuta);
    formRuta->addRow("EQUIPOS.DAT:", filaRuta);

    QHBoxLayout *filaNum = new QHBoxLayout();
    sp_numero = new QSpinBox(this);
    sp_numero->setRange(1, 300);
    QPushButton *btnLeer = new QPushButton("Leer equipación del fichero", this);
    filaNum->addWidget(sp_numero);
    filaNum->addWidget(btnLeer);
    filaNum->addStretch();
    formRuta->addRow("Número de equipo (n):", filaNum);
    gbRuta->setLayout(formRuta);

    QLabel *lblAviso = new QLabel(
        "⚠️ El número de equipo (n) es la posición del equipo entre los ficheros "
        "EQ*.DBC ordenados, sin contar Estrellas Mundiales. Si no es exacto, se "
        "puede sobrescribir la equipación de OTRO equipo.", this);
    lblAviso->setWordWrap(true);
    lblAviso->setStyleSheet("color:#e0b34a;");

    // --- Lienzo -------------------------------------------------------------
    m_cbEquipacion = new QComboBox(this);
    m_cbEquipacion->addItem("1ª equipación");
    m_cbEquipacion->addItem("2ª equipación");

    m_lienzo = new LienzoCamiseta(this);
    m_lienzo->setZoom(16);

    m_lblInfo = new QLabel("Clic izq.: pintar · Clic der.: cuentagotas · "
                           "Arrastrar: pintar seguido · Ctrl+Z: deshacer", this);
    m_lblInfo->setWordWrap(true);
    m_lblInfo->setMinimumWidth(340);
    m_lblInfo->setStyleSheet("color:#9fb0b8;");

    m_spZoom = new QSpinBox(this);
    m_spZoom->setRange(6, 32);
    m_spZoom->setValue(16);
    m_spZoom->setSuffix(" px");

    m_chkRejilla = new QCheckBox("Rejilla", this);
    m_chkRejilla->setChecked(true);

    m_chkSombra = new QCheckBox("Proteger sombra (recomendado)", this);
    m_chkSombra->setChecked(true);
    m_chkSombra->setToolTip("El byte 7 de la textura es el contorno negro del sprite "
                            "(0x60 en los 470 equipos originales, sin excepción). "
                            "Si lo desprotege y lo pinta, la camiseta perderá su "
                            "silueta negra dentro del juego.");

    QHBoxLayout *filaOpts = new QHBoxLayout();
    filaOpts->addWidget(new QLabel("Zoom:", this));
    filaOpts->addWidget(m_spZoom);
    filaOpts->addWidget(m_chkRejilla);
    filaOpts->addStretch();

    QVBoxLayout *colIzq = new QVBoxLayout();
    colIzq->addWidget(m_cbEquipacion);
    colIzq->addWidget(m_lienzo, 0, Qt::AlignCenter);
    colIzq->addLayout(filaOpts);
    colIzq->addWidget(m_chkSombra);
    colIzq->addWidget(m_lblInfo);
    colIzq->addStretch();

    // --- Paleta -------------------------------------------------------------
    m_paleta = new PaletaCamiseta(this);
    m_chkPaletaCompleta = new QCheckBox("Paleta completa (256 colores)", this);
    m_lblColor = new QLabel(this);
    m_lblColor->setFixedHeight(26);
    m_lblColor->setAutoFillBackground(true);

    QGroupBox *gbPaleta = new QGroupBox("Color", this);
    QVBoxLayout *lp = new QVBoxLayout(gbPaleta);
    lp->addWidget(m_paleta, 0, Qt::AlignLeft);
    lp->addWidget(m_chkPaletaCompleta);
    lp->addWidget(m_lblColor);

    // --- Patrones -----------------------------------------------------------
    m_cbPatron = new QComboBox(this);
    m_cbPatron->addItem("Liso");
    m_cbPatron->addItem("Rayas verticales");
    m_cbPatron->addItem("Franjas horizontales");

    m_cbColorA = new QComboBox(this);
    m_cbColorB = new QComboBox(this);
    for (const PCFutbolEquipacion::ColorEquipacion &c : colores)
    {
        QPixmap pm(14, 14);
        pm.fill(colorPaleta(c.rampa.at(c.rampa.size() / 2)));
        m_cbColorA->addItem(QIcon(pm), c.nombre);
        m_cbColorB->addItem(QIcon(pm), c.nombre);
    }
    if (m_cbColorB->count() > 2)
    {
        m_cbColorB->setCurrentIndex(2); // Blanco
    }

    m_spAncho = new QSpinBox(this);
    m_spAncho->setRange(1, 12);
    m_spAncho->setValue(4);
    m_spAncho->setToolTip("Ancho de cada raya en la textura. 4 = igual que los equipos originales.");

    QPushButton *btnPatron = new QPushButton("Aplicar patrón", this);

    QGroupBox *gbPatron = new QGroupBox("Patrón predefinido", this);
    QFormLayout *fp = new QFormLayout(gbPatron);
    fp->addRow("Patrón:", m_cbPatron);
    fp->addRow("Color A:", m_cbColorA);
    fp->addRow("Color B:", m_cbColorB);
    fp->addRow("Ancho de raya:", m_spAncho);
    fp->addRow(btnPatron);

    // --- Zonas --------------------------------------------------------------
    m_cbZona = new QComboBox(this);
    m_cbZona->addItem("Cuello");
    m_cbZona->addItem("Ribete inferior");
    m_cbZona->addItem("Manga izquierda");
    m_cbZona->addItem("Manga derecha");
    m_cbZona->addItem("Ambas mangas");
    m_cbZona->addItem("Hombros");
    m_cbZona->addItem("Torso");
    m_cbZona->addItem("Faldón (parte baja)");
    m_cbZona->addItem("Toda la camiseta");
    QPushButton *btnZona = new QPushButton("Rellenar zona con el color elegido", this);

    QGroupBox *gbZona = new QGroupBox("Zonas", this);
    QVBoxLayout *lz = new QVBoxLayout(gbZona);
    lz->addWidget(m_cbZona);
    lz->addWidget(btnZona);

    QVBoxLayout *colDer = new QVBoxLayout();
    colDer->addWidget(gbPaleta);
    colDer->addWidget(gbPatron);
    colDer->addWidget(gbZona);
    colDer->addStretch();

    QHBoxLayout *cols = new QHBoxLayout();
    cols->addLayout(colIzq);
    cols->addLayout(colDer);

    // --- Botones ------------------------------------------------------------
    QPushButton *btnUndo    = new QPushButton("Deshacer", this);
    QPushButton *btnRestore = new QPushButton("Restaurar original", this);
    QPushButton *btnGuardar = new QPushButton("Guardar en EQUIPOS.DAT", this);
    QPushButton *btnCerrar  = new QPushButton("Cerrar", this);
    btnGuardar->setDefault(true);

    QHBoxLayout *filaBotones = new QHBoxLayout();
    filaBotones->addWidget(btnUndo);
    filaBotones->addWidget(btnRestore);
    filaBotones->addStretch();
    filaBotones->addWidget(btnGuardar);
    filaBotones->addWidget(btnCerrar);

    QVBoxLayout *root = new QVBoxLayout(this);
    root->addWidget(gbRuta);
    root->addWidget(lblAviso);
    root->addLayout(cols);
    root->addLayout(filaBotones);

    // --- Conexiones ---------------------------------------------------------
    connect(btnBuscarRuta, &QPushButton::clicked, this, &DialogEquipacion::buscarRutaEquiposDat);
    connect(btnLeer, &QPushButton::clicked, this, &DialogEquipacion::leerDelFichero);
    connect(m_cbEquipacion, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DialogEquipacion::cambiarEquipacion);
    connect(m_paleta, &PaletaCamiseta::colorElegido, this, &DialogEquipacion::onColorElegido);
    connect(m_lienzo, &LienzoCamiseta::colorCogido, this, [this](int idx) {
        m_paleta->seleccionar(idx);
        onColorElegido(idx);
    });
    connect(m_lienzo, &LienzoCamiseta::trazoIniciado, this, &DialogEquipacion::onTrazoIniciado);
    connect(m_lienzo, &LienzoCamiseta::texturaCambiada, this, &DialogEquipacion::onTexturaCambiada);
    connect(m_lienzo, &LienzoCamiseta::pixelSobrevolado, this, &DialogEquipacion::onPixelSobrevolado);
    connect(m_chkPaletaCompleta, &QCheckBox::toggled, m_paleta, &PaletaCamiseta::setModoCompleto);
    connect(m_chkRejilla, &QCheckBox::toggled, m_lienzo, &LienzoCamiseta::setMostrarRejilla);
    connect(m_chkSombra, &QCheckBox::toggled, m_lienzo, &LienzoCamiseta::setProtegerSombra);
    connect(m_spZoom, QOverload<int>::of(&QSpinBox::valueChanged), m_lienzo, &LienzoCamiseta::setZoom);
    connect(btnPatron, &QPushButton::clicked, this, &DialogEquipacion::aplicarPatron);
    connect(btnZona, &QPushButton::clicked, this, &DialogEquipacion::rellenarZona);
    connect(btnUndo, &QPushButton::clicked, this, &DialogEquipacion::deshacer);
    connect(btnRestore, &QPushButton::clicked, this, &DialogEquipacion::restaurarOriginal);
    connect(btnGuardar, &QPushButton::clicked, this, &DialogEquipacion::guardarEnDisco);
    connect(btnCerrar, &QPushButton::clicked, this, &QDialog::close);
    new QShortcut(QKeySequence::Undo, this, SLOT(deshacer()));

    m_paleta->seleccionar(0x80);
    onColorElegido(0x80);
    refrescar();
}

// --- API pública ------------------------------------------------------------
void DialogEquipacion::setRutaEquiposDat(const QString &ruta)
{
    le_ruta->setText(ruta);
}

void DialogEquipacion::setNumeroEquipo(int n)
{
    if (n > 0)
    {
        sp_numero->setValue(n);
    }
}

void DialogEquipacion::mostrarTexturaActual(bool primera, const QByteArray &textura168)
{
    if (textura168.size() < PCFutbolEquipacion::TAM_TEXTURA)
    {
        return;
    }
    const int i = primera ? 0 : 1;
    m_tex[i] = textura168;
    m_texOrig[i] = textura168;
    m_undo[i].clear();
    refrescar();
}

QByteArray DialogEquipacion::texturaEditada(bool primera) const
{
    return m_tex[primera ? 0 : 1];
}

// --- Slots ------------------------------------------------------------------
void DialogEquipacion::onColorElegido(int idxPaleta)
{
    m_lienzo->setColorActual(idxPaleta);
    QPalette pal = m_lblColor->palette();
    pal.setColor(QPalette::Window, colorPaleta(idxPaleta));
    m_lblColor->setPalette(pal);
    m_lblColor->setText(QString("  Color 0x%1")
                        .arg(idxPaleta, 2, 16, QChar('0')).toUpper());
}

void DialogEquipacion::refrescar()
{
    m_lienzo->setTextura(m_tex[m_actual]);
}

void DialogEquipacion::empujarUndo()
{
    m_undo[m_actual].append(m_tex[m_actual]);
    if (m_undo[m_actual].size() > 100)
    {
        m_undo[m_actual].removeFirst();
    }
}

void DialogEquipacion::cambiarEquipacion(int idx)
{
    m_tex[m_actual] = m_lienzo->textura();
    m_actual = qBound(0, idx, 1);
    refrescar();
}

void DialogEquipacion::onTrazoIniciado()
{
    m_tex[m_actual] = m_lienzo->textura();
    empujarUndo();
}

void DialogEquipacion::onTexturaCambiada()
{
    m_tex[m_actual] = m_lienzo->textura();
}

void DialogEquipacion::onPixelSobrevolado(int sx, int sy, int pos)
{
    if (pos < 0)
    {
        m_lblInfo->setText("Fondo (fuera de la camiseta)");
        return;
    }
    const int color = PCFutbolEquipacion::colorEnPixel(m_tex[m_actual], sx, sy);
    m_lblInfo->setText(QString("Píxel (%1,%2) → byte %3 de la textura (fila %4, col %5) · "
                               "zona: %6 · color 0x%7")
                       .arg(sx).arg(sy).arg(pos)
                       .arg(pos / PCFutbolEquipacion::ANCHO)
                       .arg(pos % PCFutbolEquipacion::ANCHO)
                       .arg(nombreZona(pos))
                       .arg(color, 2, 16, QChar('0')));
}

QString DialogEquipacion::nombreZona(int pos) const
{
    struct Entrada { PCFutbolEquipacion::Zona z; const char *n; };
    static const Entrada tabla[] = {
        { PCFutbolEquipacion::Zona::Sombra,         "SOMBRA (fija)" },
        { PCFutbolEquipacion::Zona::Cuello,         "cuello" },
        { PCFutbolEquipacion::Zona::Ribete,         "ribete inferior" },
        { PCFutbolEquipacion::Zona::MangaIzquierda, "manga izquierda" },
        { PCFutbolEquipacion::Zona::MangaDerecha,   "manga derecha" },
        { PCFutbolEquipacion::Zona::Hombros,        "hombros" },
        { PCFutbolEquipacion::Zona::Torso,          "torso" },
        { PCFutbolEquipacion::Zona::Faldon,         "faldón" }
    };
    for (const Entrada &e : tabla)
    {
        if (PCFutbolEquipacion::posicionesZona(e.z).contains(pos))
        {
            return QString::fromUtf8(e.n);
        }
    }
    return "otra";
}

void DialogEquipacion::aplicarPatron()
{
    const int ia = qBound(0, m_cbColorA->currentIndex(), colores.size() - 1);
    const int ib = qBound(0, m_cbColorB->currentIndex(), colores.size() - 1);

    PCFutbolEquipacion::Patron patron = PCFutbolEquipacion::Patron::Liso;
    if (m_cbPatron->currentIndex() == 1)
    {
        patron = PCFutbolEquipacion::Patron::RayasVerticales;
    }
    else if (m_cbPatron->currentIndex() == 2)
    {
        patron = PCFutbolEquipacion::Patron::FranjasHorizontales;
    }

    m_tex[m_actual] = m_lienzo->textura();
    empujarUndo();
    m_tex[m_actual] = PCFutbolEquipacion::generarTextura(patron,
                                                         colores.at(ia), colores.at(ib),
                                                         m_spAncho->value(),
                                                         m_chkSombra->isChecked());
    refrescar();
}

void DialogEquipacion::rellenarZona()
{
    static const PCFutbolEquipacion::Zona zonas[] = {
        PCFutbolEquipacion::Zona::Cuello,
        PCFutbolEquipacion::Zona::Ribete,
        PCFutbolEquipacion::Zona::MangaIzquierda,
        PCFutbolEquipacion::Zona::MangaDerecha,
        PCFutbolEquipacion::Zona::Mangas,
        PCFutbolEquipacion::Zona::Hombros,
        PCFutbolEquipacion::Zona::Torso,
        PCFutbolEquipacion::Zona::Faldon,
        PCFutbolEquipacion::Zona::TodoLoVisible
    };
    const int i = qBound(0, m_cbZona->currentIndex(), 8);

    m_tex[m_actual] = m_lienzo->textura();
    empujarUndo();
    PCFutbolEquipacion::pintarZona(m_tex[m_actual], zonas[i],
                                   static_cast<quint8>(m_paleta->colorSeleccionado()),
                                   m_chkSombra->isChecked());
    refrescar();
}

void DialogEquipacion::deshacer()
{
    if (m_undo[m_actual].isEmpty())
    {
        return;
    }
    m_tex[m_actual] = m_undo[m_actual].takeLast();
    refrescar();
}

void DialogEquipacion::restaurarOriginal()
{
    m_tex[m_actual] = m_lienzo->textura();
    empujarUndo();
    m_tex[m_actual] = m_texOrig[m_actual];
    refrescar();
}

void DialogEquipacion::buscarRutaEquiposDat()
{
    const QString ruta = QFileDialog::getOpenFileName(
                this, "Selecciona PALETAS/EQUIPOS.DAT",
                le_ruta->text(), "EQUIPOS.DAT (EQUIPOS.DAT);;Todos los ficheros (*)");
    if (!ruta.isEmpty())
    {
        le_ruta->setText(ruta);
    }
}

void DialogEquipacion::leerDelFichero()
{
    if (le_ruta->text().isEmpty())
    {
        QMessageBox::warning(this, "Falta la ruta",
                             "Indica primero la ruta a PALETAS/EQUIPOS.DAT.");
        return;
    }

    bool alguna = false;
    for (int i = 0; i < 2; i++)
    {
        bool ok = false;
        const QByteArray bloque = PCFutbolEquipacion::leerBloqueEquipacion(
                    le_ruta->text(), sp_numero->value(), (i == 0), &ok);
        if (ok && bloque.size() == PCFutbolEquipacion::TAM_BLOQUE)
        {
            m_tex[i] = bloque.mid(8, PCFutbolEquipacion::TAM_TEXTURA);
            m_texOrig[i] = m_tex[i];
            m_undo[i].clear();
            alguna = true;
        }
    }

    if (!alguna)
    {
        QMessageBox::warning(this, "No se ha podido leer",
                             "No se ha podido leer el bloque de ese equipo. "
                             "Comprueba la ruta y el número de equipo.");
        return;
    }
    refrescar();
}

void DialogEquipacion::guardarEnDisco()
{
    m_tex[m_actual] = m_lienzo->textura();

    if (le_ruta->text().isEmpty())
    {
        QMessageBox::warning(this, "Falta la ruta",
                             "Indica primero la ruta a PALETAS/EQUIPOS.DAT.");
        return;
    }

    if (QMessageBox::question(this, "Confirmar guardado",
            QString("Se van a sobrescribir las DOS equipaciones del equipo n = %1.\n\n"
                    "Se hará antes una copia de seguridad (.bak) del fichero original.\n\n"
                    "¿Continuar?").arg(sp_numero->value()),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
    {
        return;
    }

    const QString rutaBak = le_ruta->text() + ".bak";
    if (!QFile::exists(rutaBak))
    {
        if (!QFile::copy(le_ruta->text(), rutaBak))
        {
            QMessageBox::critical(this, "Error",
                                  "No se ha podido crear la copia de seguridad. "
                                  "No se ha guardado nada.");
            return;
        }
    }

    bool ok = true;
    for (int i = 0; i < 2; i++)
    {
        const QByteArray bloque = PCFutbolEquipacion::bloqueDesdeTextura(m_tex[i]);
        if (!PCFutbolEquipacion::escribirBloqueEquipacion(le_ruta->text(),
                                                          sp_numero->value(),
                                                          (i == 0), bloque))
        {
            ok = false;
        }
    }

    if (ok)
    {
        m_guardado = true;
        for (int i = 0; i < 2; i++)
        {
            m_texOrig[i] = m_tex[i];
        }
        QMessageBox::information(this, "Guardado",
                                 "Equipaciones guardadas correctamente.\n"
                                 "Copia de seguridad del original en:\n" + rutaBak);
    }
    else
    {
        QMessageBox::critical(this, "Error",
                              "No se ha podido escribir alguna de las equipaciones. "
                              "Revisa la ruta y el número de equipo.\n"
                              "Tienes la copia de seguridad en:\n" + rutaBak);
    }
}
