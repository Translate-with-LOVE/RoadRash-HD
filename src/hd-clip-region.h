/* Native credits SelectObject(HRGN) uses device units even on the logical HD DC. */
typedef HGDIOBJ (WINAPI *RRNextSelect)(HDC,HGDIOBJ);
static RRNextSelect rrNextSelectObject=SelectObject;
static HGDIOBJ WINAPI rr_select_object(HDC dc,HGDIOBJ object){
 DWORD incoming=GetLastError();HRGN mapped=NULL;DWORD caller=(DWORD)((BYTE*)_ReturnAddress()-(BYTE*)GetModuleHandleW(NULL));
 if(caller==0x38f1c&&uiEnabled&&GetMapMode(dc)==MM_ANISOTROPIC&&GetObjectType(object)==OBJ_REGION){
  SIZE window={0},viewport={0};RECT box={0};
  if(GetWindowExtEx(dc,&window)&&GetViewportExtEx(dc,&viewport)&&window.cx==640&&window.cy==480&&viewport.cx==(LONG)uiWidth&&viewport.cy==(LONG)uiHeight&&GetRgnBox((HRGN)object,&box)==SIMPLEREGION&&box.left==0&&box.top==0&&box.right==640&&box.bottom==480){
   POINT corners[2]={{box.left,box.top},{box.right,box.bottom}};
   if(LPtoDP(dc,corners,2)){RECT pixels={corners[0].x,corners[0].y,corners[1].x,corners[1].y};mapped=CreateRectRgnIndirect(&pixels);}
   if(mapped){static DWORD loggedWidth;if(loggedWidth!=uiWidth){char line[160];wsprintfA(line,"Native credits clip mapped: logical=640x480 device=%ux%u callerRVA=%x",uiWidth,uiHeight,caller);trace(line);loggedWidth=uiWidth;}}
  }
 }
 SetLastError(incoming);HGDIOBJ result=rrNextSelectObject(dc,mapped?(HGDIOBJ)mapped:object);DWORD outgoing=GetLastError();
 if(mapped)DeleteObject(mapped);SetLastError(outgoing);return result;
}
