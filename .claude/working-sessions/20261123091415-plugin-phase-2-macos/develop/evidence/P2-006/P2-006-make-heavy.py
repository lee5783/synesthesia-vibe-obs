import sys
fr, sw, out = sys.argv[1:]
for src, name in ((fr, 'heavy fractopia.milk'), (sw, 'heavy swarm.milk')):
    text = open(src, encoding='latin-1').read()
    pad = ''.join(f'per_frame_{1000+i}=q30=q30+{i%7}*0.0001;\n' for i in range(60000))
    open(f'{out}/{name}', 'w', encoding='latin-1').write(text.rstrip('\n') + '\n' + pad)
