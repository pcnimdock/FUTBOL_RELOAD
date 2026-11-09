# empujar.py CX CY DX DY [click]
# Mueve el cursor DEL JUEGO desde donde está (CX,CY, leído en una captura) a
# (CX+DX, CY+DY) y opcionalmente hace clic. Con autolock=false DOSBox sigue los
# movimientos del puntero del sistema, pero cada pantalla del juego usa su
# propia escala vertical, así que los clics absolutos no son fiables: se saca
# el puntero de la ventana, se vuelve a entrar justo donde está el cursor del
# juego y se avanza a pasos pequeños.
import sys, time
from Xlib import display, X
from Xlib.ext import xtest

d = display.Display(); root = d.screen().root

def buscar(w):
    try:
        c = w.get_wm_class()
        if c and 'dosbox' in c[0].lower():
            return w
    except Exception:
        pass
    for h in w.query_tree().children:
        r = buscar(h)
        if r:
            return r

w = buscar(root)
if not w:
    sys.exit('no hay ventana de DOSBox')
t = w.translate_coords(root, 0, 0)
OX, OY = -t.x, -t.y
cx, cy, dx, dy = map(int, sys.argv[1:5])

def mv(x, y):
    xtest.fake_input(d, X.MotionNotify, x=x, y=y); d.sync(); time.sleep(0.05)

pant = d.screen()
mv(pant.width_in_pixels - 2, pant.height_in_pixels - 2); time.sleep(0.3)   # fuera de la ventana
mv(OX + cx, OY + cy); time.sleep(0.4)                                       # entrar sobre el cursor
n = max(abs(dx), abs(dy)) // 5 + 1
for k in range(1, n + 1):
    mv(OX + cx + dx * k // n, OY + cy + dy * k // n)
time.sleep(0.4)
if len(sys.argv) > 5:
    xtest.fake_input(d, X.ButtonPress, 1); d.sync(); time.sleep(0.4)
    xtest.fake_input(d, X.ButtonRelease, 1); d.sync()
