/* Original Graphics and Sound menus, using the game's own popup lifecycle. */
static int displayWidths[5]={0,960,1440,1920,2880};
static int displayHeights[5]={0,720,1080,1440,2160};
static void display_config_path(char *path){lstrcpyA(path,root);lstrcatA(path,"\\rr4k-native-test.ini");}
static BOOL display_write(int selection,int windowed,int fast){
 char path[MAX_PATH],temp[MAX_PATH],data[256];display_config_path(path);lstrcpyA(temp,root);lstrcatA(temp,"\\rr-display-settings.tmp");
 int len=wsprintfA(data,"[render]\r\nwidth=%d\r\nheight=%d\r\nwindowed=%d\r\nfast_output=%d\r\n",displayWidths[selection],displayHeights[selection],windowed,fast);DWORD done=0;
 HANDLE file=CreateFileA(temp,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);if(file==INVALID_HANDLE_VALUE)return FALSE;
 BOOL ok=WriteFile(file,data,len,&done,NULL)&&done==(DWORD)len;FlushFileBuffers(file);CloseHandle(file);
 if(ok)ok=MoveFileExA(temp,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);if(ok){char line[160];wsprintfA(line,"Display settings saved: %dx%d windowed=%d fast=%d; next launch only",displayWidths[selection],displayHeights[selection],windowed,fast);trace(line);}return ok;
}

static DWORD nativeResolutionRows[8][14],nativeModeRows[4][14];
static const char *nativeResolutionLabels[7]={"640x480 (Fallback)","720p - 960x720","1080p - 1440x1080","1440p - 1920x1440","2160p - 2880x2160","Auto-detect screen","Cancel"};
static const char *nativeModeLabels[3]={"Full screen (desktop)","Resizable window","Cancel"};
static BOOL native_chinese(HINSTANCE module){
 if(GetACP()!=936)return FALSE;WCHAR text[64];int n=LoadStringW(module,72,text,64);for(int i=0;i<n;i++)if(text[i]>=0x4e00&&text[i]<=0x9fff)return TRUE;return FALSE;
}
static const char *native_label(int index,BOOL mode){
 static int chinese=-1;static char cnResolution[7][96],cnMode[3][96];
 if(chinese<0){BYTE *base=(BYTE*)GetModuleHandleW(NULL);chinese=native_chinese(*(HINSTANCE*)(base+0x1cd470));
  const WCHAR *res[7]={L"640x480（原版回退）",L"720p - 960x720",L"1080p - 1440x1080",L"1440p - 1920x1440",L"2160p - 2880x2160",L"按屏幕自动选择",L"取消"};
  const WCHAR *modes[3]={L"无边框全屏（当前桌面）",L"可调整窗口",L"取消"};
  if(chinese){for(int i=0;i<7;i++)WideCharToMultiByte(CP_ACP,0,res[i],-1,cnResolution[i],96,NULL,NULL);for(int i=0;i<3;i++)WideCharToMultiByte(CP_ACP,0,modes[i],-1,cnMode[i],96,NULL,NULL);}
 }
 return mode?(chinese?cnMode[index]:nativeModeLabels[index]):(chinese?cnResolution[index]:nativeResolutionLabels[index]);
}
typedef const char *(__fastcall *NativeText)(DWORD*,void*);
static NativeText nativeOriginalText;
static const char *__fastcall native_text(DWORD *item,void *unused){
 for(int i=1;i<=7;i++)if(item==nativeResolutionRows[i])return native_label(i-1,FALSE);
 for(int i=1;i<=3;i++)if(item==nativeModeRows[i])return native_label(i-1,TRUE);
 return nativeOriginalText(item,unused);
}
static BOOL native_install_text(BYTE *base){
 BYTE *target=base+0x5b930;static const BYTE signature[6]={0x55,0x8b,0xec,0x83,0xec,0x20};if(memcmp(target,signature,6))return FALSE;
 BYTE *copy=VirtualAlloc(NULL,16,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);if(!copy)return FALSE;memcpy(copy,target,6);copy[6]=0xe9;*(DWORD*)(copy+7)=(DWORD)((target+6)-(copy+11));nativeOriginalText=(NativeText)copy;
 DWORD old;if(!VirtualProtect(target,6,PAGE_EXECUTE_READWRITE,&old))return FALSE;target[0]=0xe9;*(DWORD*)(target+1)=(DWORD)((BYTE*)native_text-(target+5));target[5]=0x90;VirtualProtect(target,6,old,&old);FlushInstructionCache(GetCurrentProcess(),target,6);return TRUE;
}
static int nativeSaveStatus;
static void native_save_result(BOOL ok){nativeSaveStatus=ok?1:-1;if(!ok){BYTE *base=(BYTE*)GetModuleHandleW(NULL);trace("Native display settings save failed");MessageBoxW(*(HWND*)(base+0x1cd474),L"Display settings could not be saved. Existing settings are retained.",L"Road Rash",MB_OK|MB_ICONERROR);}}

