/* Local reconstruction batch. Original JPEG decoder/cache and all game files stay intact. */
typedef struct {const char *name;const char *batch;BOOL wholeFrameCheck;WORD *reference;DWORD width,height;BOOL checked;ULONGLONG knownHash,referenceHash;} UIAsset;
static UIAsset uiAssets[]={{"CREDITS","batch01"},{"MAIN","batch01"},{"OLLEYS","batch01"},{"PANZER","batch01"},{"SPLASH","batch01"},{"STREET","batch01",TRUE},{"RESTROOM_REST1","batch02"},{"RESTROOM_REST2","batch02"},{"RESTROOM_REST3","batch02"},{"RESTROOM_REST4","batch02"},{"RESTROOM_REST5","batch02"},{"RESTROOM_REST6","batch02"},{"SHOWROOM_RAT_BIKES","batch02",TRUE},{"SHOWROOM_SPORT_BIKES","batch02",TRUE},{"SHOWROOM_SUPER_BIKES","batch02",TRUE},{"POSTCARD_CITY","batch03",TRUE},{"POSTCARD_KLUB","batch03",TRUE},{"POSTCARD_NAPA","batch03",TRUE},{"POSTCARD_PCH","batch03",TRUE},{"POSTCARD_PENIN","batch03",TRUE},{"POSTCARD_QCITY","batch03",TRUE},{"POSTCARD_QNAPA","batch03",TRUE},{"POSTCARD_QPCH","batch03",TRUE},{"POSTCARD_QPENIN","batch03",TRUE},{"POSTCARD_QSIERRA","batch03",TRUE},{"POSTCARD_QSIERRAR","batch03",TRUE},{"POSTCARD_RESTA","batch03",TRUE},{"POSTCARD_RESTB","batch03",TRUE},{"POSTCARD_SIERRA","batch03",TRUE},{"POSTCARD_SIERRAR","batch03",TRUE},{"STREET_HOME","batch03",TRUE},{"STREET_KLUB","batch03",TRUE},{"STREET_OLLEY","batch03",TRUE},{"CHARS_AXLE_N","batch04",TRUE},{"CHARS_BOSE_N","batch04",TRUE},{"CHARS_CYDNEY_N","batch04",TRUE},{"CHARS_JIM_N","batch04",TRUE},{"CHARS_JON_N","batch04",TRUE},{"CHARS_MIKE_N","batch04",TRUE},{"CHARS_PEARL_N","batch04",TRUE},{"CHARS_RHONDA_N","batch04",TRUE},{"RESULTS_LEVEL1","batch04",TRUE},{"RESULTS_LEVEL2","batch04",TRUE},{"RESULTS_LEVEL3","batch04",TRUE},{"RESULTS_LEVEL4","batch04",TRUE},{"RESULTS_WIN1","batch04",TRUE},{"RESULTS_WIN2","batch04",TRUE},{"RESULTS_WIN3","batch04",TRUE},{"RESULTS_WIN4","batch04",TRUE},{"RESULTS_WIN5","batch04",TRUE},{"RESULTS_WIN6","batch04",TRUE},{"CHARS_AXLE_F","batch05",TRUE},{"CHARS_AXLE_H","batch05",TRUE},{"CHARS_BOSE_F","batch05",TRUE},{"CHARS_BOSE_H","batch05",TRUE},{"CHARS_CYDNEY_F","batch05",TRUE},{"CHARS_CYDNEY_H","batch05",TRUE},{"CHARS_JIM_F","batch05",TRUE},{"CHARS_JIM_H","batch05",TRUE},{"CHARS_JON_F","batch05",TRUE},{"CHARS_JON_H","batch05",TRUE},{"CHARS_MIKE_F","batch05",TRUE},{"CHARS_MIKE_H","batch05",TRUE},{"CHARS_PEARL_F","batch05",TRUE},{"CHARS_PEARL_H","batch05",TRUE},{"CHARS_RHONDA_F","batch05",TRUE},{"CHARS_RHONDA_H","batch05",TRUE},{"RESULTS_BUST1","batch05",TRUE},{"RESULTS_BUST2","batch05",TRUE},{"RESULTS_BUST3","batch05",TRUE},{"RESULTS_BUST4","batch05",TRUE},{"RESULTS_BUST5","batch05",TRUE},{"RESULTS_BUST6","batch05",TRUE},{"RESULTS_FINALE","batch05",TRUE},{"RESULTS_LOSE1","batch05",TRUE},{"RESULTS_LOSE2","batch05",TRUE},{"RESULTS_LOSE3","batch05",TRUE},{"RESULTS_LOSE4","batch05",TRUE},{"RESULTS_LOSE5","batch05",TRUE},{"RESULTS_LOSE6","batch05",TRUE},{"RESULTS_WRECK1","batch05",TRUE},{"RESULTS_WRECK2","batch05",TRUE},{"RESULTS_WRECK3","batch05",TRUE},{"RESULTS_WRECK4","batch05",TRUE},{"RESULTS_WRECK5","batch05",TRUE},{"RESULTS_WRECK6","batch05",TRUE},{"BIKES_RAT_BANZAI","batch06",TRUE},{"BIKES_RAT_CORSAIR","batch06",TRUE},{"BIKES_RAT_KAMIKAZE","batch06",TRUE},{"BIKES_RAT_KILLER","batch06",TRUE},{"BIKES_RAT_PERRO","batch06",TRUE},{"BIKES_SPORT_DIABLO","batch06",TRUE},{"BIKES_SPORT_DMG","batch06",TRUE},{"BIKES_SPORT_KAMIKAZE","batch06",TRUE},{"BIKES_SPORT_PERRO","batch06",TRUE},{"BIKES_SPORT_STILETTO","batch06",TRUE},{"BIKES_SUPER_BANZAI","batch06",TRUE},{"BIKES_SUPER_CORSAIR","batch06",TRUE},{"BIKES_SUPER_DIABLO","batch06",TRUE},{"BIKES_SUPER_KAMIKAZE","batch06",TRUE},{"BIKES_SUPER_STILETTO","batch06",TRUE},{"SHOWROOM_RAT_BANZAI","batch06",TRUE},{"SHOWROOM_RAT_CORSAIR","batch06",TRUE},{"SHOWROOM_RAT_KAMIKAZE","batch06",TRUE},{"SHOWROOM_RAT_KILLER","batch06",TRUE},{"SHOWROOM_RAT_PERRO","batch06",TRUE},{"SHOWROOM_SPORT_DIABLO","batch06",TRUE},{"SHOWROOM_SPORT_DMG","batch06",TRUE},{"SHOWROOM_SPORT_KAMIKAZE","batch06",TRUE},{"SHOWROOM_SPORT_PERRO","batch06",TRUE},{"SHOWROOM_SPORT_STILETTO","batch06",TRUE},{"SHOWROOM_SUPER_BANZAI","batch06",TRUE},{"SHOWROOM_SUPER_CORSAIR","batch06",TRUE},{"SHOWROOM_SUPER_DIABLO","batch06",TRUE},{"SHOWROOM_SUPER_KAMIKAZE","batch06",TRUE},{"SHOWROOM_SUPER_STILETTO","batch06",TRUE}};
#define UI_ASSET_COUNT ((int)(sizeof(uiAssets)/sizeof(uiAssets[0])))
static WORD *uiAssetPixels;static int uiAssetLoaded=-1;
static BOOL ui_asset_off(void){char value[4]={0};return GetEnvironmentVariableA("RR_DISABLE_HD_ASSETS",value,sizeof(value))&&value[0]=='1';}
static HANDLE ui_asset_open(int index,DWORD header[6]){
 char path[MAX_PATH];if(lstrlenA(root)>MAX_PATH-64)return INVALID_HANDLE_VALUE;wsprintfA(path,"%s\\HD\\%s\\%s.rrhd",root,uiAssets[index].batch,uiAssets[index].name);
 HANDLE f=CreateFileA(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);if(f==INVALID_HANDLE_VALUE)return f;
 char magic[8];DWORD n;LARGE_INTEGER size;BOOL ok=ReadFile(f,magic,8,&n,NULL)&&n==8&&!memcmp(magic,"RRHDv001",8)&&ReadFile(f,header,24,&n,NULL)&&n==24&&GetFileSizeEx(f,&size);
 if(!ok||header[0]<640||header[0]>2880||header[1]<480||header[1]>2160||header[0]*3!=header[1]*4||header[2]!=640*480*2||header[3]!=(ULONGLONG)header[0]*header[1]*2||size.QuadPart!=32LL+header[2]+header[3]){CloseHandle(f);return INVALID_HANDLE_VALUE;}return f;
}
static ULONGLONG ui_asset_hash(const WORD *pixels);
static BOOL ui_asset_reference(int index){
 UIAsset *a=&uiAssets[index];if(a->checked)return a->reference!=NULL;a->checked=TRUE;DWORD h[6],n;HANDLE f=ui_asset_open(index,h);if(f==INVALID_HANDLE_VALUE)return FALSE;
 WORD *ref=HeapAlloc(GetProcessHeap(),0,h[2]);BOOL ok=ref&&ReadFile(f,ref,h[2],&n,NULL)&&n==h[2];CloseHandle(f);if(!ok){if(ref)HeapFree(GetProcessHeap(),0,ref);return FALSE;}
 a->reference=ref;a->referenceHash=ui_asset_hash(ref);a->width=h[0];a->height=h[1];return TRUE;
}
static int ui_asset_score(const WORD *image,const WORD *reference){
 int bad=0,sum=0;for(int y=4;y<480;y+=8)for(int x=4;x<640;x+=8){WORD p=image[y*640+x],q=reference[y*640+x];int max=0;for(int shift=0;shift<=10;shift+=5){int d=abs((int)((p>>shift)&31)-(int)((q>>shift)&31));sum+=d;if(d>max)max=d;}if(max>2&&++bad>48)return -1;}
 return sum<=4800*3?sum:-1;
}
static BOOL ui_asset_pixels(int index){
 if(uiAssetLoaded==index)return TRUE;DWORD h[6],n;HANDLE f=ui_asset_open(index,h);if(f==INVALID_HANDLE_VALUE)return FALSE;if(h[0]!=uiAssets[index].width||h[1]!=uiAssets[index].height){CloseHandle(f);return FALSE;}SetFilePointer(f,640*480*2,NULL,FILE_CURRENT);
 WORD *p=HeapAlloc(GetProcessHeap(),0,h[3]);BOOL ok=p&&ReadFile(f,p,h[3],&n,NULL)&&n==h[3];CloseHandle(f);if(!ok){if(p)HeapFree(GetProcessHeap(),0,p);return FALSE;}
 if(uiAssetPixels)HeapFree(GetProcessHeap(),0,uiAssetPixels);uiAssetPixels=p;uiAssetLoaded=index;return TRUE;
}
static ULONGLONG ui_asset_hash(const WORD *pixels){ULONGLONG h=14695981039346656037ULL;for(int i=0;i<640*480;i++){h^=pixels[i]&0x7fff;h*=1099511628211ULL;}return h;}
/* Composite variants differ by as little as 2.65%; character expressions and numbered results also require full verification. Sampling alone is insufficient.
   Every RGB555 channel must match within one quantization step; unknown/overlaid data falls back. */
