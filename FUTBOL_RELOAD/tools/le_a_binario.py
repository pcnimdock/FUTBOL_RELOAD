"""Cargador mínimo de ejecutables LE (DOS/4GW): genera una imagen plana por objeto
con las reubicaciones internas aplicadas. Uso: le_a_binario.py EXE carpeta_salida
Sólo reubicaciones internas (las de DOS/4GW); los objetos se escriben como objN.bin
y la base de cada uno en info.json."""
import sys, struct, json, os
if len(sys.argv) != 3:
    sys.exit(__doc__)
d = open(sys.argv[1], 'rb').read(); out = sys.argv[2]
le = struct.unpack_from('<I', d, 0x3C)[0]
assert d[le:le+2] == b'LE', d[le:le+2]
u32 = lambda o: struct.unpack_from('<I', d, le+o)[0]
npages, eip_obj, eip = u32(0x14), u32(0x18), u32(0x1C)
pgsize, lastpg = u32(0x28), u32(0x2C)
objtab, nobj, pagemap = u32(0x40), u32(0x44), u32(0x48)
fxpage, fxrec, datapg = u32(0x68), u32(0x6C), u32(0x80)
objs = []
for i in range(nobj):
    vs, base, fl, pidx, npe, _ = struct.unpack_from('<6I', d, le+objtab+i*24)
    objs.append(dict(vsize=vs, base=base, flags=fl, pidx=pidx, npe=npe))
def pagenum(p):  # p 1-based
    b = d[le+pagemap+(p-1)*4: le+pagemap+p*4]
    return (b[0] << 16) | (b[1] << 8) | b[2]
mem = []
for o in objs:
    m = bytearray(o['vsize'] if o['vsize'] else o['npe']*pgsize)
    for k in range(o['npe']):
        p = o['pidx'] + k; pn = pagenum(p)
        off = datapg + (pn-1)*pgsize
        sz = lastpg if pn == npages else pgsize
        chunk = d[off:off+sz]
        m[k*pgsize:k*pgsize+len(chunk)] = chunk[:max(0, len(m)-k*pgsize)]
    mem.append(m)
nfix = 0; unk = {}
def put(objn, addr_in_obj, val, size):
    m = mem[objn]
    if 0 <= addr_in_obj and addr_in_obj + size <= len(m):
        m[addr_in_obj:addr_in_obj+size] = val.to_bytes(size, 'little', signed=False) if val >= 0 else (val & ((1 << (8*size))-1)).to_bytes(size, 'little')
for oi, o in enumerate(objs):
    for k in range(o['npe']):
        p = o['pidx'] + k
        a = struct.unpack_from('<I', d, le+fxpage+(p-1)*4)[0]
        b = struct.unpack_from('<I', d, le+fxpage+p*4)[0]
        r = le + fxrec + a; end = le + fxrec + b
        while r < end:
            src, fl = d[r], d[r+1]; r += 2
            if src & 0x20:
                cnt = d[r]; r += 1
            else:
                srcoff = struct.unpack_from('<h', d, r)[0]; r += 2
            if fl & 0x03 != 0:
                unk[fl & 3] = unk.get(fl & 3, 0) + 1
                raise SystemExit('fixup externo no soportado (flags %02x)' % fl)
            if fl & 0x40: tobj = struct.unpack_from('<H', d, r)[0]; r += 2
            else: tobj = d[r]; r += 1
            stype = src & 0x0F
            if stype == 0x02:  # selector 16 bits: sin offset
                toff = 0
            elif fl & 0x10: toff = struct.unpack_from('<I', d, r)[0]; r += 4
            else: toff = struct.unpack_from('<H', d, r)[0]; r += 2
            srcs = []
            if src & 0x20:
                for _ in range(cnt):
                    srcs.append(struct.unpack_from('<h', d, r)[0]); r += 2
            else:
                srcs = [srcoff]
            target = objs[tobj-1]['base'] + toff
            for so in srcs:
                at = k*pgsize + so
                if stype == 0x07: put(oi, at, target, 4)
                elif stype == 0x08: put(oi, at, target - (o['base'] + at + 4), 4)
                elif stype == 0x05: put(oi, at, toff & 0xFFFF, 2)
                elif stype == 0x06: put(oi, at, target, 4)
                nfix += 1
os.makedirs(out, exist_ok=True)
info = dict(eip_obj=eip_obj, eip=eip, objs=[])
for i, (o, m) in enumerate(zip(objs, mem)):
    fn = os.path.join(out, 'obj%d.bin' % (i+1)); open(fn, 'wb').write(m)
    info['objs'].append(dict(n=i+1, base=hex(o['base']), size=len(m), flags=hex(o['flags'])))
json.dump(info, open(os.path.join(out, 'info.json'), 'w'), indent=1)
print(json.dumps(info), 'fixups', nfix)
