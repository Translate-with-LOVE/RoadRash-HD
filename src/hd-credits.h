/* Narrow native-resolution layer experiment. Original JPEG cache is unchanged. */
typedef struct { char text[1024]; RECT rect; LOGFONTA font; COLORREF color; UINT flags; DRAWTEXTPARAMS params; BOOL hasParams; } HDText;
static HDText hdPending[256],hdLast[256];
static int hdPendingCount,hdLastCount,hdCreditsActive;
static HBITMAP hdBitmap;
static DIBSECTION hdDib;
static DWORD *hdNativeBits;
static BITMAPINFO hdNativeInfo;
static BOOL hd_set_rgb555(const WORD *pixels,int width,int height) {
 DWORD *next=(DWORD*)HeapAlloc(GetProcessHeap(),0,(SIZE_T)width*height*4);if(!next)return FALSE;
 for(int i=0;i<width*height;i++){WORD v=pixels[i];DWORD r=(v>>10)&31,g=(v>>5)&31,b=v&31;next[i]=((r*255/31)<<16)|((g*255/31)<<8)|(b*255/31);}
 if(hdNativeBits)HeapFree(GetProcessHeap(),0,hdNativeBits);hdNativeBits=next;
 ZeroMemory(&hdNativeInfo,sizeof(hdNativeInfo));hdNativeInfo.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);hdNativeInfo.bmiHeader.biWidth=width;hdNativeInfo.bmiHeader.biHeight=-height;hdNativeInfo.bmiHeader.biPlanes=1;hdNativeInfo.bmiHeader.biBitCount=32;
 char line[128];wsprintfA(line,"Native JPEG credits master: %dx%d, independent decode buffer",width,height);trace(line);return TRUE;
}
static struct { LOGFONTA spec; HFONT handle; } hdFonts[16];
static int hdFontCount,hdRenderLogged;
static void hd_on_file(LPCWSTR path) {
 if(!path)return;const WCHAR *tail=wcsrchr(path,L'\\'),*forward=wcsrchr(path,L'/');if(forward&&(!tail||forward>tail))tail=forward;tail=tail?tail+1:path;
 if(!lstrcmpiW(tail,L"CREDITS.RRI")){hdCreditsActive=1;return;}
 int n=lstrlenW(tail);
 if(n>4&&!lstrcmpiW(tail+n-4,L".RRI")){hdCreditsActive=0;hdPendingCount=hdLastCount=0;}
}
static BOOL hd_load(void) {
 if(hdNativeBits)return TRUE;
 if(hdBitmap)return TRUE;
 WCHAR file[MAX_PATH];if(!MultiByteToWideChar(CP_ACP,0,root,-1,file,MAX_PATH))return FALSE;
 if(lstrlenW(file)>MAX_PATH-32)return FALSE;
 lstrcatW(file,L"\\HD\\CREDITS.bmp");
 if(GetFileAttributesW(file)==INVALID_FILE_ATTRIBUTES)return FALSE;
 hdBitmap=(HBITMAP)LoadImageW(NULL,file,IMAGE_BITMAP,0,0,LR_LOADFROMFILE|LR_CREATEDIBSECTION);
 if(!hdBitmap)return FALSE;
 if(GetObject(hdBitmap,sizeof(hdDib),&hdDib)!=sizeof(hdDib)||!hdDib.dsBm.bmBits||hdDib.dsBm.bmWidth<640||hdDib.dsBm.bmWidth>8192||hdDib.dsBm.bmHeight<480||hdDib.dsBm.bmHeight>8192){DeleteObject(hdBitmap);hdBitmap=NULL;return FALSE;}
 char line[128];wsprintfA(line,"HD credits asset loaded: %dx%d bpp=%d",hdDib.dsBm.bmWidth,hdDib.dsBm.bmHeight,hdDib.dsBm.bmBitsPixel);trace(line);return TRUE;
}
static BOOL hd_text(HDC dc,LPSTR text,int len,LPRECT rect,UINT flags,LPDRAWTEXTPARAMS params,int *result) {
 if(!hdCreditsActive||flags&DT_CALCRECT||flags&DT_MODIFYSTRING||!hd_load())return FALSE;
 BITMAP bitmap={0};GetObject(GetCurrentObject(dc,OBJ_BITMAP),sizeof(bitmap),&bitmap);
 if(bitmap.bmWidth!=640||bitmap.bmHeight!=480)return FALSE;
 if(hdPendingCount==256)return FALSE;
 int length=len<0?lstrlenA(text):len;
 if(length<0||length>1023)return FALSE;
 HDText *item=&hdPending[hdPendingCount++];memcpy(item->text,text,length);item->text[length]=0;
 item->rect=*rect;item->flags=flags;item->color=GetTextColor(dc);
 item->hasParams=params!=NULL;if(params)item->params=*params;
 ZeroMemory(&item->font,sizeof(item->font));GetObjectA(GetCurrentObject(dc,OBJ_FONT),sizeof(LOGFONTA),&item->font);
 RECT copy=*rect;DRAWTEXTPARAMS cp={0};if(params)cp=*params;
 *result=DrawTextExA(dc,text,len,&copy,flags|DT_CALCRECT,params?&cp:NULL);
 return TRUE;
}
static HFONT hd_font(LOGFONTA *source,int width,int height) {
 LOGFONTA lf=*source;lf.lfHeight=MulDiv(lf.lfHeight,height,480);lf.lfWidth=MulDiv(lf.lfWidth,width,640);lf.lfQuality=ANTIALIASED_QUALITY;
 for(int i=0;i<hdFontCount;i++)if(!memcmp(&hdFonts[i].spec,&lf,sizeof(lf)))return hdFonts[i].handle;
 if(hdFontCount==16)return NULL;
 HFONT font=CreateFontIndirectA(&lf);if(font){hdFonts[hdFontCount].spec=lf;hdFonts[hdFontCount++].handle=font;}return font;
}
static BOOL hd_present(HDC dc,int x,int y,int width,int height,int sw,int sh,int *result) {
 if(!hdCreditsActive||sw!=640||sh!=480||!hd_load())return FALSE;
 if(hdPendingCount){hdLastCount=hdPendingCount;memcpy(hdLast,hdPending,hdPendingCount*sizeof(HDText));hdPendingCount=0;}
 int saved=SaveDC(dc);IntersectClipRect(dc,x,y,x+width,y+height);
 SetStretchBltMode(dc,HALFTONE);SetBrushOrgEx(dc,0,0,NULL);
 int assetWidth=hdNativeBits?hdNativeInfo.bmiHeader.biWidth:hdDib.dsBm.bmWidth,assetHeight=hdNativeBits?-hdNativeInfo.bmiHeader.biHeight:hdDib.dsBm.bmHeight;
 *result=StretchDIBits(dc,x,y,width,height,0,0,assetWidth,assetHeight,hdNativeBits?(void*)hdNativeBits:hdDib.dsBm.bmBits,hdNativeBits?&hdNativeInfo:(BITMAPINFO*)&hdDib.dsBmih,DIB_RGB_COLORS,SRCCOPY);
 SetBkMode(dc,TRANSPARENT);
 for(int i=0;i<hdLastCount;i++){
  HDText *item=&hdLast[i];HFONT font=hd_font(&item->font,width,height);if(!font)continue;
  SelectObject(dc,font);SetTextColor(dc,item->color);
  RECT rect={x+MulDiv(item->rect.left,width,640),y+MulDiv(item->rect.top,height,480),x+MulDiv(item->rect.right,width,640),y+MulDiv(item->rect.bottom,height,480)};
  DRAWTEXTPARAMS params=item->params;params.iLeftMargin=MulDiv(params.iLeftMargin,width,640);params.iRightMargin=MulDiv(params.iRightMargin,width,640);
  int drawn=DrawTextExA(dc,item->text,-1,&rect,item->flags,item->hasParams?&params:NULL);
  if(hdRenderLogged!=width && i<2){char line[256];wsprintfA(line,"HD text sample: font=%.32s height=%d color=%x rect=%d,%d,%d,%d result=%d text=%.70s",item->font.lfFaceName,item->font.lfHeight,item->color,rect.left,rect.top,rect.right,rect.bottom,drawn,item->text);trace(line);}
 }
 RestoreDC(dc,saved);
 if(hdRenderLogged!=width){char line[192];wsprintfA(line,"HD credits native layer: asset=%dx%d destination=%dx%d vectorText=%d",assetWidth,assetHeight,width,height,hdLastCount);trace(line);hdRenderLogged=width;}
 return TRUE;
}
