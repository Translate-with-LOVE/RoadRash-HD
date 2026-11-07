#include <windows.h>
#include <ddraw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static char root[MAX_PATH];
static DWORD nativeWidth=2880,nativeHeight=2160,raceStartTick;static BOOL nativeProjection2x=TRUE,raceTestEnabled=FALSE;
static DWORD replayCaller;static int hits,checks,failures;
static void trace(const char *s){if(!strncmp(s,"HD background applied:",22))hits++;}
static void rr_startup_phase(const char *p,HRESULT h){}static int rr_startup_test_mode(void){return 0;}static void rr_startup_line(const char *p){}static void rr_startup_intentional(void){}static void rr_startup_exit(UINT code){}static BOOL rr_on_input_desktop(void){return FALSE;}
#define _ReturnAddress() ((BYTE*)GetModuleHandleW(NULL)+replayCaller)
#include "../src/surface-probe.h"
#define VERSION "v20"
#define CHECK(x) do{checks++;if(!(x)){failures++;printf("FAIL line=%d %s\n",__LINE__,#x);}}while(0)
struct FakeSurface{void **table;DDSURFACEDESC d;BYTE *pixels;LONG pitch;};
static ULONG WINAPI fake_release(void *o){struct FakeSurface *s=o;HeapFree(GetProcessHeap(),0,s->pixels);HeapFree(GetProcessHeap(),0,s);return 0;}
static HRESULT WINAPI fake_lock(void *o,RECT *r,DDSURFACEDESC *d,DWORD flags,HANDLE event){struct FakeSurface *s=o;*d=s->d;d->lpSurface=s->pixels;d->lPitch=s->pitch;return DD_OK;}
static HRESULT WINAPI fake_unlock(void *o,void *b){struct FakeSurface *s=o;return !b||b==s->pixels?DD_OK:DDERR_INVALIDPARAMS;}
static void *surfaceTable[36];
static HRESULT WINAPI fake_create(void *o,DDSURFACEDESC *d,void **out,IUnknown *u){struct FakeSurface *s=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*s));if(!s)return E_OUTOFMEMORY;s->d=*d;s->pitch=d->dwWidth*2+16;s->pixels=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(SIZE_T)s->pitch*d->dwHeight+64);if(!s->pixels){HeapFree(GetProcessHeap(),0,s);return E_OUTOFMEMORY;}s->table=surfaceTable;for(DWORD y=0;y<d->dwHeight;y++)memset(s->pixels+(SIZE_T)y*s->pitch+d->dwWidth*2,0xa5,16);memset(s->pixels+(SIZE_T)s->pitch*d->dwHeight,0xa5,64);*out=s;return DD_OK;}
static ULONG WINAPI fake_draw_release(void *o){return 1;}static HRESULT WINAPI fake_coop(void *o,HWND h,DWORD f){return DD_OK;}
static WORD to565(WORD p){return (WORD)(((p&0x7fe0)<<1)|(p&31)|((p&0x200)>>4));}
static void frame(const char *name,struct FakeSurface *s){}
static void exercise(void *draw,int asset,int width,int height,BOOL fmt555,BOOL capture,BOOL overlay){
 uiWidth=width;uiHeight=height;uiEnabled=TRUE;DDSURFACEDESC d={0};d.dwSize=sizeof(d);d.dwFlags=0x1007;d.dwWidth=640;d.dwHeight=480;d.ddsCaps.dwCaps=0x840;d.ddpfPixelFormat.dwSize=32;d.ddpfPixelFormat.dwFlags=DDPF_RGB;d.ddpfPixelFormat.dwRGBBitCount=16;d.ddpfPixelFormat.dwRBitMask=fmt555?0x7c00:0xf800;d.ddpfPixelFormat.dwGBitMask=fmt555?0x3e0:0x7e0;d.ddpfPixelFormat.dwBBitMask=0x1f;
 void *object=NULL;replayCaller=0x5fcae;CHECK(SUCCEEDED(rr_create_surface(draw,&d,&object,NULL)));CHECK(d.dwWidth==640&&d.dwHeight==480);if(!object)return;struct FakeSurface *surface=object;CHECK(surface->d.dwWidth==uiWidth&&surface->d.dwHeight==uiHeight);struct RRSurface *s=rr_surface(object);CHECK(s&&s->menuHD);
 DDSURFACEDESC lock={0};lock.dwSize=sizeof(lock);replayCaller=0x36444;CHECK(SUCCEEDED(rr_lock(object,NULL,&lock,DDLOCK_WAIT,NULL)));CHECK(lock.dwWidth==640&&lock.dwHeight==480&&lock.lPitch==1280);WORD *target=lock.lpSurface;for(int p=0;p<640*480;p++)target[p]=fmt555?uiAssets[asset].reference[p]:to565(uiAssets[asset].reference[p]);if(overlay)target[103*640+101]^=0x4000;int before=hits;CHECK(SUCCEEDED(ui_unlock(object,lock.lpSurface)));
 CHECK(hits-before==(!overlay?1:0));
 BOOL expectHd;
 expectHd=!overlay;
 for(DWORD y=0;y<uiHeight;y+=7)for(DWORD x=0;x<uiWidth;x+=11){WORD expected;if(expectHd){DWORD sy=(DWORD)(((ULONGLONG)(y*2+1)*uiAssets[asset].height)/(uiHeight*2));DWORD sx=(DWORD)(((ULONGLONG)(x*2+1)*uiAssets[asset].width)/(uiWidth*2));expected=uiAssetPixels[(SIZE_T)sy*uiAssets[asset].width+sx];if(!fmt555)expected=to565(expected);}else{int sy=y*480/uiHeight,sx=x*640/uiWidth;expected=fmt555?uiAssets[asset].reference[sy*640+sx]:to565(uiAssets[asset].reference[sy*640+sx]);if(overlay&&sy==103&&sx==101)expected^=0x4000;}CHECK(*(WORD*)(surface->pixels+(SIZE_T)y*surface->pitch+x*2)==expected);}
 for(DWORD y=0;y<uiHeight;y++)for(int z=0;z<16;z++)CHECK(surface->pixels[(SIZE_T)y*surface->pitch+uiWidth*2+z]==0xa5);for(int z=0;z<64;z++)CHECK(surface->pixels[(SIZE_T)surface->pitch*uiHeight+z]==0xa5);
 if(capture)frame(uiAssets[asset].name,surface);CHECK(rr_release_surface(object)==0);
}
static void synthetic(void){char path[MAX_PATH];wsprintfA(path,"%s\\HD",root);CHECK(CreateDirectoryA(path,NULL));wsprintfA(path,"%s\\HD\\batch01",root);CHECK(CreateDirectoryA(path,NULL));wsprintfA(path,"%s\\HD\\batch01\\CREDITS.rrhd",root);FILE *f=NULL;CHECK(fopen_s(&f,path,"wb")==0&&f);if(!f)return;DWORD header[6]={960,720,640*480*2,960*720*2,0,0};CHECK(fwrite("RRHDv001",1,8,f)==8);CHECK(fwrite(header,4,6,f)==6);for(int y=0;y<480;y++)for(int x=0;x<640;x++){WORD p=((x/8+y/8)&1)?0x5ad6:0x219c;CHECK(fwrite(&p,2,1,f)==1);}for(int y=0;y<720;y++)for(int x=0;x<960;x++){WORD p=(WORD)(((x%32)<<10)|((y%32)<<5)|((x+y)%32));CHECK(fwrite(&p,2,1,f)==1);}fclose(f);}
int main(int argc,char **argv){SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);CHECK(argc==2);if(argc!=2)return 2;CHECK(GetFullPathNameA(argv[1],MAX_PATH,root,NULL)>0);CHECK(CreateDirectoryA(root,NULL));synthetic();surfaceTable[2]=fake_release;surfaceTable[25]=fake_lock;surfaceTable[32]=fake_unlock;void *drawTable[23]={0};drawTable[2]=fake_draw_release;drawTable[6]=fake_create;drawTable[20]=fake_coop;void **draw=drawTable;CHECK(SUCCEEDED(rr_probe_draw(&draw)));CHECK(ui_asset_reference(0));for(int p=0;p<32768;p++)CHECK(ui_565_to_555(ui_555_to_565((WORD)p))==p);int sizes[][2]={{640,480},{960,720},{1440,1080},{1920,1440},{2880,2160}};for(int i=0;i<5;i++){exercise(&draw,0,sizes[i][0],sizes[i][1],FALSE,FALSE,FALSE);exercise(&draw,0,sizes[i][0],sizes[i][1],TRUE,FALSE,FALSE);}exercise(&draw,0,960,720,FALSE,FALSE,TRUE);printf("synthetic pixel-format replay: checks=%d failures=%d; five targets RGB555/RGB565, overlay preservation; no game/window/input/audio\n",checks,failures);return failures?1:0;}
