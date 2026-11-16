#!/usr/bin/env python3
import struct,json,hashlib,sys
from pathlib import Path
def verify_pbp(p):
 b=p.read_bytes()
 if len(b)<40 or b[:4]!=b'\0PBP':raise ValueError('Invalid PBP magic')
 offs=list(struct.unpack_from('<8I',b,8))+[len(b)]
 if offs[0]<40 or offs!=sorted(offs) or offs[-2]>len(b):raise ValueError('PBP bounds')
 s=b[offs[0]:offs[1]]
 if s[:4]!=b'\0PSF':raise ValueError('SFO magic')
 _,keys,values,n=struct.unpack_from('<4I',s,4);fields={}
 if n>100 or 20+n*16>len(s):raise ValueError('SFO table')
 for i in range(n):
  key,fmt,length,capacity,offset=struct.unpack_from('<HHIII',s,20+i*16)
  if values+offset+capacity>len(s) or length>capacity or keys+key>=values:raise ValueError('SFO entry bounds')
  name=s[keys+key:values].split(b'\0')[0].decode();v=s[values+offset:values+offset+length]
  fields[name]=int.from_bytes(v,'little') if fmt==0x404 else v.rstrip(b'\0').decode()
 assert fields['MEMSIZE'] in (0, 1) and fields['EFAWARE']==1 and fields['TITLE']=='Descent PSP Go'
 elf=b[offs[6]:offs[7]]
 assert elf[:4]==b'\x7fELF','unencrypted PRX expected'
 assert elf[4:6]==b'\x01\x01' and struct.unpack_from('<H',elf,18)[0]==8,'32-bit little-endian MIPS required'
 phoff=struct.unpack_from('<I',elf,28)[0];phsize,phnum=struct.unpack_from('<HH',elf,42)
 if phoff+phsize*phnum>len(elf):raise ValueError('ELF program header bounds')
 for i in range(phnum):
  _,offset,_,_,filesize,memsize,_,_=struct.unpack_from('<8I',elf,phoff+i*phsize)
  if offset+filesize>len(elf) or filesize>memsize:raise ValueError('ELF segment bounds')
 return fields
def verify(root):
 fields=verify_pbp(root/'EBOOT.PBP');m=json.loads((root/'manifest.json').read_text())
 for f in m['files']:
  rel=Path(f['path'])
  if rel.is_absolute() or '..' in rel.parts:raise ValueError('Manifest path')
  p=root/rel
  if p.stat().st_size!=f['bytes'] or hashlib.sha256(p.read_bytes()).hexdigest()!=f['sha256']:raise ValueError('Hash mismatch '+str(rel))
 edition=json.loads((root/'edition.json').read_text())
 for name,info in edition['files'].items():
  p=root/'data'/name
  if p.stat().st_size!=info['bytes'] or hashlib.file_digest(open(p,'rb'),'sha256').hexdigest()!=info['sha256']:raise ValueError('Data edition mismatch')
 for song in edition['songs']:
  if 'pcm' not in song or not edition.get('music_included', True):
   continue
  track = root / 'music' / song['pcm']
  if not track.is_file():
   raise ValueError('Incomplete music: ' + track.name)
  if 'pcm_sha256' in song:
   with track.open('rb') as stream:
    if hashlib.file_digest(stream, 'sha256').hexdigest() != song['pcm_sha256']:
     raise ValueError('Music hash mismatch: ' + track.name)
 return {'sfo':fields,'files_verified':len(m['files']),'edition':m['edition'],'version':m['version']}
if __name__=='__main__':print(json.dumps(verify(Path(sys.argv[1])),indent=2))
