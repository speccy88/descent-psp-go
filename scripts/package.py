#!/usr/bin/env python3
from pathlib import Path
import shutil,json,hashlib,os,zipfile,importlib.util
ROOT=Path(__file__).resolve().parents[1]
def main():
 sdk=Path(os.environ['PSPDEV']);out=ROOT/'dist/Descent-Go';out.mkdir(parents=True,exist_ok=True)
 for folder in ('data','music','userdata','logs','licenses'):(out/folder).mkdir(exist_ok=True)
 if any((out/'userdata').iterdir()) or any((out/'logs').iterdir()):raise ValueError('Package must have clean user state')
 shutil.copy2(ROOT/'build/psp/EBOOT.PBP',out/'EBOOT.PBP')
 for name in ('descent.hog','descent.pig'):shutil.copy2(ROOT/'assets/shareware'/name,out/'data'/name)
 for p in (ROOT/'assets/music').iterdir():
  if p.suffix in ('.wav','.loop'):shutil.copy2(p,out/'music'/p.name)
 shutil.copy2(ROOT/'docs/README-PSP.txt',out/'README-PSP.txt')
 shutil.copy2(ROOT/'engine/COPYING.txt',out/'licenses/D1X-Rebirth.txt')
 shutil.copy2(ROOT/'assets/shareware/readme.txt',out/'licenses/Descent-shareware-readme.txt')
 shutil.copy2(ROOT/'deps/root/usr/share/doc/timgm6mb-soundfont/copyright',out/'licenses/TimGM6mb.txt')
 shutil.copy2(ROOT/'reference/TinySoundFont/LICENSE',out/'licenses/TinySoundFont.txt')
 shutil.copy2('/usr/share/common-licenses/GPL-2',out/'licenses/GPL-2.txt')
 for lib in ('sdl','sdl-mixer','physfs','libogg','libvorbis','libmikmod','pspirkeyb'):
  src=sdk/'psp/share/licenses'/lib
  if src.exists():
   for p in src.rglob('*'):
    if p.is_file():shutil.copy2(p,out/'licenses'/(lib+'-'+p.name))
 edition=json.loads((ROOT/'docs/edition.json').read_text());edition['level_song_map']={str(i):'game'+str((i-1)%5)+'.wav' for i in range(1,8)}
 for song in edition['songs']:
  if 'pcm' in song:
   f=out/'music'/song['pcm']
   if not f.exists():raise ValueError('Missing mandatory track')
   song['pcm_sha256']=hashlib.file_digest(open(f,'rb'),'sha256').hexdigest()
 shutil.copy2(ROOT/'provenance.lock.json',out/'licenses/provenance.lock.json')
 (out/'edition.json').write_text(json.dumps(edition,indent=2))
 files=[]
 for p in sorted(out.rglob('*')):
  if p.is_file() and p.name!='manifest.json':files.append({'path':p.relative_to(out).as_posix(),'bytes':p.stat().st_size,'sha256':hashlib.file_digest(open(p,'rb'),'sha256').hexdigest()})
 manifest={'version':'0.1.0-candidate','edition':edition['edition'],'hardware_tested':False,'files':files}
 (out/'manifest.json').write_text(json.dumps(manifest,indent=2))
 (ROOT/'dist/SHA256SUMS.txt').write_text(''.join(f"{f['sha256']}  Descent-Go/{f['path']}\n" for f in files))
 debug=ROOT/'dist/debug';debug.mkdir(exist_ok=True)
 for name in ('descent-go','descent-go.map','descent-go.prx'):shutil.copy2(ROOT/'build/psp'/name,debug/(name+'.elf' if name=='descent-go' else name))
 with zipfile.ZipFile(ROOT/'dist/Descent-Go-0.1.0-shareware.zip','w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
  for p in sorted(out.rglob('*')):
   if p.is_file():z.write(p,Path('Descent-Go')/p.relative_to(out))
  for d in ('userdata','logs'):z.writestr('Descent-Go/'+d+'/',b'')
 print('Packaged',sum(f['bytes'] for f in files),'bytes',len(files),'files')
if __name__=='__main__':main()
