for l in open('i.txt').read().split('/')[1]:v=[open('i.txt').read().split('/')[0]]+([(v[1]+1)%(len(v[0])-2),open('o.txt','a').write(chr((ord(l)+ord(v[0][v[1]+2])*(1 if v[0][0]=='e' else -1))%128))] if 'v' in vars() else [1,open('o.txt','w').write(chr((ord(l)+ord(open('i.txt').read()[2])*(-1 if open('i.txt').read(1)=='d' else 1))%128))])