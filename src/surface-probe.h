/* Per-object DirectDraw vtables, confined to this isolated process. */
#include <ddraw.h>
typedef HRESULT (WINAPI *RRUnlock)(void*,void*);
typedef HRESULT (WINAPI *RRLock)(void*,RECT*,DDSURFACEDESC*,DWORD,HANDLE);
typedef HRESULT (WINAPI *RRCreateSurface)(void*,DDSURFACEDESC*,void**,IUnknown*);
typedef HRESULT (WINAPI *RRBlt)(void*,RECT*,void*,RECT*,DWORD,DDBLTFX*);
typedef HRESULT (WINAPI *RRBltFast)(void*,DWORD,DWORD,void*,RECT*,DWORD);
typedef HRESULT (WINAPI *RRGetDC)(void*,HDC*);
typedef HRESULT (WINAPI *RRReleaseDC)(void*,HDC);
static struct RRSurface {void *object;void **table;RRLock lock;RRBlt blt;RRBltFast fast;RRGetDC getDC;RRReleaseDC releaseDC;int savedDC;DWORD locks,lastTick,bpp;RRUnlock unlock;ULONG (WINAPI *release)(void*);BOOL menuHD,shadowActive,partialLock,readOnly,imageUpload,rgb555,rgb565;RECT lockRegion;void *virtualBits;WORD *shadow,*shadowBefore,*lockedBits;LONG lockedPitch;} rrSurfaces[128];
static int rrSurfaceCount;

typedef HRESULT (WINAPI *RRCoop)(void*,HWND,DWORD);
#include "draw-hooks.h"
static HRESULT WINAPI rr_coop(void *object,HWND window,DWORD flags){
 DWORD savedIncomingError=GetLastError();RRCoop originalCooperative=NULL;
 if(!rr_draw_originals(object,NULL,&originalCooperative)){SetLastError(savedIncomingError);return DDERR_INVALIDOBJECT;}
 rr_startup_phase("COOPERATIVE_LEVEL_BEGIN",S_OK);int mode=rr_startup_test_mode();BOOL injected=mode==1;
 if(mode==2){rr_startup_line("TEST_INJECTION intentional pre-frame exit");rr_startup_intentional();rr_startup_exit(0);ExitProcess(0);}
 if(mode==3){rr_startup_line("TEST_INJECTION nonzero pre-frame exit");rr_startup_exit(42);ExitProcess(42);}
 if(mode==4){rr_startup_line("TEST_INJECTION delayed startup");Sleep(1800);}
 {DWORD owner=0,foreground=0;DWORD thread=GetWindowThreadProcessId(window,&owner);GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
  char cls[64]={0},context[384];GetClassNameA(window,cls,64);LONG style=GetWindowLongA(window,GWL_STYLE);HWND rootWindow=GetAncestor(window,GA_ROOT);
  wsprintfA(context,"COOP_CONTEXT_BEFORE callerRVA=%x hwnd=%08x owner=%u ownerThread=%u callingThread=%u sameThread=%d root=%08x child=%d class=%.63s inputDesktop=%d foregroundOwner=%u visible=%d iconic=%d",(DWORD)((BYTE*)_ReturnAddress()-(BYTE*)GetModuleHandleW(NULL)),(DWORD)window,owner,thread,GetCurrentThreadId(),thread==GetCurrentThreadId(),(DWORD)rootWindow,(style&WS_CHILD)!=0,cls,rr_on_input_desktop(),foreground,IsWindowVisible(window),IsIconic(window));trace(context);rr_startup_line(context);
 }
 DWORD cooperativeStart=GetTickCount();
 SetLastError(savedIncomingError);
 HRESULT result=injected?E_FAIL:originalCooperative(object,window,flags);DWORD savedOutgoingError=GetLastError();char line[192];DWORD pid=0;GetWindowThreadProcessId(window,&pid);
 wsprintfA(line,"SetCooperativeLevel: hwnd=%08x valid=%d owner=%u ownPid=%u flags=%x result=%08x",(DWORD)window,IsWindow(window),pid,GetCurrentProcessId(),flags,result);trace(line);

 {char duration[96];wsprintfA(duration,"COOP_CALL_DURATION_MS %u",GetTickCount()-cooperativeStart);rr_startup_line(duration);}
 if(injected)rr_startup_line("TEST_INJECTION cooperative level failure");rr_startup_phase("COOPERATIVE_LEVEL_RESULT",result);SetLastError(savedOutgoingError);return result;
}