static int native_saved_selection(void){char path[MAX_PATH];display_config_path(path);int w=GetPrivateProfileIntA("render","width",nativeWidth,path);for(int i=0;i<5;i++)if(w==displayWidths[i])return i;return rr_display_fit(nativeWidth?nativeWidth:640,nativeHeight?nativeHeight:480);}
static int native_saved_value(const char *key,int fallback){char path[MAX_PATH];display_config_path(path);return GetPrivateProfileIntA("render",key,fallback,path);}
static void __fastcall native_resolution_selected(DWORD value,void *unused){
 if(value==7)return;
 if(value==6){DWORD w,h;native_save_result(display_write(rr_display_screen(&w,&h),native_saved_value("windowed",configuredWindowed)!=0,native_saved_value("fast_output",fastOutput)!=0));}
 else if(value>=1&&value<=5)native_save_result(display_write(value-1,native_saved_value("windowed",0)!=0,native_saved_value("fast_output",1)!=0));
}
static void __fastcall native_mode_selected(DWORD value,void *unused){if(value==3)return;if(value==1||value==2)native_save_result(display_write(native_saved_selection(),value==2,native_saved_value("fast_output",1)!=0));}
typedef int (__fastcall *NativePopup)(int,DWORD*,int);
static int __fastcall native_resolution_open(void *item,void *unused){BYTE *base=(BYTE*)GetModuleHandleW(NULL);*(void**)(base+0x1cd224)=native_resolution_selected;trace("Native Race Resolution menu opened");((NativePopup)(base+0x3a4e0))(8,&nativeResolutionRows[0][0],native_saved_selection()+1);return 0;}
static int __fastcall native_mode_open(void *item,void *unused){BYTE *base=(BYTE*)GetModuleHandleW(NULL);*(void**)(base+0x1cd224)=native_mode_selected;trace("Native Display menu opened");((NativePopup)(base+0x3a4e0))(4,&nativeModeRows[0][0],native_saved_value("windowed",0)?2:1);return 0;}
static void native_build_rows(DWORD (*rows)[14],int options,const DWORD *original){
 BYTE *base=(BYTE*)GetModuleHandleW(NULL);memcpy(rows[0],original,56);rows[0][0]=0;rows[0][3]=316;rows[0][4]=198;rows[0][5]=300;
 for(int i=1;i<=options;i++){
  memcpy(rows[i],original+14,56);rows[i][0]=0;rows[i][1]=i;rows[i][2]=89;rows[i][3]=316;rows[i][4]=230+32*(i-1);rows[i][5]=300;
  BYTE *nav=(BYTE*)&rows[i][7];int previous=i==1?options:i-1,next=i==options?1:i+1;nav[0]=nav[1]=(BYTE)i;nav[2]=nav[4]=(BYTE)previous;nav[3]=nav[5]=(BYTE)next;
  rows[i][9]|=0x100000;rows[i][10]=1;rows[i][11]=(DWORD)(base+0x5afb0);rows[i][12]=0;
 }
}
static BOOL native_settings_init(void){
 BYTE *base=(BYTE*)GetModuleHandleW(NULL);DWORD *resolution=(DWORD*)(base+0x1a08d4),*mode=(DWORD*)(base+0x1a09b4);
 if(*resolution!=(DWORD)(base+0x5b3c0)||*mode!=(DWORD)(base+0x5b4b0)){trace("Native settings signature mismatch; refusing to patch descriptors");return FALSE;}
 if(!native_install_text(base)){trace("Native dynamic text signature mismatch");return FALSE;}
 native_build_rows(nativeResolutionRows,7,(DWORD*)(base+0x1a1458));native_build_rows(nativeModeRows,3,(DWORD*)(base+0x1a16c0));
 *resolution=(DWORD)native_resolution_open;*mode=(DWORD)native_mode_open;trace("Native Graphics and Sound callbacks installed; no overlay");return TRUE;
}
static int WINAPI native_load_string(HINSTANCE module,UINT id,LPSTR buffer,int capacity){
 const char *text=NULL;char help[512];
 if(id==73||id==991){
  if(id==73)lstrcpyA(help,"4:3 race rendering.\nSave, then restart to apply.");
  else lstrcpyA(help,"Fits desktop/window at 4:3.\nSave, then restart to apply.");text=help;
 }
 if(text&&native_chinese(module)){const WCHAR *cn=id==73?L"保持4:3比赛画面。\n选择即保存，重启生效。":L"保持比例，自动留黑边。\n选择即保存，重启生效。";WideCharToMultiByte(CP_ACP,0,cn,-1,help,sizeof(help),NULL,NULL);text=help;}
 if(!text)return rrNextLoadStringA(module,id,buffer,capacity);if(!buffer||capacity<=0)return 0;lstrcpynA(buffer,text,capacity);return lstrlenA(buffer);
}

/* Private verification only. Regular user launches never capture frames. */
static void native_menu_frame(const BITMAPINFO *info,const void *bits){
 static int enabled=-1;static DWORD first,last;
 if(enabled<0){char path[MAX_PATH];lstrcpyA(path,root);lstrcatA(path,"\\rr4k-menu-test.enabled");enabled=GetFileAttributesA(path)!=INVALID_FILE_ATTRIBUTES;}
 if(!enabled||!info||info->bmiHeader.biBitCount!=32)return;
 BYTE *base=(BYTE*)GetModuleHandleW(NULL);if(*(DWORD*)(base+0x1c4044)!=19||!*(DWORD*)(base+0x1c404c)){first=last=0;return;}
 DWORD now=GetTickCount();if(!first)first=now;if(now-first<500||now-last<500)return;last=now;
 BITMAPINFOHEADER header=info->bmiHeader;if(header.biWidth!=640||abs(header.biHeight)!=480)return;DWORD size=640*480*4,done;
 BITMAPFILEHEADER file={0};file.bfType=0x4d42;file.bfOffBits=sizeof(file)+sizeof(header);file.bfSize=file.bfOffBits+size;
 char path[MAX_PATH];lstrcpyA(path,root);lstrcatA(path,"\\rr4k-native-menu-frame.bmp");HANDLE h=CreateFileA(path,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
 if(h!=INVALID_HANDLE_VALUE){WriteFile(h,&file,sizeof(file),&done,NULL);WriteFile(h,&header,sizeof(header),&done,NULL);WriteFile(h,bits,size,&done,NULL);CloseHandle(h);}
}