static BOOL ui_asset_whole_frame(const WORD *image,const WORD *reference){
 for(int p=0;p<640*480;p++)for(int shift=0;shift<=10;shift+=5)if(abs((int)((image[p]>>shift)&31)-(int)((reference[p]>>shift)&31))>1)return FALSE;
 return TRUE;
}
static int ui_asset_composite_match(const WORD *image){
 int found=-1;for(int i=0;i<UI_ASSET_COUNT;i++)if(uiAssets[i].wholeFrameCheck&&ui_asset_reference(i)&&ui_asset_score(image,uiAssets[i].reference)>=0&&ui_asset_whole_frame(image,uiAssets[i].reference)){if(found>=0)return -2;found=i;}return found;
}
static WORD ui_565_to_555(WORD p){return (WORD)(((p>>1)&0x7fe0)|(p&31));}
static WORD ui_555_to_565(WORD p){return (WORD)(((p&0x7fe0)<<1)|(p&31)|((p&0x200)>>4));}
static BOOL ui_asset_apply_image(struct RRSurface *s,const WORD *image,BOOL rgb565){

 ULONGLONG hash=ui_asset_hash(image);int known=ui_asset_composite_match(image);if(known==-2)return FALSE;for(int i=0;known<0&&i<UI_ASSET_COUNT;i++)if(!uiAssets[i].wholeFrameCheck&&uiAssets[i].knownHash&&uiAssets[i].knownHash==hash)known=i;
 int changed=0;for(int i=0;i<640*480;i++)if(s->shadow[i]!=s->shadowBefore[i])changed++;if(known<0&&changed<640*480*9/10){
  /* Near-identical refreshes are safe only when the entire logical image equals a reference. */
  for(int i=0;i<UI_ASSET_COUNT;i++)if(!uiAssets[i].wholeFrameCheck&&ui_asset_reference(i)&&hash==uiAssets[i].referenceHash){BOOL exact=TRUE;for(int p=0;p<640*480;p++)if((image[p]^uiAssets[i].reference[p])&0x7fff){exact=FALSE;break;}if(exact){if(known>=0)return FALSE;known=i;}}
  if(known<0)return FALSE;
 }
 int best=known,bestScore=known<0?0x7fffffff:0,second=0x7fffffff;
 for(int i=0;known<0&&i<UI_ASSET_COUNT;i++)if(!uiAssets[i].wholeFrameCheck&&ui_asset_reference(i)){int score=ui_asset_score(image,uiAssets[i].reference);if(score>=0){if(score<bestScore){second=bestScore;bestScore=score;best=i;}else if(score<second)second=score;}}
 if(best<0||second-bestScore<4800)return FALSE;
 if(rgb565&&(!uiAssets[best].reference||!ui_asset_whole_frame(image,uiAssets[best].reference)))return FALSE;
 if(!ui_asset_pixels(best))return FALSE;
 uiAssets[best].knownHash=hash;UIAsset *a=&uiAssets[best];for(DWORD y=0;y<uiHeight;y++){WORD *to=(WORD*)((BYTE*)s->lockedBits+(SIZE_T)y*s->lockedPitch);const WORD *from=uiAssetPixels+(SIZE_T)(((ULONGLONG)(y*2+1)*a->height)/(uiHeight*2))*a->width;for(DWORD x=0;x<uiWidth;x++){WORD p=from[((ULONGLONG)(x*2+1)*a->width)/(uiWidth*2)];to[x]=rgb565?ui_555_to_565(p):p;}}
 char line[192];wsprintfA(line,"HD background applied: %s master=%ux%u target=%ux%u matchScore=%d",a->name,a->width,a->height,uiWidth,uiHeight,bestScore);trace(line);return TRUE;
}

/* Native menu surfaces are RGB565. Do not mutate the raw shadow or fallback writes. */
static BOOL ui_asset_apply(struct RRSurface *s){
 if(!s->imageUpload||s->partialLock||s->readOnly||(!s->rgb555&&!s->rgb565)||ui_asset_off())return FALSE;
 if(s->rgb555)return ui_asset_apply_image(s,s->shadow,FALSE);
 WORD *normalized=HeapAlloc(GetProcessHeap(),0,640*480*2);if(!normalized)return FALSE;
 for(int p=0;p<640*480;p++)normalized[p]=ui_565_to_555(s->shadow[p]);
 BOOL result=ui_asset_apply_image(s,normalized,TRUE);HeapFree(GetProcessHeap(),0,normalized);return result;
}
