/* Per-process startup evidence. No process-global desktop or display changes. */
static char rrStartupPath[MAX_PATH];
static DWORD rrStartupTick;
static BOOL rrStartupReady,rrStartupIntentional;
static void rr_startup_line(const char *line) {
 DWORD savedError=GetLastError();
 if(!rrStartupPath[0])return;HANDLE h=CreateFileA(rrStartupPath,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
 if(h!=INVALID_HANDLE_VALUE){char text[640];int n=wsprintfA(text,"t=%u %s\r\n",GetTickCount()-rrStartupTick,line);DWORD done;WriteFile(h,text,n,&done,NULL);CloseHandle(h);}
 SetLastError(savedError);
}
static void rr_startup_begin(void) {
 DWORD savedError=GetLastError();
 rrStartupTick=GetTickCount();char name[64],token[80]={0},line[256];wsprintfA(name,"\\rr4k-startup-%u.log",GetCurrentProcessId());lstrcpyA(rrStartupPath,root);lstrcatA(rrStartupPath,name);
 HANDLE h=CreateFileA(rrStartupPath,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);if(h!=INVALID_HANDLE_VALUE)CloseHandle(h);
 GetEnvironmentVariableA("RR_STARTUP_TOKEN",token,sizeof(token));token[sizeof(token)-1]=0;wsprintfA(line,"START pid=%u token=%s",GetCurrentProcessId(),token);rr_startup_line(line);SetLastError(savedError);
}
static void rr_startup_phase(const char *phase,HRESULT result) {DWORD savedError=GetLastError();char line[192];wsprintfA(line,"PHASE %s HRESULT=%08x",phase,result);rr_startup_line(line);SetLastError(savedError);}
static int rr_startup_test_mode(void) {
 char flag[MAX_PATH],mode[32]={0};GetEnvironmentVariableA("RR_STARTUP_TEST",mode,sizeof(mode));
 lstrcpyA(flag,root);lstrcatA(flag,"\\rr4k-startup-test.enabled");if(GetFileAttributesA(flag)==INVALID_FILE_ATTRIBUTES)return 0;
 if(!lstrcmpA(mode,"coop-fail"))return 1;if(!lstrcmpA(mode,"user-exit"))return 2;if(!lstrcmpA(mode,"fatal-exit"))return 3;if(!lstrcmpA(mode,"delay"))return 4;return 0;
}
static void rr_startup_ready(void){if(!rrStartupReady){rrStartupReady=TRUE;rr_startup_line("READY first software frame presented");}}
static void rr_startup_intentional(void){rrStartupIntentional=TRUE;rr_startup_line("INTENTIONAL user exit requested");}
static void rr_startup_exit(UINT code) {
 rr_release_cursor();
 char line[256];wsprintfA(line,"EXIT code=%u ready=%d intentional=%d",code,rrStartupReady,rrStartupIntentional);rr_startup_line(line);
 if(!rrStartupReady&&!rrStartupIntentional){void *stack[16];USHORT n=CaptureStackBackTrace(0,16,stack,NULL);BYTE *base=(BYTE*)GetModuleHandleW(NULL);for(USHORT i=0;i<n;i++){wsprintfA(line,"Early exit stack[%u]=%08x moduleRVA=%x",i,(DWORD)stack[i],(DWORD)((BYTE*)stack[i]-base));trace(line);rr_startup_line(line);}}
}
