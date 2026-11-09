# le_plano.py EXE salida.bin
# Carga un ejecutable LE (DOS/4GW) como lo haría el cargador: aplica las
# reubicaciones y escribe UNA imagen plana desde la base más baja (0x10000).
# Las direcciones de docs/ se refieren a esta imagen:  dirección = 0x10000 + offset.
# Desensamblar:  objdump -D -b binary -m i386 --adjust-vma=0x10000 salida.bin
# Cargador mínimo de ejecutables LE (DOS/4GW): genera imagen plana con fixups aplicados.
# uso: le.py EXE salida.bin   -> imprime base de cada objeto
import sys,struct
d=open(sys.argv[1],'rb').read()
le=struct.unpack_from('<I',d,0x3c)[0]
assert d[le:le+2]==b'LE'
u32=lambda o:struct.unpack_from('<I',d,le+o)[0]
npages=u32(0x14); eipobj=u32(0x18); eip=u32(0x1c); psize=u32(0x28); lastp=u32(0x2c)
objtab=le+u32(0x40); nobj=u32(0x44); pmap=le+u32(0x48)
fpt=le+u32(0x68); frt=le+u32(0x6c); datap=u32(0x80)
objs=[]
for i in range(nobj):
    vs,base,fl,pidx,pn,_=struct.unpack_from('<6I',d,objtab+24*i); objs.append((vs,base,fl,pidx,pn))
lo=min(o[1] for o in objs); hi=max(o[1]+o[0] for o in objs)
img=bytearray(hi-lo)
def pagefile(p):  # p 1-based
    e=d[pmap+4*(p-1):pmap+4*p]; num=(e[0]<<16)|(e[1]<<8)|e[2]
    return datap+(num-1)*psize
pagebase={}
for i,(vs,base,fl,pidx,pn) in enumerate(objs):
    print("obj%d base=%08x size=%08x flags=%x pages=%d"%(i+1,base,vs,fl,pn))
    for k in range(pn):
        p=pidx+k; off=pagefile(p); n=psize if p<npages else lastp
        chunk=d[off:off+n]; a=base+k*psize-lo
        img[a:a+len(chunk)]=chunk[:max(0,min(len(chunk),len(img)-a))]
        pagebase[p]=base+k*psize
nfix=0
for p in range(1,npages+1):
    s=struct.unpack_from('<I',d,fpt+4*(p-1))[0]; e=struct.unpack_from('<I',d,fpt+4*p)[0]
    o=frt+s; end=frt+e
    while o<end:
        st=d[o]; fl=d[o+1]; o+=2
        if st&0x20: cnt=d[o]; o+=1; srcs=None
        else: srcs=[struct.unpack_from('<h',d,o)[0]]; o+=2
        if fl&3!=0: raise SystemExit("fixup no interno en pagina %d"%p)
        if fl&0x40: tobj=struct.unpack_from('<H',d,o)[0]; o+=2
        else: tobj=d[o]; o+=1
        t=st&0xf
        if t==2: toff=0
        elif fl&0x10: toff=struct.unpack_from('<I',d,o)[0]; o+=4
        else: toff=struct.unpack_from('<H',d,o)[0]; o+=2
        if srcs is None:
            srcs=[struct.unpack_from('<h',d,o+2*j)[0] for j in range(cnt)]; o+=2*cnt
        tgt=objs[tobj-1][1]+toff
        for so in srcs:
            a=pagebase[p]+so-lo
            if t==7 and 0<=a and a+4<=len(img): struct.pack_into('<I',img,a,tgt); nfix+=1
            elif t==8 and 0<=a and a+4<=len(img): struct.pack_into('<i',img,a,tgt-(pagebase[p]+so+4)); nfix+=1
print("fixups aplicados",nfix,"base imagen %08x"%lo,"entrada %08x"%(objs[eipobj-1][1]+eip))
open(sys.argv[2],'wb').write(img)
