/* Recoverable userdata writes. FAT rename is not power-loss atomic. */
#include <physfs.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
static struct{PHYSFS_File*f;char name[512],pending[540];int failed;} writes[16];
PHYSFS_File*psp_open_write(const char*name){if(!name||strlen(name)>500||strstr(name,"..")||strchr(name,':')||name[0]=='/')return NULL;int i;for(i=0;i<16&&writes[i].f;i++);if(i==16)return NULL;snprintf(writes[i].name,512,"%s",name);snprintf(writes[i].pending,540,"%s.pending",name);writes[i].failed=0;writes[i].f=PHYSFS_openWrite(writes[i].pending);return writes[i].f;}
PHYSFS_sint64 psp_write(PHYSFS_File*f,const void*b,PHYSFS_uint32 size,PHYSFS_uint32 count){PHYSFS_sint64 n=PHYSFS_write(f,b,size,count);if(n!=count)for(int i=0;i<16;i++)if(writes[i].f==f)writes[i].failed=1;return n;}
int psp_close(PHYSFS_File*f){int i;for(i=0;i<16&&writes[i].f!=f;i++);if(i==16)return PHYSFS_close(f);int good=PHYSFS_flush(f)&&!writes[i].failed;int closed=PHYSFS_close(f);writes[i].f=NULL;if(!closed||!good){fprintf(stderr,"Write failed: %s; previous file retained\n",writes[i].name);return 0;}char target[1100],temp[1100],backup[1120];snprintf(target,sizeof target,"%s/%s",PHYSFS_getWriteDir(),writes[i].name);snprintf(temp,sizeof temp,"%s/%s",PHYSFS_getWriteDir(),writes[i].pending);snprintf(backup,sizeof backup,"%s.bak",target);int existed=access(target,F_OK)==0;if(existed){remove(backup);if(rename(target,backup))return 0;}if(rename(temp,target)){if(existed)rename(backup,target);return 0;}return 1;}
