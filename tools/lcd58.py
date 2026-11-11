import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from bdf import *
g,a,d=load(Path(__file__).resolve().parent / 'fonts' / '6x13.bdf')
def cell(cp):
    c=g[cp]; w,h,xo,yo=c['bbx']; B=bits(c)
    cl=[[0]*6 for _ in range(13)]
    top=a-(yo+h)
    for y in range(h):
        for x in range(w):
            if B[y][x]: cl[top+y][xo+x]=1
    return cl
def orr(*rs): return [max(v) for v in zip(*rs)]
out={}
for cp in range(0x10d0,0x10f1):
    c=cell(cp)
    rows=[orr(c[1],c[2],c[3]) if any(c[1]) else orr(c[2],c[3]), orr(c[4],c[5]), c[6],c[7],c[8],c[9],c[10], c[12] if any(c[12]) else c[11]]
    out[cp]=[r[:5] for r in rows]
if __name__=='__main__':
    cps=list(out)
    for s in range(0,33,11):
        ch=cps[s:s+11]
        print('   '.join('%-5s'%('%d%s'%(s+j,chr(cp))) for j,cp in enumerate(ch)))
        for r in range(8): print('   '.join(''.join('#' if v else '.' for v in out[cp][r]) for cp in ch))
        print()
