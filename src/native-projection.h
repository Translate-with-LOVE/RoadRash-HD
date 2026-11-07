/* Convert logical 16.16 quad positions directly to the target raster grid.
   This runs before scan conversion, never on an already rendered image. */
static void *rr_quad_resume1,*rr_quad_resume2;
static void __cdecl rr_quad_pixels(int *out,const int *source) {
 for(int i=0;i<8;i++){
  LONGLONG n=(LONGLONG)source[i]*(i&1?nativeHeight:nativeWidth);
  LONGLONG d=(LONGLONG)(i&1?480:640)*32768;
  if(n<0)n-=d-1;out[i]=(int)(n/d);
 }
}
__declspec(naked) static void rr_quad1(void){__asm{
 mov edi,ecx
 pushfd
 pushad
 lea eax,[ebx+44h]
 push eax
 lea eax,[edi+18h]
 push eax
 call rr_quad_pixels
 add esp,8
 popad
 popfd
 jmp dword ptr [rr_quad_resume1]
}}
__declspec(naked) static void rr_quad2(void){__asm{
 pushfd
 pushad
 lea edx,[eax+44h]
 push edx
 lea edx,[eax+80h]
 push edx
 call rr_quad_pixels
 add esp,8
 popad
 popfd
 jmp dword ptr [rr_quad_resume2]
}}
static BOOL rr_quad_patch(DWORD rva,const BYTE *expected,void *hook){
 BYTE *p=(BYTE*)GetModuleHandleW(NULL)+rva;DWORD old;if(memcmp(p,expected,5)||!VirtualProtect(p,5,PAGE_EXECUTE_READWRITE,&old))return FALSE;
 p[0]=0xe9;*(LONG*)(p+1)=(BYTE*)hook-(p+5);VirtualProtect(p,5,old,&old);FlushInstructionCache(GetCurrentProcess(),p,5);return TRUE;
}
static void rr_native_projection(void){
 if(!nativeRational)return;BYTE *base=(BYTE*)GetModuleHandleW(NULL);
 const BYTE first[5]={0x8b,0x43,0x44,0x8b,0xf9},second[5]={0x8b,0x50,0x44,0xd3,0xfa};
 rr_quad_resume1=base+0x22693;rr_quad_resume2=base+0x228b0;
 BOOL a=rr_quad_patch(0x2263b,first,rr_quad1),b=rr_quad_patch(0x22848,second,rr_quad2);
 trace(a&&b?"Native rational quad projection installed before rasterization":"Native rational quad projection rejected code bytes");
}
