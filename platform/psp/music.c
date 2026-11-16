/* Bounded PCM music producer; SDL owns the only audio output worker. */
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <stdio.h>
#include <stdint.h>
#include <stdatomic.h>
#include <string.h>
#include "digi_mixer_music.h"
#include "startup.h"
#define CAP 32768
static int16_t ring[CAP*2];
static unsigned rd,wr;static _Atomic unsigned used;static _Atomic int active,paused,volume=8,eof;static int repeat;
static FILE*file;static SDL_Thread*worker;static void(*finished)(void);
static _Atomic unsigned underruns,blocks;static uint32_t bytes_left,pcm_bytes,loop_start,loop_end;
static uint32_t le32(const unsigned char*p){return p[0]|(p[1]<<8)|(p[2]<<16)|((uint32_t)p[3]<<24);}
static int reader(void*p){int16_t buf[2048];while(active){unsigned room=CAP-atomic_load(&used);if(room<1024||paused){SDL_Delay(4);continue;}unsigned n=bytes_left/4;if(n>1024)n=1024;if(!n){if(repeat){fseek(file,44+loop_start*4,SEEK_SET);bytes_left=(loop_end-loop_start)*4;continue;}eof=1;break;}unsigned got=fread(buf,4,n,file);if(!got){eof=1;break;}bytes_left-=got*4;for(unsigned i=0;i<got;i++){ring[wr*2]=buf[i*2];ring[wr*2+1]=buf[i*2+1];wr=(wr+1)%CAP;}atomic_fetch_add(&used,got);}return 0;}
static void mix(void*p,Uint8*stream,int bytes){memset(stream,0,bytes);if(paused||!active)return;int16_t*out=(int16_t*)stream;unsigned n=bytes/4;blocks++;if(used<n&&!eof)underruns++;if(n>used)n=used;for(unsigned i=0;i<n;i++){out[i*2]=ring[rd*2]*volume/8;out[i*2+1]=ring[rd*2+1]*volume/8;rd=(rd+1)%CAP;}atomic_fetch_sub(&used,n);}
void mix_free_music(void)
{
    PSP_BOOT_CHECKPOINT("music: detaching mixer callback");
    Mix_HookMusic(NULL, NULL);
    active = 0;
    if (worker) {
        PSP_BOOT_CHECKPOINT("music: waiting for reader thread");
        SDL_WaitThread(worker, 0);
        worker = 0;
        PSP_BOOT_CHECKPOINT("music: reader thread stopped");
    }
    if (file) {
        fclose(file);
        file = 0;
    }
    used = rd = wr = 0;
    finished = 0;
    PSP_BOOT_CHECKPOINT("music: stopped");
}
int mix_play_file(char*filename,int loop,void(*hook)(void)){mix_free_music();char name[128],path[160];snprintf(name,sizeof name,"%s",filename);char*dot=strrchr(name,'.');if(!dot||strchr(name,'/')||strchr(name,'\\'))return 0;*dot=0;snprintf(path,sizeof path,"music/%s.wav",name);PSP_BOOT_CHECKPOINT("music: opening PCM track");PSP_BOOT_STAGE(path);file=fopen(path,"rb");if(!file){printf("Music asset absent: %s\n",path);return 0;}unsigned char h[44];if(fread(h,1,44,file)!=44||memcmp(h,"RIFF",4)||memcmp(h+8,"WAVEfmt ",8)||le32(h+16)!=16||h[20]!=1||h[22]!=2||le32(h+24)!=44100||h[34]!=16||memcmp(h+36,"data",4)){fclose(file);file=0;return 0;}pcm_bytes=bytes_left=le32(h+40);if(!pcm_bytes||pcm_bytes%4){fclose(file);file=0;return 0;}loop_start=0;loop_end=pcm_bytes/4;
 char lp[160];snprintf(lp,sizeof lp,"music/%s.loop",name);FILE*lf=fopen(lp,"r");if(lf){unsigned a,b;if(fscanf(lf,"%u%u",&a,&b)==2&&a<b&&b<=pcm_bytes/4){loop_start=a;loop_end=b;}fclose(lf);}
 repeat=loop;if(repeat)bytes_left=loop_end*4;eof=paused=0;finished=hook;active=1;PSP_BOOT_CHECKPOINT("music: starting reader thread");worker=SDL_CreateThread(reader,0);if(!worker){mix_free_music();return 0;}/* Prefill outside real-time worker and render loop. */
 for(int i=0;i<500&&used<4096&&!eof;i++)SDL_Delay(1);
 PSP_BOOT_CHECKPOINT("music: reader prefilled; attaching callback");Mix_HookMusic(mix,NULL);printf("Music stream: %s bytes=%u\n",path,pcm_bytes);return 1;}
void psp_music_service(void){if(active&&eof&&!used){void(*f)(void)=finished;mix_free_music();if(f)f();}}
void psp_music_stats(void){printf("Music blocks=%u starvation=%u ring_frames=%u\n",blocks,underruns,used);}
void mix_set_music_volume(int v){volume=v<0?0:v>8?8:v;}
void mix_stop_music(void){mix_free_music();}
void mix_pause_music(void){paused=1;}
void mix_resume_music(void){paused=0;}
void mix_pause_resume_music(void){paused=!paused;}
