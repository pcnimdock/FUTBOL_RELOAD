#ifndef PCFUTBOL_DIBUJOKIT_H
#define PCFUTBOL_DIBUJOKIT_H

/*
 * pcfutbol_dibujokit.h / .cpp
 *
 * Dibujo de la equipación que sale en la ficha del equipo (Base de datos,
 * junto al botón «Plantilla»): DBDAT/CAMISAS/EQ95xxxx.DFG. Sólo hay 50.
 *
 * FORMATO (familia A): cabecera de 8 bytes 01 | ancho u16 | alto u16 | 04 00 00,
 * después, por fila, 4 planos de (ancho+7)/8 bytes (bit alto = píxel izquierdo).
 * Habitualmente 71x37 (1341 bytes con un byte final).
 *
 * PALETA (verificado en el juego, DOSBox, 9/11/2026): el fichero no lleva
 * paleta. Toda la ficha del equipo (dibujo, cintas y escudo grande) usa la
 * paleta de 16 colores de la FOTO DEL ESTADIO, DBDAT/BIGCAMP/ES95xxxx.DFG
 * (familia B: "00 10" + 16 x RGB). Si el equipo no tiene foto (11 de los 50),
 * se usa la del ESCUDO GRANDE, DBDAT/BIGESC/EQ95xxxx.DFG. Comprobado
 * cambiando colores en esos ficheros (Betis y Mérida).
 *   - Colores 1, 2 y 8: propios del equipo (los de su camiseta/escudo).
 *   - Colores 9-11: los usa la foto del estadio (magenta en los escudos).
 *   - Resto (0, 3-7, 12-15): fijos del interfaz, iguales en todos.
 * El índice 13 es el gris del fondo de la pantalla.
 * Los valores están en 8 bits pero la VGA sólo usa 6 (el juego hace >> 2).
 */

#include <QString>
#include <QByteArray>
#include <QImage>
#include <QVector>
#include <QColor>
#include <QStringList>

class PCFutbolDibujoKit
{
public:
    // id = número de equipo de 6 cifras (p. ej. "950019")
    bool cargar(const QString &carpetaJuego, const QString &id, QString *error);
    bool cargado() const { return !m_dfg.isEmpty(); }

    int ancho() const { return m_ancho; }
    int alto() const { return m_alto; }
    int indice(int x, int y) const { return m_indices.at(y * m_ancho + x); }
    void setIndice(int x, int y, int idx) { m_indices[y * m_ancho + x] = static_cast<quint8>(idx & 15); }
    // Sustituye un índice por otro en todo el dibujo; devuelve cuántos píxeles cambian
    int sustituir(int de, int a);
    // Píxeles de cada índice (para mostrar qué colores usa el dibujo)
    QVector<int> recuento() const;

    QVector<QRgb> paleta() const { return m_paleta; }
    QString origenPaleta() const { return m_origenPaleta; } // "BIGCAMP/..." o "BIGESC/..."
    static bool esColorDeEquipo(int idx) { return idx == 1 || idx == 2 || idx == 8; }
    // Cambia un color de equipo (1, 2 u 8). Se guarda en BIGCAMP y BIGESC.
    void setColorEquipo(int idx, QColor color);

    QImage imagen(int escala = 1) const;
    // Sustituye el dibujo por una imagen (se escala al tamaño y se ajusta a la paleta)
    void importar(const QImage &img);

    bool modificado() const;
    // Escribe CAMISAS (y BIGCAMP/BIGESC si cambiaron los colores), con .bak
    bool guardar(QString *error, QStringList *informe);

private:
    static QVector<QRgb> leerPaletaB(const QByteArray &d);
    static bool escribirConBak(const QString &ruta, const QByteArray &datos, QString *error);

    QString m_rutaDfg, m_rutaCamp, m_rutaEsc;
    QByteArray m_dfg, m_camp, m_esc;          // ficheros tal y como se leyeron
    int m_ancho = 0, m_alto = 0;
    QVector<quint8> m_indices, m_indicesOrig;
    QVector<QRgb> m_paleta, m_paletaOrig;
    QString m_origenPaleta;
};

#endif // PCFUTBOL_DIBUJOKIT_H
