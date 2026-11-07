/* Never let a background/private-desktop instance restrict the user's cursor. */
typedef BOOL (WINAPI *RRNextClip)(const RECT*);static RRNextClip rrNextClipCursor=ClipCursor;
typedef int (WINAPI *RRNextShow)(BOOL);static RRNextShow rrNextShowCursor=ShowCursor;
static BOOL rrOwnCursorClip;
static BOOL rr_on_input_desktop(void){
 HDESK input=OpenInputDesktop(0,FALSE,DESKTOP_READOBJECTS);if(!input)return FALSE;
 WCHAR active[128]={0},own[128]={0};DWORD used=0;BOOL a=GetUserObjectInformationW(input,UOI_NAME,active,sizeof(active),&used),b=GetUserObjectInformationW(GetThreadDesktop(GetCurrentThreadId()),UOI_NAME,own,sizeof(own),&used);CloseDesktop(input);return a&&b&&!lstrcmpW(active,own);
}
static BOOL rr_cursor_allowed(BOOL sameDesktop,BOOL foregroundOwned){return sameDesktop&&foregroundOwned;}
static BOOL WINAPI rr_guard_clip(const RECT *rect){
 if(!rect){if(!rrOwnCursorClip)return TRUE;BOOL ok=rrNextClipCursor(NULL);if(ok)rrOwnCursorClip=FALSE;return ok;}
 DWORD owner=0;GetWindowThreadProcessId(GetForegroundWindow(),&owner);
 if(!rr_cursor_allowed(rr_on_input_desktop(),owner==GetCurrentProcessId())){static int n;if(!n++)trace("Suppressed background ClipCursor request");return TRUE;}
 BOOL ok=rrNextClipCursor(rect);if(ok)rrOwnCursorClip=TRUE;return ok;
}
static int WINAPI rr_guard_show(BOOL show){DWORD owner=0;GetWindowThreadProcessId(GetForegroundWindow(),&owner);if(!rr_cursor_allowed(rr_on_input_desktop(),owner==GetCurrentProcessId()))return show?0:-1;return rrNextShowCursor(show);}
static void rr_release_cursor(void){if(rrOwnCursorClip){rrNextClipCursor(NULL);rrOwnCursorClip=FALSE;}}
