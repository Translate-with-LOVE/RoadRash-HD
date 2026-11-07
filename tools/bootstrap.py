"""Add the HD bootstrap import to a user-supplied supported PE32 copy."""
import argparse,hashlib,struct
from pathlib import Path
KNOWN={"9E40E25737149AB095061B7154797A35495FAEA97644FAFC5A1E616F1400C968","EA74AC8F96F6F7451973B76B087E162517E33BF93BC1FDE69B8E97D128B0A7E2"}
def inject(data):
 if hashlib.sha256(data).hexdigest().upper() not in KNOWN:raise ValueError("Unsupported EXE; preserve it and use a verified build 26105967 input")
 b=bytearray(data);pe=struct.unpack_from("<I",b,60)[0];opt=pe+24;n=struct.unpack_from("<H",b,pe+6)[0];sh=opt+struct.unpack_from("<H",b,pe+20)[0]
 if b[pe:pe+4]!=b"PE\0\0" or struct.unpack_from("<H",b,opt)[0]!=0x10b:raise ValueError("Expected PE32")
 sections=[struct.unpack_from("<4I",b,sh+i*40+8) for i in range(n)]
 if sh+(n+1)*40>struct.unpack_from("<I",b,opt+60)[0]:raise ValueError("No section-header space")
 def offset(rva):
  for vs,va,rs,rp in sections:
   if va<=rva<va+rs:return rp+rva-va
  raise ValueError("Invalid RVA")
 align=lambda x,a:(x+a-1)//a*a
 sa,fa=struct.unpack_from("<II",b,opt+32);va=align(max(v+max(vs,rs) for vs,v,rs,rp in sections),sa);raw=align(len(b),fa)
 begin=offset(struct.unpack_from("<I",b,opt+104)[0]);end=begin
 while any(struct.unpack_from("<5I",b,end)):end+=20
 old=end-begin;block=bytearray(4096);block[:old]=b[begin:end];cursor=align(old+40,4);dll=cursor;name=b"rr4k-isolation.dll\0";block[cursor:cursor+len(name)]=name;cursor+=len(name);cursor=align(cursor,2);symbol=cursor;name=b"\0\0RR4KBootstrap\0";block[cursor:cursor+len(name)]=name;cursor=align(cursor+len(name),4);ilt=cursor;struct.pack_into("<I",block,cursor,va+symbol);cursor+=8;iat=cursor;struct.pack_into("<I",block,cursor,va+symbol);cursor+=8;struct.pack_into("<5I",block,old,va+ilt,0,0,va+dll,va+iat)
 out=bytearray(raw+align(cursor,fa));out[:len(b)]=b;out[raw:raw+align(cursor,fa)]=block[:align(cursor,fa)];q=sh+n*40;struct.pack_into("<8sIIIIIIHHI",out,q,b".rrhd\0\0\0",cursor,va,align(cursor,fa),raw,0,0,0,0,0xc0000040);struct.pack_into("<H",out,pe+6,n+1);struct.pack_into("<I",out,opt+56,align(va+cursor,sa));struct.pack_into("<II",out,opt+104,va,old+40);struct.pack_into("<II",out,opt+128,0,0);struct.pack_into("<I",out,opt+64,0);return bytes(out)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument("--input",type=Path,required=True);p.add_argument("--output",type=Path,required=True);a=p.parse_args()
 if a.input.resolve()==a.output.resolve() or a.output.exists():raise ValueError("Use a new output path; source and existing files are never overwritten")
 out=inject(a.input.read_bytes());a.output.parent.mkdir(parents=True,exist_ok=True)
 with a.output.open("xb") as f:f.write(out)
 print("Created supported local EXE copy:",a.output)
if __name__=="__main__":main()
