import importlib.util,struct,sys
from pathlib import Path
from PIL import Image
root=Path(__file__).resolve().parents[1]
def load(name):
 spec=importlib.util.spec_from_file_location(name,root/"tools"/(name+".py"));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
pack=load("pack_sidecar");b=pack.pack(Image.new("RGB",(640,480),(255,0,0)),Image.new("RGB",(960,720),(0,255,0)));assert b[:8]==b"RRHDv001";assert struct.unpack_from("<6I",b,8)==(960,720,614400,1382400,0,0);assert struct.unpack_from("<H",b,32)[0]==0x7c00;assert struct.unpack_from("<H",b,32+614400)[0]==0x3e0
try:load("bootstrap").inject(b"not a game")
except ValueError:pass
else:raise AssertionError("Unknown EXE was accepted")
print("Synthetic sidecar encoding and unsupported EXE rejection: PASS")
