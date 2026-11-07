#include <windows.h>
#include <string.h>
#include <dbghelp.h>
#include <intrin.h>

/* Test-only isolation. The game sees fresh in-memory settings and writes files
   only inside this copied game directory. It never changes the installed game. */
static char root[MAX_PATH];
static HKEY fake=(HKEY)0x5244344b;
static struct { char name[128]; BYTE data[2048]; DWORD size,type; } values[128];
static int count;
static void trace(const char *line) {
 DWORD savedError=GetLastError();
 char file[MAX_PATH];lstrcpyA(file,root);lstrcatA(file,"\\rr4k-trace.log");
 HANDLE h=CreateFileA(file,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
 if(h!=INVALID_HANDLE_VALUE){DWORD done;WriteFile(h,line,lstrlenA(line),&done,NULL);WriteFile(h,"\r\n",2,&done,NULL);CloseHandle(h);}
 SetLastError(savedError);
}
typedef int (WINAPI *RRNextDrawText)(HDC,LPSTR,int,LPRECT,UINT,LPDRAWTEXTPARAMS);
static RRNextDrawText rrNextDrawTextExA=DrawTextExA;
typedef int (WINAPI *RRNextString)(HINSTANCE,UINT,LPSTR,int);
static RRNextString rrNextLoadStringA=LoadStringA;
#include "input-safety.h"
#include "startup-state.h"
#include "hd-credits.h"
#include "safe-jpeg-rows.h"
#include "auto-display.h"
#include "race-test.h"
#include "native-settings.h"
#include "native-projection.h"
#include "native-edge-tables.h"
#include "surface-probe.h"
#include "hd-clip-region.h"
static LONG CALLBACK crash_trace(EXCEPTION_POINTERS *exception) {
 if(exception->ExceptionRecord->ExceptionCode!=EXCEPTION_ACCESS_VIOLATION)return EXCEPTION_CONTINUE_SEARCH;
 char line[512];CONTEXT *r=exception->ContextRecord;
 wsprintfA(line,"ACCESS VIOLATION eip=%08x esp=%08x ebp=%08x eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x operation=%u address=%08x",r->Eip,r->Esp,r->Ebp,r->Eax,r->Ebx,r->Ecx,r->Edx,r->Esi,r->Edi,(DWORD)exception->ExceptionRecord->ExceptionInformation[0],(DWORD)exception->ExceptionRecord->ExceptionInformation[1]);trace(line);
 for(int i=0;i<24;i++){
  DWORD *p=(DWORD*)(r->Esp+4*i);MEMORY_BASIC_INFORMATION m;
  if(VirtualQuery(p,&m,sizeof(m)) && m.State==MEM_COMMIT && !(m.Protect&(PAGE_GUARD|PAGE_NOACCESS))){wsprintfA(line,"stack[%02d]=%08x",i,*p);trace(line);}
 }
 char file[MAX_PATH];lstrcpyA(file,root);lstrcatA(file,"\\rr4k-crash.dmp");
 HANDLE h=CreateFileA(file,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
 if(h!=INVALID_HANDLE_VALUE){MINIDUMP_EXCEPTION_INFORMATION info={GetCurrentThreadId(),exception,FALSE};MiniDumpWriteDump(GetCurrentProcess(),GetCurrentProcessId(),h,MiniDumpNormal,&info,NULL,NULL);CloseHandle(h);}
 return EXCEPTION_CONTINUE_SEARCH;
}
static LONG WINAPI open_key(HKEY key,LPCSTR sub,DWORD opt,REGSAM access,PHKEY out) {
 if(key==fake || (sub && strstr(sub,"Electronic Arts\\RoadRash 95"))) { *out=fake;return ERROR_SUCCESS; }
 return RegOpenKeyExA(key,sub,opt,access & ~KEY_WRITE,out);
}
static LONG WINAPI create_key(HKEY key,LPCSTR sub,DWORD reserved,LPSTR cls,DWORD opt,REGSAM access,LPSECURITY_ATTRIBUTES sa,PHKEY out,LPDWORD disposition) {
 if(key==fake || (sub && strstr(sub,"Electronic Arts\\RoadRash 95"))) { *out=fake;if(disposition)*disposition=REG_OPENED_EXISTING_KEY;return ERROR_SUCCESS; }
 return ERROR_ACCESS_DENIED;
}
static LONG copy_value(DWORD typ,const void *src,DWORD size,LPDWORD type,LPBYTE dest,LPDWORD capacity) {
 if(type)*type=typ;if(!capacity)return ERROR_INVALID_PARAMETER;
 DWORD available=*capacity;*capacity=size;if(!dest)return ERROR_SUCCESS;
 if(available<size)return ERROR_MORE_DATA;memcpy(dest,src,size);return ERROR_SUCCESS;
}
static LONG WINAPI query_value(HKEY key,LPCSTR name,LPDWORD reserved,LPDWORD type,LPBYTE data,LPDWORD size) {
 if(key!=fake)return RegQueryValueExA(key,name,reserved,type,data,size);
 if(!name)return ERROR_FILE_NOT_FOUND;
 static int logged=0;if(logged++<100){char line[256];wsprintfA(line,"registry query: %.120s",name);trace(line);}
 if(!lstrcmpiA(name,"path"))return copy_value(REG_SZ,root,lstrlenA(root)+1,type,data,size);
 if(!lstrcmpiA(name,"language"))return copy_value(REG_SZ,"English",8,type,data,size);
 if(!lstrcmpiA(name,"HiRes")||!lstrcmpiA(name,"HiResBikers")||!lstrcmpiA(name,"HiResCars")){BYTE yes=1;return copy_value(REG_BINARY,&yes,1,type,data,size);}
 if(!lstrcmpiA(name,"Windowed")){BYTE no=configuredWindowed?1:0;return copy_value(REG_BINARY,&no,1,type,data,size);}
 for(int i=0;i<count;i++)if(!lstrcmpiA(name,values[i].name))return copy_value(values[i].type,values[i].data,values[i].size,type,data,size);
 /* Preserve the native four-byte movie mode; 0 disables playback, 3 tries bundled enhanced movies first. */
 if(!lstrcmpiA(name,"VideoMode")){DWORD mode=3;return copy_value(REG_BINARY,&mode,sizeof(mode),type,data,size);}
 return ERROR_FILE_NOT_FOUND;
}
static LONG WINAPI set_value(HKEY key,LPCSTR name,DWORD reserved,DWORD type,const BYTE *data,DWORD size) {
 if(key!=fake)return ERROR_ACCESS_DENIED;
 if(!name||lstrlenA(name)>=128||size>2048)return ERROR_INVALID_PARAMETER;
 int i=0;for(;i<count;i++)if(!lstrcmpiA(name,values[i].name))break;
 if(i==128)return ERROR_NOT_ENOUGH_MEMORY;if(i==count)count++;
 lstrcpyA(values[i].name,name);values[i].size=size;values[i].type=type;memcpy(values[i].data,data,size);return ERROR_SUCCESS;
}
static LONG WINAPI close_key(HKEY key) {return key==fake?ERROR_SUCCESS:RegCloseKey(key);}
static LONG WINAPI flush_key(HKEY key) {return key==fake?ERROR_SUCCESS:ERROR_ACCESS_DENIED;}
static LONG WINAPI enum_value(HKEY key,DWORD index,LPSTR name,LPDWORD nameSize,LPDWORD reserved,LPDWORD type,LPBYTE data,LPDWORD size) {
 if(key==fake)return ERROR_NO_MORE_ITEMS;return RegEnumValueA(key,index,name,nameSize,reserved,type,data,size);
}
static LONG WINAPI delete_key(HKEY key,LPCSTR name){return ERROR_ACCESS_DENIED;}
static BOOL allowed_write(LPCWSTR path) {
 WCHAR full[32768],base[MAX_PATH];if(!GetFullPathNameW(path,32768,full,NULL))return FALSE;
 if(!MultiByteToWideChar(CP_ACP,0,root,-1,base,MAX_PATH))return FALSE;
 int n=lstrlenW(base);return !_wcsnicmp(full,base,n)&&full[n]==L'\\';
}
static HANDLE WINAPI create_file_w(LPCWSTR name,DWORD access,DWORD share,LPSECURITY_ATTRIBUTES sa,DWORD creation,DWORD flags,HANDLE templateFile) {
 if(((access&(GENERIC_WRITE|GENERIC_ALL|DELETE|WRITE_DAC|WRITE_OWNER))||creation!=OPEN_EXISTING)&&!allowed_write(name)){SetLastError(ERROR_ACCESS_DENIED);return INVALID_HANDLE_VALUE;}
 if(creation==OPEN_EXISTING)hd_on_file(name);
 return CreateFileW(name,access,share,sa,creation,flags,templateFile);
}
static HANDLE WINAPI create_file_a(LPCSTR name,DWORD access,DWORD share,LPSECURITY_ATTRIBUTES sa,DWORD creation,DWORD flags,HANDLE templateFile) {
 WCHAR wide[32768];if(!MultiByteToWideChar(CP_ACP,0,name,-1,wide,32768)){SetLastError(ERROR_INVALID_NAME);return INVALID_HANDLE_VALUE;}
 return create_file_w(wide,access,share,sa,creation,flags,templateFile);
}
static int WINAPI stretch_dibits(HDC dc,int x,int y,int width,int height,int sx,int sy,int sw,int sh,const void *bits,const BITMAPINFO *info,UINT usage,DWORD rop) {
 native_menu_frame(info,bits);rr_race_frame(info,bits);
 if(nativePresentStarted){static RECT previous;RECT clip;int kind=GetClipBox(dc,&clip);if(memcmp(&previous,&clip,sizeof(clip))){char line[192];wsprintfA(line,"Native output clip: kind=%d rect=%d,%d,%d,%d requested=%d,%d,%d,%d device=%dx%d",kind,clip.left,clip.top,clip.right,clip.bottom,x,y,width,height,GetDeviceCaps(dc,HORZRES),GetDeviceCaps(dc,VERTRES));trace(line);previous=clip;}}
 static int lastW,lastH,lastSrcW,lastSrcH;
 if(info && (width!=lastW||height!=lastH||info->bmiHeader.biWidth!=lastSrcW||info->bmiHeader.biHeight!=lastSrcH)){
  char line[256];wsprintfA(line,"StretchDIBits: bitmap=%dx%d bpp=%d source=%dx%d dest=%dx%d stretchmode=%d",info->bmiHeader.biWidth,info->bmiHeader.biHeight,info->bmiHeader.biBitCount,sw,sh,width,height,GetStretchBltMode(dc));trace(line);
  lastW=width;lastH=height;lastSrcW=info->bmiHeader.biWidth;lastSrcH=info->bmiHeader.biHeight;
 }
 int result;if(!raceTestEnabled&&!trialMode&&hd_present(dc,x,y,width,height,sw,sh,&result))return result;
 int oldMode=0;if(nativePresentStarted&&fastOutput&&(width!=sw||height!=sh)){oldMode=SetStretchBltMode(dc,COLORONCOLOR);static int once;if(!once++)trace("Fast native output scaling enabled");}
 result=StretchDIBits(dc,x,y,width,height,sx,sy,sw,sh,bits,info,usage,rop);if(oldMode)SetStretchBltMode(dc,oldMode);if(result>0&&info&&info->bmiHeader.biBitCount==32&&*(DWORD*)((BYTE*)GetModuleHandleW(NULL)+0x1c4044)!=0)rr_startup_ready();return result;
}
static HBITMAP WINAPI create_dib_section(HDC dc,const BITMAPINFO *info,UINT usage,void **bits,HANDLE section,DWORD offset) {
 if(info){char line[128];wsprintfA(line,"CreateDIBSection: %dx%d bpp=%d",info->bmiHeader.biWidth,info->bmiHeader.biHeight,info->bmiHeader.biBitCount);trace(line);}
 return CreateDIBSection(dc,info,usage,bits,section,offset);
}
typedef HRESULT (WINAPI *DRAWCREATE)(GUID*,void**,IUnknown*);
static HRESULT WINAPI draw_create(GUID *guid,void **out,IUnknown *outer) {
 DWORD incoming=GetLastError();trace("DirectDrawCreate called");rr_startup_phase("DIRECTDRAW_CREATE_BEGIN",S_OK);
 DRAWCREATE fn=(DRAWCREATE)GetProcAddress(GetModuleHandleA("ddraw.dll"),"DirectDrawCreate");
 SetLastError(incoming);HRESULT result=fn?fn(guid,out,outer):E_FAIL;DWORD outgoing=GetLastError();
 {char line[128];wsprintfA(line,"DirectDrawCreate driver result=%08x",result);trace(line);}
 if(SUCCEEDED(result)&&(raceTestEnabled||trialMode||nativeWidth)){
  HRESULT hookResult=out&&*out?rr_probe_draw(*out):E_UNEXPECTED;rr_startup_phase("DIRECTDRAW_HOOK_RESULT",hookResult);
  if(FAILED(hookResult)){
   if(out&&*out){void *object=*out;RRDrawRelease release=(RRDrawRelease)(*(void***)object)[2];release(object);*out=NULL;}
   result=hookResult;trace("DirectDraw hook unavailable; refusing untracked object");
  }
 }
 rr_startup_phase("DIRECTDRAW_CREATE_RESULT",result);SetLastError(outgoing);return result;
}
static int WINAPI draw_text_ex(HDC dc,LPSTR text,int len,LPRECT rect,UINT flags,LPDRAWTEXTPARAMS params) {
 ULONG_PTR caller=(BYTE*)_ReturnAddress()-(BYTE*)GetModuleHandleW(NULL);
 if(caller==0x38f5a||caller==0x38fc4)hdCreditsActive=1;
 else if(!(flags&DT_CALCRECT)){hdCreditsActive=0;hdPendingCount=hdLastCount=0;}
 static int logged=0;if(logged<8 && !(flags&DT_CALCRECT)){
  char line[256];BITMAP bitmap={0};GetObject(GetCurrentObject(dc,OBJ_BITMAP),sizeof(bitmap),&bitmap);
  wsprintfA(line,"DrawTextExA: bitmap=%dx%d rect=%d,%d,%d,%d flags=%x",bitmap.bmWidth,bitmap.bmHeight,rect->left,rect->top,rect->right,rect->bottom,flags);trace(line);logged++;
 }
 int result;if(!raceTestEnabled&&!trialMode&&hd_text(dc,text,len,rect,flags,params,&result))return result;
 return rrNextDrawTextExA(dc,text,len,rect,flags,params);
}
static void replace_import(const char *name,void *replacement) {
 BYTE *base=(BYTE*)GetModuleHandleW(NULL);IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER*)base;IMAGE_NT_HEADERS *nt=(IMAGE_NT_HEADERS*)(base+dos->e_lfanew);
 IMAGE_IMPORT_DESCRIPTOR *d=(IMAGE_IMPORT_DESCRIPTOR*)(base+nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
 for(;d->Name;d++) {
  if(!d->OriginalFirstThunk)continue;
  IMAGE_THUNK_DATA *n=(IMAGE_THUNK_DATA*)(base+d->OriginalFirstThunk),*t=(IMAGE_THUNK_DATA*)(base+d->FirstThunk);
  for(;n->u1.AddressOfData;n++,t++) {
   if(IMAGE_SNAP_BY_ORDINAL(n->u1.Ordinal))continue;
   IMAGE_IMPORT_BY_NAME *s=(IMAGE_IMPORT_BY_NAME*)(base+n->u1.AddressOfData);
   if(!lstrcmpA((char*)s->Name,name)){DWORD old;if(VirtualProtect(&t->u1.Function,sizeof(void*),PAGE_READWRITE,&old)){if(!lstrcmpA(name,"DrawTextExA")&&t->u1.Function!=(ULONG_PTR)replacement)rrNextDrawTextExA=(RRNextDrawText)t->u1.Function;
 if(!lstrcmpA(name,"SelectObject")&&t->u1.Function!=(ULONG_PTR)replacement)rrNextSelectObject=(RRNextSelect)t->u1.Function;
 if(!lstrcmpA(name,"LoadStringA")&&t->u1.Function!=(ULONG_PTR)replacement)rrNextLoadStringA=(RRNextString)t->u1.Function;
 if(!lstrcmpA(name,"ClipCursor")&&t->u1.Function!=(ULONG_PTR)replacement)rrNextClipCursor=(RRNextClip)t->u1.Function;
 if(!lstrcmpA(name,"ShowCursor")&&t->u1.Function!=(ULONG_PTR)replacement)rrNextShowCursor=(RRNextShow)t->u1.Function;
 t->u1.Function=(ULONG_PTR)replacement;VirtualProtect(&t->u1.Function,sizeof(void*),old,&old);}}
  }
 }
}
__declspec(dllexport) void RR4KBootstrap(void) {}
static void WINAPI trial_exit_log(UINT code){char line[128];wsprintfA(line,"Main ExitProcess: code=%u callerRVA=%x",code,(DWORD)((BYTE*)_ReturnAddress()-(BYTE*)GetModuleHandleW(NULL)));trace(line);rr_startup_exit(code);ExitProcess(code);}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID reserved) {
 if(reason==DLL_PROCESS_ATTACH) {
  GetModuleFileNameA(instance,root,MAX_PATH);char *last=strrchr(root,'\\');if(!last)return FALSE;*last=0;
  rr_startup_begin();trace("Isolation DLL loaded; no shared registry writes; no desktop switching.");
  AddVectoredExceptionHandler(1,crash_trace);
  race_init();
  ui_init();if(!ui_install_coordinates((BYTE*)GetModuleHandleW(NULL)))return FALSE;
  if(!native_settings_init())return FALSE;
  rr_native_projection();
  rr_native_edges();
  if(!raceTestEnabled&&!trialMode)install_safe_jpeg_rows();
  char modulePath[MAX_PATH];GetModuleFileNameA(GetModuleHandleA("ddraw.dll"),modulePath,MAX_PATH);trace(modulePath);
  replace_import("RegOpenKeyExA",open_key);replace_import("RegCreateKeyExA",create_key);
  replace_import("RegQueryValueExA",query_value);replace_import("RegSetValueExA",set_value);
  replace_import("RegCloseKey",close_key);replace_import("RegFlushKey",flush_key);
  replace_import("RegEnumValueA",enum_value);replace_import("RegDeleteKeyA",delete_key);
  replace_import("CreateFileA",create_file_a);replace_import("CreateFileW",create_file_w);
  replace_import("StretchDIBits",stretch_dibits);
  replace_import("CreateDIBSection",create_dib_section);replace_import("DirectDrawCreate",draw_create);
  replace_import("DrawTextExA",draw_text_ex);replace_import("LoadStringA",native_load_string);replace_import("SelectObject",rr_select_object);
  replace_import("GetKeyboardState",race_keyboard);replace_import("GetAsyncKeyState",race_async_key);
  replace_import("DispatchMessageA",race_dispatch);
  replace_import("ClipCursor",rr_guard_clip);replace_import("ShowCursor",rr_guard_show);
  replace_import("ExitProcess",trial_exit_log);
  DisableThreadLibraryCalls(instance);
 }
 return TRUE;
}
