#include <windows.h>
#include <stdio.h>
#include <string.h>
static char root[MAX_PATH];static DWORD screenW=1920,screenH=1080;static int queryCalls,checks,failures;static BOOL enumOk=TRUE;static char lastTrace[256];
#define CHECK(x) do{checks++;if(!(x)){failures++;printf("FAIL line=%d %s\n",__LINE__,#x);}}while(0)
static void trace(const char *s){lstrcpynA(lastTrace,s,sizeof(lastTrace));}
static BOOL WINAPI mock_enum(LPCSTR name,DWORD mode,DEVMODEA *dm){queryCalls++;CHECK(name==NULL&&mode==ENUM_CURRENT_SETTINGS&&dm->dmSize==sizeof(*dm));if(!enumOk)return FALSE;dm->dmPelsWidth=screenW;dm->dmPelsHeight=screenH;return TRUE;}
static int WINAPI mock_metrics(int index){return index==SM_CXSCREEN?(int)screenW:index==SM_CYSCREEN?(int)screenH:0;}
#define EnumDisplaySettingsA mock_enum
#define GetSystemMetrics mock_metrics
#include "../src/auto-display.h"
static void rr_startup_intentional(void){}static void rr_startup_exit(UINT code){}
#include "../src/race-test.h"
static int (WINAPI *rrNextLoadStringA)(HINSTANCE,UINT,LPSTR,int)=LoadStringA;
static int WINAPI mock_message(HWND h,LPCWSTR text,LPCWSTR title,UINT flags){CHECK(FALSE);return 0;}
#define MessageBoxW mock_message
#include "../src/native-settings.h"
static void put(const char *path,const char *data){HANDLE h=CreateFileA(path,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);CHECK(h!=INVALID_HANDLE_VALUE);DWORD n=0;if(h!=INVALID_HANDLE_VALUE){CHECK(WriteFile(h,data,(DWORD)strlen(data),&n,NULL)&&n==strlen(data));CloseHandle(h);}}
static BOOL exact(const char *path,const char *expected){HANDLE h=CreateFileA(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);if(h==INVALID_HANDLE_VALUE)return FALSE;char data[1024]={0};DWORD n=0;BOOL ok=ReadFile(h,data,sizeof(data)-1,&n,NULL);CloseHandle(h);return ok&&n==strlen(expected)&&!memcmp(data,expected,n);}
int main(int argc,char **argv){SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);CHECK(argc==2);if(argc!=2)return 2;CHECK(GetFullPathNameA(argv[1],MAX_PATH,root,NULL)>0);CHECK(CreateDirectoryA(root,NULL));
 struct{DWORD w,h;int preset;} cases[]={{640,480,0},{800,600,0},{1024,768,1},{1280,720,1},{1280,800,1},{1366,768,1},{1600,900,1},{1920,1080,2},{1920,1200,2},{2560,1080,2},{2560,1440,3},{3440,1440,3},{3840,2160,4},{5120,1440,3},{5120,2880,4},{1080,1920,1},{320,240,0}};
 for(int i=0;i<sizeof(cases)/sizeof(cases[0]);i++){screenW=cases[i].w;screenH=cases[i].h;DWORD sw=0,sh=0;CHECK(rr_display_screen(&sw,&sh)==cases[i].preset&&sw==screenW&&sh==screenH);char p[MAX_PATH];wsprintfA(p,"%s\\fresh-%d.ini",root,i);DWORD w=999,h=999;BOOL win=TRUE,fast=FALSE;rr_display_initialize(p,&w,&h,&win,&fast);CHECK(w==rrPreset[cases[i].preset][0]&&h==rrPreset[cases[i].preset][1]&&!win&&fast);DWORD rw,rh;BOOL rwnd,rfast;CHECK(rr_display_existing(p,&rw,&rh,&rwnd,&rfast)&&rw==w&&rh==h);char raw[192];wsprintfA(raw,"[render]\r\nwidth=%u\r\nheight=%u\r\nwindowed=0\r\nfast_output=1\r\n",w,h);CHECK(exact(p,raw));screenW=3840;screenH=2160;queryCalls=0;rr_display_initialize(p,&w,&h,&win,&fast);CHECK(!queryCalls&&w==rw&&h==rh&&exact(p,raw));}
 for(DWORD sw=640;sw<=5120;sw+=137)for(DWORD sh=480;sh<=2880;sh+=113){int i=rr_display_fit(sw,sh);CHECK(i>=0&&i<=4);if(i){CHECK(rrPreset[i][0]<=sw&&rrPreset[i][1]<=sh);CHECK(rrPreset[i][0]*3==rrPreset[i][1]*4);}for(int j=i+1;j<5;j++)CHECK(rrPreset[j][0]>sw||rrPreset[j][1]>sh);}
 char p[MAX_PATH];wsprintfA(p,"%s\\manual.ini",root);const char *manual="[render]\r\nwidth=1920\r\nheight=1440\r\nwindowed=1\r\nfast_output=0\r\n; User note retained\r\n";put(p,manual);screenW=1280;screenH=720;DWORD w,h;BOOL win,fast;queryCalls=0;rr_display_initialize(p,&w,&h,&win,&fast);CHECK(w==1920&&h==1440&&win&&!fast&&!queryCalls&&exact(p,manual));CHECK(!rr_display_create(p,2880,2160)&&exact(p,manual));
 const char *bad[] = {"[render]\r\nwidth=1440\r\n", "[render]\r\nwidth=99999\r\nheight=1080\r\n", "[render]\r\nwidth=abc\r\nheight=def\r\n", "[render]\r\nwidth=1441\r\nheight=1080\r\n"};
 for(int i=0;i<4;i++){wsprintfA(p,"%s\\invalid-%d.ini",root,i);put(p,bad[i]);screenW=1920;screenH=1080;rr_display_initialize(p,&w,&h,&win,&fast);CHECK(w==1440&&h==1080&&exact(p,bad[i]));}
 wsprintfA(p,"%s\\no-parent\\render.ini",root);rr_display_initialize(p,&w,&h,&win,&fast);CHECK(w==1440&&h==1080&&GetFileAttributesA(p)==INVALID_FILE_ATTRIBUTES);
 enumOk=FALSE;screenW=1366;screenH=768;CHECK(rr_display_screen(&w,&h)==1&&w==1366&&h==768);screenW=screenH=0;CHECK(rr_display_screen(&w,&h)==0&&w==640&&h==480);enumOk=TRUE;
 // Exercise the real initialization path called by the DLL on normal direct launch.
 wsprintfA(p,"%s\\rr4k-trial.enabled",root);put(p,"");screenW=1920;screenH=1080;SetEnvironmentVariableA("RR_FORCE_ORIGINAL",NULL);race_init();CHECK(trialMode&&!raceTestEnabled&&nativeWidth==1440&&nativeHeight==1080&&nativeProjection2x&&nativeRational);
 for(int i=0;i<5;i++){CHECK(display_write(i,TRUE,FALSE));screenW=3840;screenH=2160;queryCalls=0;race_init();CHECK(nativeWidth==rrPreset[i][0]&&nativeHeight==rrPreset[i][1]&&configuredWindowed&&!fastOutput&&!queryCalls);CHECK(native_saved_selection()==i);}
 CHECK(display_write(2,FALSE,TRUE));SetEnvironmentVariableA("RR_FORCE_ORIGINAL","1");race_init();CHECK(nativeWidth==0&&nativeHeight==0);SetEnvironmentVariableA("RR_FORCE_ORIGINAL",NULL);race_init();CHECK(nativeWidth==1440&&nativeHeight==1080);
 CHECK(display_write(4,TRUE,FALSE));screenW=1920;screenH=1080;native_resolution_selected(6,NULL);race_init();CHECK(nativeWidth==1440&&nativeHeight==1080&&configuredWindowed&&!fastOutput);screenW=3840;screenH=2160;race_init();CHECK(nativeWidth==1440&&nativeHeight==1080&&configuredWindowed&&!fastOutput);
 WIN32_FIND_DATAA found;wsprintfA(p,"%s\\rr-display-first-*.tmp",root);HANDLE find=FindFirstFileA(p,&found);CHECK(find==INVALID_HANDLE_VALUE);if(find!=INVALID_HANDLE_VALUE)FindClose(find);
 printf("auto-display checks=%d failures=%d screenCases=%d manualPresets=5 realRaceInit=1 realMenuWrite=1 noGameWindowInputAudio=1\n",checks,failures,(int)(sizeof(cases)/sizeof(cases[0])));return failures?1:0;}