/* Verified old tables: 0x5ac768 and 0x5adde8, each 480 records of 12 bytes.
   Rewrite only HIGHLOW relocations in executable code that point into them. */
static BYTE *rr_guarded_rows(DWORD rows) {
 DWORD bytes=rows*12,aligned=(bytes+4095)&~4095;BYTE *p=VirtualAlloc(NULL,aligned+4096,MEM_RESERVE,PAGE_NOACCESS);
 if(!p||!VirtualAlloc(p,aligned,MEM_COMMIT,PAGE_READWRITE))return NULL;
 return p+aligned-bytes;
}
static void rr_native_edges(void) {
 if(!nativeProjection2x)return;
 BYTE *base=(BYTE*)GetModuleHandleW(NULL),*left=rr_guarded_rows(nativeHeight),*right=rr_guarded_rows(nativeHeight);
 if(!left||!right){trace("Native edge allocation failed");return;}
 IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER*)base;IMAGE_NT_HEADERS *nt=(IMAGE_NT_HEADERS*)(base+dos->e_lfanew);
 IMAGE_DATA_DIRECTORY dir=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];DWORD count=0;
 for(DWORD offset=0;offset<dir.Size;){
  IMAGE_BASE_RELOCATION *block=(IMAGE_BASE_RELOCATION*)(base+dir.VirtualAddress+offset);if(block->SizeOfBlock<8||offset+block->SizeOfBlock>dir.Size)break;
  WORD *items=(WORD*)(block+1);DWORD n=(block->SizeOfBlock-8)/2;
  for(DWORD i=0;i<n;i++)if((items[i]>>12)==IMAGE_REL_BASED_HIGHLOW){
   DWORD rva=block->VirtualAddress+(items[i]&4095);if(rva<0x1000||rva>=0x103000)continue;
   DWORD *where=(DWORD*)(base+rva),value=*where-(DWORD)base;BYTE *replacement=NULL;
   if(value>=0x1ac768&&value<0x1adde8)replacement=left+(value-0x1ac768);
   else if(value>=0x1adde8&&value<0x1af468)replacement=right+(value-0x1adde8);
   if(replacement){DWORD old;if(VirtualProtect(where,4,PAGE_EXECUTE_READWRITE,&old)){*where=(DWORD)replacement;VirtualProtect(where,4,old,&old);FlushInstructionCache(GetCurrentProcess(),where,4);count++;}}
  }
  offset+=block->SizeOfBlock;
 }
 char line[192];wsprintfA(line,"Native edge tables: rows=%u bytesEach=%u left=%08x right=%08x patchedReferences=%u endGuardPages=yes",nativeHeight,nativeHeight*12,(DWORD)left,(DWORD)right,count);trace(line);
 BYTE *limit=base+0x2e574;DWORD old;
 if(count==18&&limit[0]==0xba&&*(DWORD*)(limit+1)==479&&VirtualProtect(limit,5,PAGE_EXECUTE_READWRITE,&old)){
  *(DWORD*)(limit+1)=nativeHeight-1;VirtualProtect(limit,5,old,&old);FlushInstructionCache(GetCurrentProcess(),limit,5);trace("Texture edge writer row limit raised with guarded tables");
 }
}
