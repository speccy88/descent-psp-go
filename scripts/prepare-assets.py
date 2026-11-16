#!/usr/bin/env python3
"""Validate PC shareware assets and convert HMP delta encoding with strict bounds.
HMP layout/tempo semantics follow D1X-Rebirth 0.58.1 misc/hmp.c (Arne de Bruijn/JFFEE).
"""
from pathlib import Path
import struct,json,hashlib,subprocess
ROOT=Path(__file__).resolve().parents[1]
def u32(b,p):
 if p<0 or p+4>len(b):raise ValueError('u32 bounds')
 return struct.unpack_from('<I',b,p)[0]
def hog_entries(b):
 if b[:3]!=b'DHF':raise ValueError('HOG magic')
 out={};p=3
 while p<len(b):
  if p+17>len(b):raise ValueError('HOG directory bounds')
  name=b[p:p+13].split(b'\0')[0].decode('ascii').lower();n=u32(b,p+13);p+=17
  if not name or '/' in name or '\\' in name or name in out or p+n>len(b):raise ValueError('HOG entry')
  out[name]=b[p:p+n];p+=n
 return out
def vlq(n):
 if n<0 or n>0xfffffff:raise ValueError('MIDI delta overflow')
 out=[n&127];n>>=7
 while n:out.insert(0,(n&127)|128);n>>=7
 return bytes(out)
TIMED=[]
def convert_track(b):
 p=0;out=bytearray();ended=False;ticks=0
 while p<len(b):
  dt=0
  for i in range(4):
   if p>=len(b):raise ValueError('HMP delta truncated')
   c=b[p];p+=1;dt|=(c&127)<<(7*i)
   if c&128:break
  else:raise ValueError('HMP delta unterminated')
  ticks+=dt
  out+=vlq(dt)
  if p>=len(b):raise ValueError('HMP missing event')
  status=b[p];p+=1
  if status==255:
   if p+2>len(b):raise ValueError('meta truncated')
   typ,n=b[p:p+2];p+=2
   if n&128 or p+n>len(b):raise ValueError('meta length unsupported')
   if typ==81 and n==3:TIMED.append((ticks,"tempo",int.from_bytes(b[p:p+n],"big")))
   out+=bytes([255,typ,n])+b[p:p+n];p+=n
   if typ==47:
    if n:raise ValueError('bad end marker')
    ended=True;break
  elif status&240 in (128,144,160,176,224):
   if p+2>len(b):raise ValueError('event bounds')
   if status&240==176 and b[p] in (108,109,110,111):
    # HMI control/loop directives are not 7-bit MIDI data. Retain as sequencer metadata.
    TIMED.append((ticks,"control",b[p]))
    out+=bytes([255,127,3,status])+b[p:p+2]
   else:
    if max(b[p:p+2])>127:raise ValueError('invalid MIDI data')
    out+=bytes([status])+b[p:p+2]
   p+=2
  elif status&240 in (192,208):
   if p>=len(b) or b[p]>127:raise ValueError(f'event data pos={p} status={status:x} next={b[p:p+8].hex()}')
   out+=bytes([status,b[p]]);p+=1
  else:raise ValueError('unsupported status '+hex(status))
 if not ended:raise ValueError('missing track end')
 return b'MTrk'+struct.pack('>I',len(out))+out
def hmp2mid(b):
 TIMED.clear()
 if b[:8]!=b'HMIMIDIP':raise ValueError('HMP magic')
 n=u32(b,48);tempo=u32(b,56)
 if not 1<=n<=32 or not 1<=tempo<=20000:raise ValueError('HMP header')
 p=0x308;tracks=[]
 for i in range(n):
  length=u32(b,p+4)
  if length<12 or p+length>len(b):raise ValueError('HMP track bounds')
  tracks.append(convert_track(b[p+12:p+length]) if i else b'');p+=length
 # Original timing track replaced as in upstream hmp2mid; all musical tracks retained.
 conductor=bytes.fromhex('00ff510318800000ff2f00')
 return b'MThd'+struct.pack('>IHHH',6,1,n,int(tempo*1.6))+b'MTrk'+struct.pack('>I',len(conductor))+conductor+b''.join(tracks[1:])
def validate_pig(b):
 if len(b)!=5092871:raise ValueError('This preparation profile requires PC 1.4 shareware PIG')
 nb,ns=struct.unpack_from('<II',b);start=8+nb*17+ns*20
 if nb>1800 or ns>300 or start>len(b):raise ValueError('PIG header bounds')
 for i in range(nb):
  p=8+i*17;offset=u32(b,p+13)
  if start+offset>=len(b):raise ValueError('bitmap bounds')
 for i in range(ns):
  p=8+nb*17+i*20;size=u32(b,p+12);offset=u32(b,p+16)
  if start+offset+size>len(b):raise ValueError('sound bounds')
 return {'bitmaps':nb,'sounds':ns,'header_bytes':start}
def main():
 a=ROOT/'assets/shareware';h=(a/'descent.hog').read_bytes();p=(a/'descent.pig').read_bytes();entries=hog_entries(h);pig=validate_pig(p)
 out=ROOT/'assets/music';out.mkdir(parents=True,exist_ok=True)
 names=([l.split()[0].decode().lower() for l in entries['descent.sng'].splitlines() if l.strip()] if 'descent.sng' in entries else ['descent.hmp','briefing.hmp','credits.hmp','endlevel.hmp','endgame.hmp']+[f'game{i}.hmp' for i in range(5)])
 songs=[]
 for i,name in enumerate(names):
  if name not in entries:
   songs.append({'id':i,'source':name,'status':'not supplied by original shareware; original fallback behavior retained'});continue
  mid=out/(Path(name).stem+'.mid');mid.write_bytes(hmp2mid(entries[name]));(out/name).write_bytes(entries[name]);markers=[(t,c) for t,kind,c in TIMED if kind=='control' and c in (110,111)]
  division=int(u32(entries[name],56)*1.6)
  def frame_at(t):
   prev=0;us=0;tempo=0x188000
   for tick,kind,value in sorted(TIMED):
    if kind!='tempo' or tick>t:continue
    us+=(tick-prev)*tempo;prev=tick;tempo=value
   return int((us+(t-prev)*tempo)*44100/(division*1000000))
  starts=[t for t,c in markers if c==110];ends=[t for t,c in markers if c==111]
  loop={'start_frame':frame_at(min(starts)) if starts else 0,'end_frame':frame_at(min(ends)) if ends else None}
  if loop['end_frame'] is not None and loop['end_frame']>loop['start_frame']:(out/(Path(name).stem+'.loop')).write_text(f"{loop['start_frame']} {loop['end_frame']}\n")
  songs.append({'id':i,'source':name,'pcm':Path(name).stem+'.wav','loop':loop,'hmp_sha256':hashlib.sha256(entries[name]).hexdigest()})
 data={'edition':'Descent 1 PC shareware 1.4','levels':7,'source':'https://icculus.org/d2x/data/desc14sw.tar.gz','pig':pig,'files':{n:{'bytes':(a/n).stat().st_size,'sha256':hashlib.sha256((a/n).read_bytes()).hexdigest()} for n in ('descent.hog','descent.pig')},'hog_entries':list(entries),'songs':songs}
 (ROOT/'docs/edition.json').write_text(json.dumps(data,indent=2));print(json.dumps(data,indent=2))
if __name__=='__main__':main()
