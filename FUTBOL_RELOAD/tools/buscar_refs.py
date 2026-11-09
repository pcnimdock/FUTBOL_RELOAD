# buscar_refs.py imagen.bin 10000 "texto" ...   (imagen de le_plano.py, base en hex)
# Dónde está cada cadena y qué instrucciones/datos contienen su dirección (u32).
# xref.py imagen.bin base cadena...  -> direcciones de la cadena y del código que las referencia
import sys,struct
img=open(sys.argv[1],'rb').read(); base=int(sys.argv[2],16)
for s in sys.argv[3:]:
    b=s.encode('latin1'); i=img.find(b)
    while i>=0:
        a=base+i; refs=[]; j=img.find(struct.pack('<I',a))
        while j>=0: refs.append('%08x'%(base+j)); j=img.find(struct.pack('<I',a),j+1)
        print('%-40r @%08x refs: %s'%(s,a,' '.join(refs)))
        i=img.find(b,i+1)
