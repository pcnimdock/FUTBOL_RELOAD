# Control de la ventana de DOSBox (requiere autolock=false en la config).
#   dbx.py shot out.png | keys "Return,sleep2,5" | click x y   (coordenadas del juego)
import sys,time
from Xlib import display,X,XK
from Xlib.ext import xtest
from PIL import Image
d=display.Display(); root=d.screen().root
def find(w):
    try:
        c=w.get_wm_class()
        if c and 'dosbox' in c[0].lower(): return w
    except Exception: pass
    for ch in w.query_tree().children:
        r=find(ch)
        if r: return r
w=find(root)
if not w: sys.exit("no hay ventana de dosbox")
cmd=sys.argv[1]
def alfrente():
    # subir el marco de la ventana (padre directo de root) por si otra la tapa
    f=w
    while True:
        q=f.query_tree()
        if q.parent==root: break
        f=q.parent
    f.configure(stack_mode=X.Above); d.sync(); time.sleep(0.2)
if cmd in ('click','keys','shot'): alfrente()
def mot(dx,dy): xtest.fake_input(d,X.MotionNotify,True,x=dx,y=dy); d.sync(); time.sleep(0.015)
def btn():
    xtest.fake_input(d,X.ButtonPress,1); d.sync(); time.sleep(0.4)
    xtest.fake_input(d,X.ButtonRelease,1); d.sync()
if cmd=='shot':
    g=w.get_geometry(); raw=w.get_image(0,0,g.width,g.height,X.ZPixmap,0xffffffff)
    Image.frombytes('RGB',(g.width,g.height),raw.data,'raw','BGRX').save(sys.argv[2])
elif cmd=='keys':
    w.set_input_focus(X.RevertToParent,X.CurrentTime); d.sync(); time.sleep(0.2)
    for k in sys.argv[2].split(','):
        if k.startswith('sleep'): time.sleep(float(k[5:])); continue
        kc=d.keysym_to_keycode(XK.string_to_keysym(k))
        xtest.fake_input(d,X.KeyPress,kc); d.sync(); time.sleep(0.08)
        xtest.fake_input(d,X.KeyRelease,kc); d.sync(); time.sleep(0.3)
elif cmd=='click':
    # sin captura (autolock=false) el cursor del juego sigue los movimientos del
    # puntero; se resincroniza saturándolo en la esquina (0,0) y se avanza a pasos
    t=w.translate_coords(root,0,0); ox,oy=-t.x,-t.y
    x,y=int(sys.argv[2]),int(sys.argv[3])
    def mv(a,b): xtest.fake_input(d,X.MotionNotify,x=ox+a,y=oy+b); d.sync(); time.sleep(0.03)
    mv(320,240)
    for k in range(1,9): mv(320-40*k, 240-30*k)       # hasta (0,0), dentro de la ventana
    mv(0,0); time.sleep(0.1)
    n=max(abs(x),abs(y))//15+1
    for k in range(1,n+1): mv(x*k//n, y*k//n)
    time.sleep(0.25)
    if len(sys.argv)<5: btn()
