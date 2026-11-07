/* IDirectDraw v1 hooks: original methods belong to each interface object.
   No original COM method runs under our bookkeeping lock. */
typedef ULONG (WINAPI *RRDrawRelease)(void*);
typedef struct RRDrawHook {
 struct RRDrawHook *next;void *object;RRCreateSurface create;RRCoop cooperative;RRDrawRelease release;void *table[23];
} RRDrawHook;
static RRDrawHook *rrDrawHooks;
static SRWLOCK rrDrawHookLock=SRWLOCK_INIT;
static HRESULT WINAPI rr_create_surface(void*,DDSURFACEDESC*,void**,IUnknown*);
static HRESULT WINAPI rr_coop(void*,HWND,DWORD);
static RRDrawHook *rr_draw_locked(void *object){
 for(RRDrawHook *h=rrDrawHooks;h;h=h->next)if(h->object==object&&*(void***)object==h->table)return h;
 return NULL;
}
static BOOL rr_draw_originals(void *object,RRCreateSurface *create,RRCoop *cooperative){
 if(!object)return FALSE;AcquireSRWLockShared(&rrDrawHookLock);RRDrawHook *h=rr_draw_locked(object);
 if(h){if(create)*create=h->create;if(cooperative)*cooperative=h->cooperative;}
 ReleaseSRWLockShared(&rrDrawHookLock);return h!=NULL;
}
static ULONG WINAPI rr_release_draw(void *object){
 DWORD incoming=GetLastError();if(!object)return 0;
 AcquireSRWLockShared(&rrDrawHookLock);RRDrawHook *h=rr_draw_locked(object);RRDrawRelease release=h?h->release:NULL;ReleaseSRWLockShared(&rrDrawHookLock);
 if(!release){SetLastError(incoming);return 0;}
 SetLastError(incoming);ULONG remaining=release(object);DWORD outgoing=GetLastError();
 if(!remaining){
  /* The COM object can already be freed: never read it after Release returns 0. */
  AcquireSRWLockExclusive(&rrDrawHookLock);RRDrawHook **link=&rrDrawHooks;while(*link&&*link!=h)link=&(*link)->next;if(*link)*link=h->next;ReleaseSRWLockExclusive(&rrDrawHookLock);
  HeapFree(GetProcessHeap(),0,h);
 }
 SetLastError(outgoing);return remaining;
}
static HRESULT rr_probe_draw(void *object){
 DWORD incoming=GetLastError();if(!object)return E_INVALIDARG;HRESULT result=S_OK;
 AcquireSRWLockExclusive(&rrDrawHookLock);
 if(rr_draw_locked(object))goto done; /* Idempotent: do not save our wrappers as originals. */
 for(RRDrawHook *h=rrDrawHooks;h;h=h->next)if(h->object==object){result=E_UNEXPECTED;goto done;}
 void **original=*(void***)object;
 if(!original||!original[2]||!original[6]||!original[20]){result=E_INVALIDARG;goto done;}
 if(original[2]==rr_release_draw||original[6]==rr_create_surface||original[20]==rr_coop){result=E_UNEXPECTED;goto done;}
 RRDrawHook *h=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*h));if(!h){result=E_OUTOFMEMORY;goto done;}
 h->object=object;memcpy(h->table,original,sizeof(h->table));h->release=(RRDrawRelease)original[2];h->create=(RRCreateSurface)original[6];h->cooperative=(RRCoop)original[20];h->table[2]=rr_release_draw;h->table[6]=rr_create_surface;h->table[20]=rr_coop;
 if(InterlockedCompareExchangePointer((void *volatile*)object,h->table,original)!=original){HeapFree(GetProcessHeap(),0,h);result=E_UNEXPECTED;goto done;}
 h->next=rrDrawHooks;rrDrawHooks=h;
 done:ReleaseSRWLockExclusive(&rrDrawHookLock);SetLastError(incoming);return result;
}
