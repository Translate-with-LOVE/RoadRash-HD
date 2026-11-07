/* Layout and pointer coordinates must not inherit the bitmap's pixel dimensions. */
static DWORD uiCoordinateWidth=640,uiCoordinateHeight=480;
static void ui_coordinate_dimensions(DWORD width,DWORD height){uiCoordinateWidth=width;uiCoordinateHeight=height;}
static BOOL ui_install_coordinates(BYTE *base){
 if(!uiEnabled)return TRUE;
 const struct {DWORD operand,target;BYTE prefix[3];int length;} changes[]={
  {0x6e274,0x1ce85c,{0x0f,0xaf,0x0d},3},{0x6e293,0x1ce860,{0x0f,0xaf,0x35},3},
  {0x6ea4f,0x1ce85c,{0x8b,0x35},2},{0x6ea65,0x1ce85c,{0x89,0x35},2},
  {0x6ea6b,0x1ce860,{0x8b,0x0d},2},{0x6ea7b,0x1ce860,{0x89,0x0d},2},
  {0x6eac4,0x1ce85c,{0x89,0x35},2},{0x6ead4,0x1ce860,{0x89,0x0d},2},
  {0x6eb71,0x1ce85c,{0x8b,0x3d},2},{0x6eb87,0x1ce85c,{0x89,0x3d},2},
  {0x6eb8d,0x1ce860,{0x8b,0x0d},2},{0x6eb9d,0x1ce860,{0x89,0x0d},2}
 };
 for(int i=0;i<12;i++){
  BYTE *p=base+changes[i].operand;
  if(*(DWORD*)p!=(DWORD)(base+changes[i].target)||memcmp(p-changes[i].length,changes[i].prefix,changes[i].length)){
   trace("Frontend coordinate signature mismatch; refusing to start");return FALSE;
  }
 }
 for(int i=0;i<12;i++){
  DWORD *p=(DWORD*)(base+changes[i].operand),prior;
  if(!VirtualProtect(p,4,PAGE_EXECUTE_READWRITE,&prior))return FALSE;
  *p=(DWORD)(changes[i].target==0x1ce85c?&uiCoordinateWidth:&uiCoordinateHeight);
  VirtualProtect(p,4,prior,&prior);
 }
 FlushInstructionCache(GetCurrentProcess(),NULL,0);
 trace("Frontend point/rectangle/inverse mapping decoupled from render pixels: 12 operands");return TRUE;
}
