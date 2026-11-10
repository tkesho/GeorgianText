def load(path):
    g={}; cur=None
    for line in open(path,encoding='latin-1'):
        p=line.split()
        if not p: continue
        if p[0]=='FONT_ASCENT': asc=int(p[1])
        if p[0]=='FONT_DESCENT': desc=int(p[1])
        if p[0]=='STARTCHAR': cur={'rows':[]}
        elif p[0]=='ENCODING': cur['enc']=int(p[1])
        elif p[0]=='DWIDTH': cur['dw']=int(p[1])
        elif p[0]=='BBX': cur['bbx']=list(map(int,p[1:5]))
        elif p[0]=='BITMAP': cur['inbm']=True
        elif p[0]=='ENDCHAR':
            g[cur['enc']]=cur; cur=None
        elif cur is not None and cur.get('inbm'):
            cur['rows'].append(int(p[0],16))
    return g,asc,desc
def bits(ch):
    w,h,xo,yo=ch['bbx']; nb=((w+7)//8)*8
    return [[(r>>(nb-1-x))&1 for x in range(w)] for r in ch['rows']]
