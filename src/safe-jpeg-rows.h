/* Replace the fixed-stride loop, not the JPEG decoder. Context layout and
   continuation RVAs are verified for build26105967 only. */
static void *jpeg_resume;
typedef UINT (__fastcall *JPEG_ROWS)(void*,void*,UINT);
static void __cdecl safe_jpeg_rows(BYTE *ctx,WORD *target,const char *name) {
 BYTE *base=(BYTE*)GetModuleHandleW(NULL);jpeg_resume=base+0x35c5d;
 DWORD width=*(DWORD*)(ctx+0x18),height=*(DWORD*)(ctx+0x1c);
 if(!target||!width||!height||width>8192||height>8192||(ULONGLONG)width*height>16777216){trace("JPEG dimensions rejected safely");return;}
 SIZE_T bytes=(SIZE_T)width*height*2;
 WORD *decoded=(WORD*)HeapAlloc(GetProcessHeap(),0,bytes);if(!decoded){trace("JPEG buffer allocation failed safely");return;}
 JPEG_ROWS read=(JPEG_ROWS)(base+0x5b4b);
 for(DWORD y=0;y<height;y++){
  WORD *row=decoded+(SIZE_T)y*width;
  if(read(ctx,&row,1)!=1){trace("JPEG scanline read failed safely");HeapFree(GetProcessHeap(),0,decoded);return;}
 }
 if(width<=640&&height<=480){
  int x=0,y=height==480?0:(height==250?230:(480-height)/2);
  if(width!=640&&width!=243)x=width==397?243:(width==258?146:(640-width)/2);
  if(x+(int)width>640||y+(int)height>480){HeapFree(GetProcessHeap(),0,decoded);return;}
  for(DWORD row=0;row<height;row++)memcpy(target+(y+row)*640+x,decoded+(SIZE_T)row*width,width*2);
 }else{
  /* Compatibility image for old gameplay/UI code; HD master is kept separately. */
  for(DWORD y=0;y<480;y++){const WORD *src=decoded+(SIZE_T)(y*height/480)*width;for(DWORD x=0;x<640;x++)target[y*640+x]=src[x*width/640];}
  const char *tail=strrchr(name,'\\'),*slash=strrchr(name,'/');if(slash&&(!tail||slash>tail))tail=slash;tail=tail?tail+1:name;
  if(!_stricmp(tail,"CREDITS.RRI")||!_stricmp(tail,"CREDITS"))hd_set_rgb555(decoded,width,height);
  char line[192];wsprintfA(line,"Safe JPEG decode: %dx%d to independent %u-byte buffer, legacy cache640x480",width,height,(DWORD)bytes);trace(line);
 }
 HeapFree(GetProcessHeap(),0,decoded);jpeg_resume=base+0x35c52;
}
__declspec(naked) static void jpeg_loop_hook(void) {
 __asm {
  pushfd
  pushad
  push esi
  push dword ptr [ebp-234h]
  lea eax,[ebp-230h]
  push eax
  call safe_jpeg_rows
  add esp,12
  popad
  popfd
  jmp dword ptr [jpeg_resume]
 }
}
static void install_safe_jpeg_rows(void) {
 BYTE *where=(BYTE*)GetModuleHandleW(NULL)+0x35bcd;
 const BYTE expected[6]={0x8b,0xbd,0xec,0xfd,0xff,0xff};
 if(memcmp(where,expected,6)){trace("Safe JPEG hook refused: unsupported code bytes");return;}
 DWORD old;if(!VirtualProtect(where,6,PAGE_EXECUTE_READWRITE,&old))return;
 where[0]=0xe9;*(LONG*)(where+1)=(BYTE*)jpeg_loop_hook-(where+5);where[5]=0x90;
 VirtualProtect(where,6,old,&old);FlushInstructionCache(GetCurrentProcess(),where,6);trace("Safe JPEG scanline hook installed");
}