static DWORD nativePresentStarted;
static struct RRSurface *rr_surface(void *object){for(int i=rrSurfaceCount-1;i>=0;i--)if(rrSurfaces[i].object==object)return &rrSurfaces[i];return NULL;}
#include "frontend-hd.h"
#include "ui-coordinates.h"
static ULONG WINAPI rr_release_surface(void *object){
 struct RRSurface *s=rr_surface(object);if(!s)return 0;
 ULONG remaining=s->release(object);if(!remaining){
  if(s->shadow)HeapFree(GetProcessHeap(),0,s->shadow);
  if(s->shadowBefore)HeapFree(GetProcessHeap(),0,s->shadowBefore);
  if(s->table)HeapFree(GetProcessHeap(),0,s->table);ZeroMemory(s,sizeof(*s));
 }return remaining;
}
static BOOL rr_primary(void *object){struct RRSurface *s=rr_surface(object);return nativeProjection2x&&s&&s->bpp==8&&object==*(void**)((BYTE*)GetModuleHandleW(NULL)+0x1cd4bc);}
static HRESULT WINAPI rr_blt(void *object,RECT *dest,void *source,RECT *src,DWORD flags,DDBLTFX *fx){
 struct RRSurface *s=rr_surface(object);if(!s)return E_FAIL;
 RECT scaled;if(rr_primary(object)&&dest){scaled=*dest;scaled.left=MulDiv(scaled.left,nativeWidth,640);scaled.top=MulDiv(scaled.top,nativeHeight,480);scaled.right=MulDiv(scaled.right,nativeWidth,640);scaled.bottom=MulDiv(scaled.bottom,nativeHeight,480);dest=&scaled;}
 RECT sourceScaled;if(ui_hd(source)&&src){sourceScaled=ui_rect(*src);src=&sourceScaled;}if(s->menuHD&&dest){scaled=ui_rect(*dest);dest=&scaled;}
 return s->blt(object,dest,source,src,flags,fx);
}
static HRESULT WINAPI rr_fast(void *object,DWORD x,DWORD y,void *source,RECT *src,DWORD flags){
 struct RRSurface *s=rr_surface(object);if(!s)return E_FAIL;
 if((rr_primary(object)||s->menuHD)&&source){
  DDSURFACEDESC d={0};d.dwSize=sizeof(d);typedef HRESULT(WINAPI *Desc)(void*,DDSURFACEDESC*);
  if(!src&&FAILED(((Desc)(*(void***)source)[22])(source,&d)))return s->fast(object,x,y,source,src,flags);
  LONG w=src?src->right-src->left:(ui_hd(source)?640:d.dwWidth),h=src?src->bottom-src->top:(ui_hd(source)?480:d.dwHeight);DWORD dw=s->menuHD?uiWidth:nativeWidth,dh=s->menuHD?uiHeight:nativeHeight;
  RECT target={MulDiv((LONG)x,dw,640),MulDiv((LONG)y,dh,480),MulDiv((LONG)x+w,dw,640),MulDiv((LONG)y+h,dh,480)};DWORD mapped=DDBLT_WAIT;
  if(flags&DDBLTFAST_SRCCOLORKEY)mapped|=DDBLT_KEYSRC;if(flags&DDBLTFAST_DESTCOLORKEY)mapped|=DDBLT_KEYDEST;
  RECT sourceScaled;if(ui_hd(source)&&src){sourceScaled=ui_rect(*src);src=&sourceScaled;}return s->blt(object,&target,source,src,mapped,NULL);
 }
 return s->fast(object,x,y,source,src,flags);
}
static HRESULT WINAPI rr_get_dc(void *object,HDC *dc){struct RRSurface *s=rr_surface(object);if(!s)return E_FAIL;HRESULT r=s->getDC(object,dc);if(SUCCEEDED(r)&&(rr_primary(object)||s->menuHD)){s->savedDC=SaveDC(*dc);SetMapMode(*dc,MM_ANISOTROPIC);SetWindowExtEx(*dc,640,480,NULL);SetViewportExtEx(*dc,s->menuHD?uiWidth:nativeWidth,s->menuHD?uiHeight:nativeHeight,NULL);}return r;}
static HRESULT WINAPI rr_release_dc(void *object,HDC dc){struct RRSurface *s=rr_surface(object);if(!s)return E_FAIL;if(s->savedDC){RestoreDC(dc,s->savedDC);s->savedDC=0;}return s->releaseDC(object,dc);}
static void rr_race_frame(const BITMAPINFO *info,const void *bits) {
 BYTE *base=(BYTE*)GetModuleHandleW(NULL);DWORD state=*(DWORD*)(base+0x1c4044);
 if(!raceTestEnabled||state<1||state>5||!info||info->bmiHeader.biBitCount!=32)return;
 static DWORD first,last,frames,total,captures;DWORD now=GetTickCount();if(!first)first=last=now;frames++;total++;
 if(now-last>=5000){char line[192];wsprintfA(line,"Race present: frames=%u intervalMs=%u total=%u elapsed=%u bitmap=%dx%d",frames,now-last,total,now-first,info->bmiHeader.biWidth,info->bmiHeader.biHeight);trace(line);frames=0;last=now;}
 if(captures<4&&now-first>=(captures+1)*7000){
  captures++;char path[MAX_PATH],name[64];wsprintfA(name,"\\rr4k-race-frame-%u.bmp",captures);lstrcpyA(path,root);lstrcatA(path,name);
  BITMAPFILEHEADER file={0};BITMAPINFOHEADER header=info->bmiHeader;DWORD size=header.biWidth*abs(header.biHeight)*4,done;
  file.bfType=0x4d42;file.bfOffBits=sizeof(file)+sizeof(header);file.bfSize=file.bfOffBits+size;
  HANDLE h=CreateFileA(path,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);if(h!=INVALID_HANDLE_VALUE){WriteFile(h,&file,sizeof(file),&done,NULL);WriteFile(h,&header,sizeof(header),&done,NULL);WriteFile(h,bits,size,&done,NULL);CloseHandle(h);}
 }
}
static HRESULT WINAPI rr_lock(void *object,RECT *rect,DDSURFACEDESC *desc,DWORD flags,HANDLE event) {
 struct RRSurface *s=NULL;for(int i=rrSurfaceCount-1;i>=0;i--)if(rrSurfaces[i].object==object){s=&rrSurfaces[i];break;}
 if(!s)return E_FAIL;
 if(s->menuHD){
  if(s->shadowActive)return DDERR_SURFACEBUSY;
  if(rect&&(rect->left<0||rect->top<0||rect->right>640||rect->bottom>480||rect->left>=rect->right||rect->top>=rect->bottom))return DDERR_INVALIDRECT;
  s->partialLock=rect!=NULL;s->lockRegion=rect?*rect:(RECT){0,0,640,480};s->readOnly=(flags&DDLOCK_READONLY)!=0;
 }
 HRESULT result=s->lock(object,s->menuHD?NULL:rect,desc,flags,event);DWORD now=GetTickCount();s->locks++;
 DWORD caller=(DWORD)((BYTE*)_ReturnAddress()-(BYTE*)GetModuleHandleW(NULL));BYTE *base=(BYTE*)GetModuleHandleW(NULL);
 if(SUCCEEDED(result)&&caller==0x3a09a&&desc->ddpfPixelFormat.dwRGBBitCount==16&&(nativePresentStarted||!uiPresented)){
  typedef BYTE (__fastcall *InitPresent)(HWND,DWORD,DWORD);ui_coordinate_dimensions(640,480);nativePresentStarted=0;uiPresented=TRUE;((InitPresent)(base+0x6e390))(*(HWND*)(base+0x1ce858),s->menuHD?uiWidth:640,s->menuHD?uiHeight:480);trace(s->menuHD?"HD menu presentation initialized/restored":"Menu presentation restored to640x480 after race");
 }
 if(SUCCEEDED(result)&&s->menuHD&&caller!=0x3a09a){
  s->imageUpload=caller==0x36444;s->rgb555=desc->ddpfPixelFormat.dwRBitMask==0x7c00&&desc->ddpfPixelFormat.dwGBitMask==0x3e0&&desc->ddpfPixelFormat.dwBBitMask==0x1f;s->rgb565=desc->ddpfPixelFormat.dwRBitMask==0xf800&&desc->ddpfPixelFormat.dwGBitMask==0x7e0&&desc->ddpfPixelFormat.dwBBitMask==0x1f;
  if(desc->dwWidth<uiWidth||desc->dwHeight<uiHeight||desc->lPitch<(LONG)(uiWidth*2)){s->unlock(object,desc->lpSurface);return DDERR_INVALIDPARAMS;}
  if(!ui_shadow_lock(s,desc)){s->unlock(object,desc->lpSurface);return DDERR_OUTOFMEMORY;}
  s->virtualBits=s->shadow+(SIZE_T)s->lockRegion.top*640+s->lockRegion.left;desc->lpSurface=s->virtualBits;
 }
 if(SUCCEEDED(result)&&caller==0x67c21&&nativeWidth&&!nativePresentStarted){
  typedef BYTE (__fastcall *InitPresent)(HWND,DWORD,DWORD);
  ui_coordinate_dimensions(nativeWidth,nativeHeight);uiPresented=FALSE;nativePresentStarted=now;((InitPresent)(base+0x6e390))(*(HWND*)(base+0x1ce858),nativeWidth,nativeHeight);trace("Native race presentation initialized after first rendered frame");
 }
 if(SUCCEEDED(result)&&(s->locks==1||now-s->lastTick>=5000)){
  char line[256];wsprintfA(line,"Surface lock: object=%08x callerRVA=%x size=%ux%u pitch=%d bpp=%u bits=%08x caps=%x locks=%u elapsed=%u",(DWORD)object,caller,desc->dwWidth,desc->dwHeight,desc->lPitch,desc->ddpfPixelFormat.dwRGBBitCount,(DWORD)desc->lpSurface,desc->ddsCaps.dwCaps,s->locks,now-raceStartTick);trace(line);s->lastTick=now;
  if(desc->ddpfPixelFormat.dwRGBBitCount==8){wsprintfA(line,"Raster globals: viewport=%d,%d,%d,%d pitch=%d base=%08x cameraBase=%dx%d",*(int*)(base+0x2036c0),*(int*)(base+0x2036c4),*(int*)(base+0x2036c8),*(int*)(base+0x2036cc),*(int*)(base+0x2036b8),*(DWORD*)(base+0x2036bc),*(int*)(base+0x191c04),*(int*)(base+0x191c08));trace(line);}
 }
 return result;
}
static HRESULT WINAPI rr_create_surface(void *object,DDSURFACEDESC *desc,void **out,IUnknown *outer) {
 DWORD incoming=GetLastError();RRCreateSurface originalCreate=NULL;
 if(!rr_draw_originals(object,&originalCreate,NULL)){SetLastError(incoming);return DDERR_INVALIDOBJECT;}
 if(!desc||!out){SetLastError(incoming);return DDERR_INVALIDPARAMS;}
 DWORD caller=(DWORD)((BYTE*)_ReturnAddress()-(BYTE*)GetModuleHandleW(NULL));
 if(nativeWidth&&caller==0x5f93b){BYTE *base=(BYTE*)GetModuleHandleW(NULL);desc->dwWidth=nativeWidth;desc->dwHeight=nativeHeight;*(DWORD*)(base+0x1a2934)=nativeProjection2x?640:nativeWidth;*(DWORD*)(base+0x1a2938)=nativeProjection2x?480:nativeHeight;if(!nativeProjection2x){*(DWORD*)(base+0x191c04)=nativeWidth/2;*(DWORD*)(base+0x191c08)=nativeHeight/2;*(DWORD*)(base+0x191c0c)=nativeWidth/2;}*(DWORD*)(base+0x2036c8)=nativeWidth;*(DWORD*)(base+0x2036cc)=nativeHeight-1;trace("Native race surface and base camera dimensions requested");}
 BOOL menuHD=uiEnabled&&desc->dwWidth==640&&desc->dwHeight==480&&desc->ddpfPixelFormat.dwRGBBitCount==16&&(caller==0x5fc4a||caller==0x5fc8e||caller==0x5fcae||caller==0x5fcce);
 if(menuHD){desc->dwWidth=uiWidth;desc->dwHeight=uiHeight;uiPresented=FALSE;}
 SetLastError(incoming);HRESULT result=originalCreate(object,desc,out,outer);DWORD outgoing=GetLastError();char line[256];
 wsprintfA(line,"CreateSurface: callerRVA=%x requested=%ux%u bpp=%u caps=%x result=%08x object=%08x",(DWORD)((BYTE*)_ReturnAddress()-(BYTE*)GetModuleHandleW(NULL)),desc->dwWidth,desc->dwHeight,desc->ddpfPixelFormat.dwRGBBitCount,desc->ddsCaps.dwCaps,result,SUCCEEDED(result)?(DWORD)*out:0);trace(line);
 int slot=0;while(slot<rrSurfaceCount&&rrSurfaces[slot].object)slot++;
 if(SUCCEEDED(result)&&slot<128){
  if(slot==rrSurfaceCount)rrSurfaceCount++;struct RRSurface *s=&rrSurfaces[slot];s->object=*out;s->menuHD=menuHD;s->bpp=desc->ddpfPixelFormat.dwRGBBitCount;s->table=HeapAlloc(GetProcessHeap(),0,36*sizeof(void*));
  if(s->table){memcpy(s->table,*(void***)*out,36*sizeof(void*));s->release=(ULONG(WINAPI*)(void*))s->table[2];s->table[2]=rr_release_surface;s->unlock=(RRUnlock)s->table[32];s->table[32]=ui_unlock;s->lock=(RRLock)s->table[25];s->table[25]=rr_lock;s->blt=(RRBlt)s->table[5];s->fast=(RRBltFast)s->table[7];s->getDC=(RRGetDC)s->table[17];s->releaseDC=(RRReleaseDC)s->table[26];s->table[5]=rr_blt;s->table[7]=rr_fast;s->table[17]=rr_get_dc;s->table[26]=rr_release_dc;*(void***)*out=s->table;}
 }
 if(SUCCEEDED(result)&&(slot>=128||!rrSurfaces[slot].table)){
  void *surface=*out;ULONG(WINAPI *release)(void*)=(ULONG(WINAPI*)(void*))(*(void***)surface)[2];release(surface);*out=NULL;
  if(slot<128)ZeroMemory(&rrSurfaces[slot],sizeof(rrSurfaces[slot]));result=DDERR_OUTOFMEMORY;trace("Surface compatibility storage unavailable; refusing untracked surface");
 }
 if(menuHD){desc->dwWidth=640;desc->dwHeight=480;}SetLastError(outgoing);return result;
}





