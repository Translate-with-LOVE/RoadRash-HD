#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <intrin.h>
static char root[MAX_PATH];
static int checks,failures,reads,traces;
static ULONG_PTR testRva=0x3f7e3;
static void hd_on_file(LPCWSTR p){(void)p;reads++;}
static void trace(const char *p){(void)p;traces++;SetLastError(123456);}
#ifdef LEGACY
#include "legacy-write-hooks.h"
#else
static void *test_return_address(void){return (void*)((ULONG_PTR)GetModuleHandleA(NULL)+testRva);}
#define _ReturnAddress() test_return_address()
#include "../src/file-access.h"
#undef _ReturnAddress
#endif
#define CHECK(x) do{checks++;if(!(x)){failures++;printf("FAIL line %d: %s error=%lu\n",__LINE__,#x,GetLastError());}}while(0)
static void fill(BYTE *p,int seed){
 for(int i=0;i<160;i++)p[i]=(BYTE)(seed+i*17);
 *(DWORD*)(p+4)=0xcafedead;DWORD sum=0,weighted=0;LONG alternating=0;
 for(int i=4;i<160;i++){DWORD n=160-i;sum+=p[i];weighted+=p[i]*n;alternating+=(n&1)?p[i]:-p[i];}
 *(DWORD*)p=((((DWORD)alternating&0xffff)<<16)|(sum&0xffff))^weighted;
}
static void seed(LPCWSTR p,const BYTE *bytes){DWORD n;HANDLE h=CreateFileW(p,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);CHECK(h!=INVALID_HANDLE_VALUE);if(h!=INVALID_HANDLE_VALUE){CHECK(WriteFile(h,bytes,160,&n,NULL)&&n==160);CloseHandle(h);}}
static void same(LPCWSTR p,const BYTE *bytes){BYTE b[161];DWORD n;HANDLE h=CreateFileW(p,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);CHECK(h!=INVALID_HANDLE_VALUE);if(h!=INVALID_HANDLE_VALUE){CHECK(ReadFile(h,b,sizeof(b),&n,NULL)&&n==160);CHECK(!memcmp(b,bytes,160));CloseHandle(h);}}
static HANDLE native_open(LPCWSTR p){char a[32768];BOOL used=FALSE;CHECK(WideCharToMultiByte(CP_ACP,WC_NO_BEST_FIT_CHARS,p,-1,a,sizeof(a),NULL,&used)>0&&!used);return create_file_a(a,GENERIC_WRITE,0,NULL,OPEN_ALWAYS,FILE_FLAG_SEQUENTIAL_SCAN,NULL);}
static void blocked(LPCWSTR p,DWORD access,DWORD creation){char a[32768];CHECK(WideCharToMultiByte(CP_ACP,0,p,-1,a,sizeof(a),NULL,NULL)>0);HANDLE h=create_file_a(a,access,0,NULL,creation,FILE_FLAG_SEQUENTIAL_SCAN,NULL);CHECK(h==INVALID_HANDLE_VALUE&&GetLastError()==ERROR_ACCESS_DENIED);if(h!=INVALID_HANDLE_VALUE)CloseHandle(h);}
int wmain(int argc,wchar_t **argv){
 if(argc!=2)return 2;
 WCHAR game[MAX_PATH],docs[MAX_PATH],save[MAX_PATH],fresh[MAX_PATH],other[MAX_PATH],internal[MAX_PATH];BYTE before[160],after[160];DWORD n;
 fill(before,7);fill(after,91);
#ifdef LEGACY
 (void)n;
#endif
 CHECK(CreateDirectoryW(argv[1],NULL));
 swprintf_s(game,MAX_PATH,L"%s\\game",argv[1]);swprintf_s(docs,MAX_PATH,L"%s\\documents",argv[1]);CHECK(CreateDirectoryW(game,NULL));CHECK(CreateDirectoryW(docs,NULL));
 CHECK(WideCharToMultiByte(CP_ACP,0,game,-1,root,sizeof(root),NULL,NULL)>0);
 swprintf_s(save,MAX_PATH,L"%s\\TestSave.RRS",docs);swprintf_s(fresh,MAX_PATH,L"%s\\NewSave.RRS",docs);swprintf_s(other,MAX_PATH,L"%s\\Other.dat",docs);swprintf_s(internal,MAX_PATH,L"%s\\Local.dat",game);
 seed(save,before);seed(other,before);seed(internal,before);
 HANDLE h=native_open(save);
#ifdef LEGACY
 CHECK(h==INVALID_HANDLE_VALUE&&GetLastError()==ERROR_ACCESS_DENIED);if(h!=INVALID_HANDLE_VALUE)CloseHandle(h);same(save,before);
 printf("legacy native-save overwrite: access denied; original bytes unchanged\n");
#else
 CHECK(h!=INVALID_HANDLE_VALUE);CHECK(GetLastError()==ERROR_ALREADY_EXISTS);if(h!=INVALID_HANDLE_VALUE){CHECK(WriteFile(h,after,160,&n,NULL)&&n==160);CHECK(CloseHandle(h));}same(save,after);
 h=native_open(fresh);CHECK(h!=INVALID_HANDLE_VALUE);CHECK(GetLastError()==ERROR_SUCCESS);if(h!=INVALID_HANDLE_VALUE){CHECK(WriteFile(h,before,160,&n,NULL)&&n==160);CloseHandle(h);}same(fresh,before);
 /* Save after loading an existing file: no common-dialog hook is required. */
 h=create_file_w(save,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);CHECK(h!=INVALID_HANDLE_VALUE);if(h!=INVALID_HANDLE_VALUE)CloseHandle(h);CHECK(reads==1);
 h=native_open(save);CHECK(h!=INVALID_HANDLE_VALUE);if(h!=INVALID_HANDLE_VALUE){CHECK(WriteFile(h,before,160,&n,NULL)&&n==160);CloseHandle(h);}same(save,before);
 /* Relative native filename follows this process' CWD, not the DLL directory. */
 WCHAR cwd[32768];CHECK(GetCurrentDirectoryW(32768,cwd)>0);CHECK(SetCurrentDirectoryW(docs));h=native_open(L"TestSave.RRS");CHECK(h!=INVALID_HANDLE_VALUE);if(h!=INVALID_HANDLE_VALUE){CHECK(WriteFile(h,after,160,&n,NULL)&&n==160);CloseHandle(h);}CHECK(SetCurrentDirectoryW(cwd));same(save,after);
 CHECK(SetFileAttributesW(save,FILE_ATTRIBUTE_READONLY));h=native_open(save);CHECK(h==INVALID_HANDLE_VALUE&&GetLastError()==ERROR_ACCESS_DENIED);if(h!=INVALID_HANDLE_VALUE)CloseHandle(h);same(save,after);CHECK(SetFileAttributesW(save,FILE_ATTRIBUTE_NORMAL));
 HANDLE lock=CreateFileW(save,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);CHECK(lock!=INVALID_HANDLE_VALUE);h=native_open(save);CHECK(h==INVALID_HANDLE_VALUE&&GetLastError()==ERROR_SHARING_VIOLATION);if(h!=INVALID_HANDLE_VALUE)CloseHandle(h);CloseHandle(lock);same(save,after);
 testRva=0x3f7e4;blocked(save,GENERIC_WRITE,OPEN_ALWAYS);testRva=0x3f7e3;
 blocked(other,GENERIC_WRITE,OPEN_ALWAYS);blocked(save,GENERIC_WRITE,CREATE_ALWAYS);blocked(save,GENERIC_WRITE|GENERIC_READ,OPEN_ALWAYS);same(other,before);same(save,after);
 h=create_file_w(save,GENERIC_WRITE,0,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);CHECK(h==INVALID_HANDLE_VALUE&&GetLastError()==ERROR_ACCESS_DENIED);if(h!=INVALID_HANDLE_VALUE)CloseHandle(h);
 h=create_file_w(internal,GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);CHECK(h!=INVALID_HANDLE_VALUE);if(h!=INVALID_HANDLE_VALUE)CloseHandle(h);
 WCHAR full[32768];CHECK(!rr_native_save_target(L"\\\\.\\Test.RRS",testRva,GENERIC_WRITE,0,NULL,OPEN_ALWAYS,FILE_FLAG_SEQUENTIAL_SCAN,NULL,full));CHECK(!rr_native_save_target(L"\\\\?\\C:\\Test.RRS",testRva,GENERIC_WRITE,0,NULL,OPEN_ALWAYS,FILE_FLAG_SEQUENTIAL_SCAN,NULL,full));CHECK(!rr_native_save_target(L"C:\\Test:Stream.RRS",testRva,GENERIC_WRITE,0,NULL,OPEN_ALWAYS,FILE_FLAG_SEQUENTIAL_SCAN,NULL,full));
 CHECK(!rr_native_save_target(save,testRva,GENERIC_WRITE,FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,FILE_FLAG_SEQUENTIAL_SCAN,NULL,full));CHECK(!rr_native_save_target(save,testRva,GENERIC_WRITE,0,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL,full));
 /* Exercise lowercase extension and a Chinese name when the game's ANSI codepage supports it. */
 WCHAR unicodePath[MAX_PATH];swprintf_s(unicodePath,MAX_PATH,L"%s\\\x6d4b\x8bd5.rRs",docs);char encoded[MAX_PATH];BOOL used=FALSE;
 int converted=WideCharToMultiByte(CP_ACP,WC_NO_BEST_FIT_CHARS,unicodePath,-1,encoded,sizeof(encoded),NULL,&used);
 if(converted&&!used){h=native_open(unicodePath);CHECK(h!=INVALID_HANDLE_VALUE);if(h!=INVALID_HANDLE_VALUE){CHECK(WriteFile(h,before,160,&n,NULL)&&n==160);CloseHandle(h);}same(unicodePath,before);}else printf("ANSI codepage cannot represent Chinese filename; native limitation retained\n");
 CHECK(traces>=7);printf("candidate overwrite/create/save-after-load/relative/read-only/sharing/policy checks completed\n");
#endif
 printf("file access checks=%d failures=%d codepage=%u\n",checks,failures,GetACP());return failures?1:0;
}
