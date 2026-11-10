#ifndef DIALOG_SONIDOS_H
#define DIALOG_SONIDOS_H

/*
 * Ventana «Sonidos del partido»: sustituir los 25 sonidos del juego por
 * cualquier audio (WAV, MP3, OGG, FLAC…) convertido para que el juego lo
 * acepte (ver pcfutbol_audio.h). Nada se escribe hasta pulsar «Guardar en el
 * juego»; la primera vez se guarda copia .bak de cada .RAW.
 */

#include <QDialog>
#include <QHash>
#include <QTemporaryDir>
#include "pcfutbol_audio.h"

class QTableWidget;
class QComboBox;
class QDoubleSpinBox;
class QCheckBox;
class QLabel;

class DialogSonidos : public QDialog
{
    Q_OBJECT
public:
    DialogSonidos(const QString &carpetaJuego, QWidget *parent = nullptr);
    bool valido() const { return !m_carpeta.isEmpty(); }
    int cambiosGuardados() const { return m_guardados; }

private:
    struct Pendiente
    {
        PCFutbolAudio::Audio audio;        // vacío si es «volver al original»
        QString origen;                    // fichero elegido
        QByteArray raw;
        PCFutbolAudio::Informe informe;
        bool restaurar = false;
    };

    void llenarTabla();
    void actualizarFila(int fila);
    void actualizarEstado();
    QString nombreFila(int fila) const;
    QByteArray rawActual(const QString &nombre) const;   // pendiente o el del juego
    PCFutbolAudio::Opciones opciones(const QString &nombre) const;
    void reconvertir(const QString &nombre);

    void sustituir();
    void escuchar();
    void exportarWav();
    void volverAlOriginal();
    void convertirSuelto();
    void guardar();

    QString m_carpeta;                    // .../SONIDOS
    QTableWidget *m_tabla = nullptr;
    QComboBox *m_volumen = nullptr;
    QDoubleSpinBox *m_ajuste = nullptr;
    QDoubleSpinBox *m_maximo = nullptr;
    QCheckBox *m_recortar = nullptr;
    QCheckBox *m_fundidos = nullptr;
    QLabel *m_estado = nullptr;
    QHash<QString, Pendiente> m_pendientes;
    QTemporaryDir m_tmp;
    int m_guardados = 0;
};

#endif // DIALOG_SONIDOS_H
