#!/usr/bin/env python3
from pathlib import Path
import subprocess,json,hashlib,urllib.parse,os
ROOT=Path(__file__).resolve().parents[1];debs=ROOT/'deps/debs';debs.mkdir(parents=True,exist_ok=True)
lock=json.loads((ROOT/'provenance.lock.json').read_text())
for name,sha in lock['host_debs'].items():
 p=debs/name
 if not p.exists():
  package,version,_=name.split('_');subprocess.run(['apt','download',package+'='+urllib.parse.unquote(version)],cwd=debs,check=True)
 if hashlib.file_digest(open(p,'rb'),'sha256').hexdigest()!=sha:raise ValueError('Dependency hash '+name)
 subprocess.run(['dpkg-deb','-x',str(p),str(ROOT/'deps/root')],check=True)
pc=ROOT/'deps/pkgconfig';pc.mkdir(exist_ok=True)
for f in (ROOT/'deps/root/usr/lib/x86_64-linux-gnu/pkgconfig').glob('*.pc'):
 s=f.read_text().replace('prefix=/usr','prefix='+str(ROOT/'deps/root/usr')).replace('libdir=/usr/','libdir='+str(ROOT/'deps/root/usr')+'/').replace('includedir=/usr/','includedir='+str(ROOT/'deps/root/usr')+'/');(pc/f.name).write_text(s)
lib=str(ROOT/'deps/root/usr/lib/x86_64-linux-gnu')
(ROOT/'scripts/desktop-env.sh').write_text('export LD_LIBRARY_PATH="'+lib+':'+lib+'/pulseaudio"\nexport PKG_CONFIG_PATH="'+str(pc)+'"\nexport PATH="'+str(ROOT/'deps/root/usr/bin')+':$PATH"\n')
