import unittest,sys,importlib,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'scripts'));m=importlib.import_module('prepare-assets')
class Assets(unittest.TestCase):
 def test_hog_directory(self):
  data=(ROOT/'assets/shareware/descent.hog').read_bytes();e=m.hog_entries(data);self.assertEqual(len([x for x in e if x.endswith('.sdl')]),7)
  for cut in (0,2,4,19,len(data)-1):
   with self.assertRaises(ValueError):m.hog_entries(data[:cut])
 def test_pig_edition(self):
  data=(ROOT/'assets/shareware/descent.pig').read_bytes();self.assertGreater(m.validate_pig(data)['bitmaps'],500)
  b=bytearray(data);struct.pack_into('<I',b,0,0xffffffff)
  with self.assertRaises(ValueError):m.validate_pig(b)
 def test_all_hmp(self):
  e=m.hog_entries((ROOT/'assets/shareware/descent.hog').read_bytes());n=0
  for name,b in e.items():
   if not name.endswith('.hmp'):continue
   n+=1;out=m.hmp2mid(b);self.assertEqual(out[:4],b'MThd')
   for cut in (0,8,50,0x309,len(b)//2):
    with self.assertRaises(ValueError):m.hmp2mid(b[:cut])
  self.assertEqual(n,8)
 def test_hmi_delta(self):
  self.assertEqual(m.convert_track(bytes.fromhex('0181ff2f00'))[-5:],bytes.fromhex('8101ff2f00'))
  with self.assertRaises(ValueError):m.convert_track(b'\0\0\0\0')
if __name__=='__main__':unittest.main()
