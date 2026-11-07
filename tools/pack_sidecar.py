"""Pack user-owned reference/enhanced images into the optional RRHD format."""
import argparse,struct,sys
from array import array
from pathlib import Path
from PIL import Image
def rgb555(im):
 values=array("H",(((r>>3)<<10)|((g>>3)<<5)|(b>>3) for r,g,b in im.convert("RGB").getdata()))
 if sys.byteorder!="little":values.byteswap()
 return values.tobytes()
def pack(reference,enhanced):
 if reference.size!=(640,480):raise ValueError("Reference must be the complete 640x480 logical frame")
 w,h=enhanced.size
 if not(640<=w<=2880 and 480<=h<=2160 and w*3==h*4):raise ValueError("Enhanced frame must be a supported 4:3 size")
 a,b=rgb555(reference),rgb555(enhanced);return b"RRHDv001"+struct.pack("<6I",w,h,len(a),len(b),0,0)+a+b
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument("--reference",type=Path,required=True);p.add_argument("--enhanced",type=Path,required=True);p.add_argument("--output",type=Path,required=True);a=p.parse_args()
 if a.output.exists():raise ValueError("Output already exists; preserving it")
 with Image.open(a.reference) as ref,Image.open(a.enhanced) as hd:data=pack(ref,hd)
 a.output.parent.mkdir(parents=True,exist_ok=True)
 with a.output.open("xb") as f:f.write(data)
if __name__=="__main__":main()
