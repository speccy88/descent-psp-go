#!/usr/bin/env python3
"""Project-local pinned downloads. Never executes archive contents or overwrites engine/."""
from pathlib import Path,PurePosixPath
import urllib.request,hashlib,tarfile,json,subprocess
ROOT=Path(__file__).resolve().parents[1]
def fetch(url,path,sha):
 path.parent.mkdir(parents=True,exist_ok=True)
 if not path.exists():urllib.request.urlretrieve(url,path)
 if hashlib.file_digest(open(path,'rb'),'sha256').hexdigest()!=sha:raise ValueError('SHA-256 mismatch '+str(path))
def extract(archive,target,allow=None):
 with tarfile.open(archive) as t:
  members=t.getmembers()
  for m in members:
   p=PurePosixPath(m.name)
   if p.is_absolute() or '..' in p.parts or '\\' in m.name or ':' in m.name or not(m.isfile() or m.isdir()):raise ValueError('Unsafe archive member '+m.name)
  for m in members:
   if not m.isfile():continue
   p=PurePosixPath(m.name)
   if allow is not None and p.name.lower() not in allow:continue
   if allow is None and p.suffix.lower() in ('.exe','.com','.bat','.msi'):raise ValueError('Unexpected executable '+m.name)
   dest=target/(p.name.lower() if allow is not None else Path(m.name));dest.parent.mkdir(parents=True,exist_ok=True)
   if dest.exists():continue
   dest.write_bytes(t.extractfile(m).read())
def main():
 lock=json.loads((ROOT/'provenance.lock.json').read_text());e=lock['engine']
 fetch(e['url'],ROOT/'reference/d1x-rebirth_v0.58.1-src.tar.gz',e['sha256'])
 extract(ROOT/'reference/d1x-rebirth_v0.58.1-src.tar.gz',ROOT/'reference/d1x')
 fetch('https://icculus.org/d2x/data/desc14sw.tar.gz',ROOT/'reference/desc14sw.tar.gz','8b65eda01d03401d7a2dc71976528fbb7e0b07a93ce8ee0a9f1d9103623ae249')
 extract(ROOT/'reference/desc14sw.tar.gz',ROOT/'assets/shareware',{'descent.hog','descent.pig','readme.txt'})
 tiny=ROOT/'reference/TinySoundFont'
 if not tiny.exists():subprocess.run(['git','clone','https://github.com/schellingb/TinySoundFont.git',str(tiny)],check=True)
 subprocess.run(['git','-C',str(tiny),'checkout','--detach',lock['music']['commit']],check=True)
 print('Pinned source/data inputs verified. Engine working tree unchanged.')
if __name__=='__main__':main()
