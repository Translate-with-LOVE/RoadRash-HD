/* Opt-in private-desktop input script. No system keyboard or cursor events. */
static BOOL raceTestEnabled;
static BOOL trialMode;
static DWORD raceStartTick;
static DWORD nativeWidth,nativeHeight;
static BOOL nativeProjection2x;
static BOOL nativeRational;
static BOOL fastOutput;
static BOOL configuredWindowed;
static void race_init(void) {
 char flag[MAX_PATH];lstrcpyA(flag,root);lstrcatA(flag,"\\rr4k-race-test.enabled");
 raceTestEnabled=GetFileAttributesA(flag)!=INVALID_FILE_ATTRIBUTES;raceStartTick=GetTickCount();
 lstrcpyA(flag,root);lstrcatA(flag,"\\rr4k-trial.enabled");trialMode=GetFileAttributesA(flag)!=INVALID_FILE_ATTRIBUTES;
 lstrcpyA(flag,root);lstrcatA(flag,"\\rr4k-native-test.ini");
 if(trialMode&&!raceTestEnabled)rr_display_initialize(flag,&nativeWidth,&nativeHeight,&configuredWindowed,&fastOutput);
 else{
 configuredWindowed=GetPrivateProfileIntA("render","windowed",0,flag)!=0;
 fastOutput=GetPrivateProfileIntA("render","fast_output",1,flag)!=0;
 if(raceTestEnabled||trialMode){nativeWidth=GetPrivateProfileIntA("render","width",0,flag);nativeHeight=GetPrivateProfileIntA("render","height",0,flag);if(nativeWidth<640||nativeWidth>3840||nativeHeight<480||nativeHeight>2160||nativeWidth%2||nativeHeight%2)nativeWidth=nativeHeight=0;}
 }
 char forced[4];DWORD forcedCount=GetEnvironmentVariableA("RR_FORCE_ORIGINAL",forced,sizeof(forced));if(forcedCount>0&&forcedCount<sizeof(forced)&&forced[0]=='1')nativeWidth=nativeHeight=0;
 nativeProjection2x=nativeWidth>=640;
 nativeRational=nativeProjection2x;
 if(nativeWidth&&!nativeProjection2x){BYTE *base=(BYTE*)GetModuleHandleW(NULL);*(DWORD*)(base+0x191c04)=nativeWidth/2;*(DWORD*)(base+0x191c08)=nativeHeight/2;*(DWORD*)(base+0x191c0c)=nativeWidth/2;trace("Native base dimensions set before game initialization");}

 if(trialMode)trace("Interactive isolated trial; real input; F10 or close exits this process only");
 /* Keep original credits step/timer in interactive and automated sessions. */

}
static BOOL race_key_down(int key) {
 DWORD t=GetTickCount()-raceStartTick;
 if(key==VK_RETURN)return t>=4000&&t<31500&&t%5000<1500;
 if(key=='W')return t>=20000&&t<90000;
 if(key=='A')return t>=32000&&t<33000;
 if(key=='D')return t>=34000&&t<35000;
 return FALSE;
}
static BOOL WINAPI race_keyboard(PBYTE keys) {
 if(!raceTestEnabled)return GetKeyboardState(keys);
 ZeroMemory(keys,256);for(int k=0;k<256;k++)if(race_key_down(k))keys[k]=0x80;
 static int calls;calls++;if(calls==1)trace("Race input: GetKeyboardState polled");
 return TRUE;
}
static SHORT WINAPI race_async_key(int key) {
 if(!raceTestEnabled)return GetAsyncKeyState(key);
 static int calls;calls++;if(calls==1)trace("Race input: GetAsyncKeyState polled");
 return race_key_down(key)?(SHORT)0x8001:0;
}
static LRESULT WINAPI race_dispatch(const MSG *message) {
 static int once;if(!once++){char line[96];wsprintfA(line,"Dispatch hook active: trial=%d test=%d",trialMode,raceTestEnabled);trace(line);}
 if(trialMode&&(message->message==WM_CLOSE||((message->message==WM_KEYDOWN||message->message==WM_SYSKEYDOWN)&&message->wParam==VK_F10)||(message->message==WM_SYSCOMMAND&&(message->wParam&0xfff0)==SC_CLOSE))){trace("Trial exit requested; no shared state saved");rr_startup_intentional();rr_startup_exit(0);ExitProcess(0);}
 if(raceTestEnabled&&message->message==WM_TIMER&&message->wParam==7&&*(DWORD*)((BYTE*)GetModuleHandleW(NULL)+0x1c4044)!=7)return 0;
 return DispatchMessageA(message);
}
