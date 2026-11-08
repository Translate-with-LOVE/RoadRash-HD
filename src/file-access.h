/* Keep copied-game writes local, except the supported game's native .RRS save.
   Opening a save still uses ordinary Windows ACLs, attributes and sharing rules. */
static BOOL allowed_write(LPCWSTR path) {
 WCHAR full[32768],base[MAX_PATH];DWORD size=GetFullPathNameW(path,32768,full,NULL);
 if(!size||size>=32768)return FALSE;
 if(!MultiByteToWideChar(CP_ACP,0,root,-1,base,MAX_PATH))return FALSE;
 int n=lstrlenW(base);return !_wcsnicmp(full,base,n)&&full[n]==L'\\';
}
static HANDLE WINAPI create_file_w(LPCWSTR name,DWORD access,DWORD share,LPSECURITY_ATTRIBUTES sa,DWORD creation,DWORD flags,HANDLE templateFile) {
 if(((access&(GENERIC_WRITE|GENERIC_ALL|DELETE|WRITE_DAC|WRITE_OWNER))||creation!=OPEN_EXISTING)&&!allowed_write(name)){SetLastError(ERROR_ACCESS_DENIED);return INVALID_HANDLE_VALUE;}
 if(creation==OPEN_EXISTING)hd_on_file(name);
 return CreateFileW(name,access,share,sa,creation,flags,templateFile);
}
static BOOL rr_native_save_target(LPCWSTR name,ULONG_PTR callerRva,DWORD access,DWORD share,LPSECURITY_ATTRIBUTES sa,DWORD creation,DWORD flags,HANDLE templateFile,LPWSTR full) {
 /* 0x43f7dd: CreateFileA in the 160-byte native save routine. Its failure path
    silently returns. Save, Save As and save-after-load all use this call site. */
 if(callerRva!=0x3f7e3||access!=GENERIC_WRITE||share||sa||creation!=OPEN_ALWAYS||flags!=FILE_FLAG_SEQUENTIAL_SCAN||templateFile)return FALSE;
 DWORD n=GetFullPathNameW(name,32768,full,NULL);
 if(!n||n>=32768||n<4||_wcsicmp(full+n-4,L".RRS"))return FALSE;
 if(!_wcsnicmp(full,L"\\\\.\\",4)||!_wcsnicmp(full,L"\\\\?\\",4))return FALSE;
 for(DWORD i=0;i<n;i++)if(full[i]==L':'&&(i!=1||!((full[0]>=L'A'&&full[0]<=L'Z')||(full[0]>=L'a'&&full[0]<=L'z'))))return FALSE;
 return TRUE;
}
static HANDLE WINAPI create_file_a(LPCSTR name,DWORD access,DWORD share,LPSECURITY_ATTRIBUTES sa,DWORD creation,DWORD flags,HANDLE templateFile) {
 ULONG_PTR callerRva=(ULONG_PTR)_ReturnAddress()-(ULONG_PTR)GetModuleHandleA(NULL);
 WCHAR wide[32768];if(!MultiByteToWideChar(CP_ACP,0,name,-1,wide,32768)){SetLastError(ERROR_INVALID_NAME);return INVALID_HANDLE_VALUE;}
 WCHAR full[32768];
 if(rr_native_save_target(wide,callerRva,access,share,sa,creation,flags,templateFile,full)){
  HANDLE h=CreateFileW(full,access,share,sa,creation,flags,templateFile);DWORD error=GetLastError();
  char line[192];wsprintfA(line,"Native save open: result=%s error=%lu",h==INVALID_HANDLE_VALUE?"failed":"opened",error);trace(line);
  SetLastError(error);return h;
 }
 return create_file_w(wide,access,share,sa,creation,flags,templateFile);
}
