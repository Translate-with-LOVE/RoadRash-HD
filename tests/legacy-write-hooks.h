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
