/* High-resolution frontend surfaces. Input/layout remains logical640x480. */
static DWORD uiWidth=640,uiHeight=480;static BOOL uiEnabled,uiPresented;
static void ui_init(void){
 char file[MAX_PATH];lstrcpyA(file,root);lstrcatA(file,"\\rr4k-native-test.ini");uiEnabled=GetPrivateProfileIntA("render","frontend_hd",1,file)!=0&&nativeWidth!=0;
 char disabled[4]={0};if(GetEnvironmentVariableA("RR_DISABLE_FRONTEND_HD",disabled,sizeof(disabled))&&disabled[0]=='1')uiEnabled=FALSE;
 if(uiEnabled){uiHeight=nativeHeight;uiWidth=nativeWidth;}
 char line[128];wsprintfA(line,"Frontend render target: %ux%u enabled=%d (logical640x480)",uiWidth,uiHeight,uiEnabled);trace(line);
}
static BOOL ui_hd(void *object){struct RRSurface *s=rr_surface(object);return s&&s->menuHD;}
static RECT ui_rect(RECT r){r.left=MulDiv(r.left,uiWidth,640);r.right=MulDiv(r.right,uiWidth,640);r.top=MulDiv(r.top,uiHeight,480);r.bottom=MulDiv(r.bottom,uiHeight,480);return r;}
static BOOL ui_shadow_lock(struct RRSurface *s,DDSURFACEDESC *desc){
 if(!s->shadow)s->shadow=HeapAlloc(GetProcessHeap(),0,640*480*2);
 if(!s->shadowBefore)s->shadowBefore=HeapAlloc(GetProcessHeap(),0,640*480*2);
 if(!s->shadow||!s->shadowBefore)return FALSE;
 s->lockedBits=desc->lpSurface;s->lockedPitch=desc->lPitch;s->shadowActive=TRUE;
 for(int y=0;y<480;y++){WORD *from=(WORD*)((BYTE*)s->lockedBits+(SIZE_T)((y*uiHeight+479)/480)*s->lockedPitch);for(int x=0;x<640;x++)s->shadow[y*640+x]=from[(x*uiWidth+639)/640];}
 memcpy(s->shadowBefore,s->shadow,640*480*2);
 desc->lpSurface=s->shadow;desc->lPitch=1280;desc->dwWidth=640;desc->dwHeight=480;return TRUE;
}
#include "hd-backgrounds.h"
static HRESULT WINAPI ui_unlock(void *object,void *bits){
 struct RRSurface *s=rr_surface(object);if(!s)return E_FAIL;
 if(!s->shadowActive)return s->unlock(object,bits);
 if(bits&&bits!=s->virtualBits&&bits!=s->shadow)return DDERR_INVALIDPARAMS;
 if(!s->readOnly&&!ui_asset_apply(s)){
  RECT region=s->partialLock?s->lockRegion:(RECT){0,0,640,480};
  for(LONG y=region.top;y<region.bottom;y++)for(LONG x=region.left;x<region.right;x++)if(s->shadow[y*640+x]!=s->shadowBefore[y*640+x]){
   DWORD top=(y*uiHeight+479)/480,bottom=((y+1)*uiHeight+479)/480;
   DWORD left=(x*uiWidth+639)/640,right=((x+1)*uiWidth+639)/640;
   for(DWORD row=top;row<bottom;row++){WORD *to=(WORD*)((BYTE*)s->lockedBits+(SIZE_T)row*s->lockedPitch);for(DWORD col=left;col<right;col++)to[col]=s->shadow[y*640+x];}
  }
 }
 HRESULT result=s->unlock(object,bits?s->lockedBits:NULL);if(SUCCEEDED(result)){s->shadowActive=FALSE;s->virtualBits=NULL;}return result;
}
