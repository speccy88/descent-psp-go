/* Offline PCM synthesis using TinySoundFont, MIT (schellingb). */
#include <stdio.h>
#include <stdint.h>
#define TSF_IMPLEMENTATION
#include "tsf.h"
#define TML_IMPLEMENTATION
#include "tml.h"
static void le32(FILE*f,uint32_t n){for(int i=0;i<4;i++)fputc(n>>(i*8),f);}
int main(int argc,char**argv){if(argc!=4)return 2;tml_message*head=tml_load_filename(argv[1]),*m=head;if(!m)return 3;tsf*s=tsf_load_filename(argv[2]);if(!s)return 4;tsf_set_output(s,TSF_STEREO_INTERLEAVED,44100,-10.0f);tsf_channel_set_bank_preset(s,9,128,0);FILE*f=fopen(argv[3],"wb");if(!f)return 5;fwrite("RIFF",1,4,f);le32(f,0);fwrite("WAVEfmt ",1,8,f);le32(f,16);fwrite("\1\0\2\0",1,4,f);le32(f,44100);le32(f,176400);fwrite("\4\0\20\0data",1,8,f);le32(f,0);uint64_t frames=0;short pcm[128];unsigned end=0;for(tml_message*t=m;t;t=t->next)end=t->time;while(frames*1000/44100 < end+1200){double ms=frames*1000.0/44100;while(m&&m->time<=ms){switch(m->type){case TML_PROGRAM_CHANGE:tsf_channel_set_presetnumber(s,m->channel,m->program,m->channel==9);break;case TML_NOTE_ON:tsf_channel_note_on(s,m->channel,m->key,m->velocity/127.f);break;case TML_NOTE_OFF:tsf_channel_note_off(s,m->channel,m->key);break;case TML_PITCH_BEND:tsf_channel_set_pitchwheel(s,m->channel,m->pitch_bend);break;case TML_CONTROL_CHANGE:tsf_channel_midi_control(s,m->channel,m->control,m->control_value);break;}m=m->next;}tsf_render_short(s,pcm,64,0);if(fwrite(pcm,4,64,f)!=64)return 6;frames+=64;}fseek(f,4,SEEK_SET);le32(f,36+frames*4);fseek(f,40,SEEK_SET);le32(f,frames*4);fclose(f);tsf_close(s);tml_free(head);printf("%s: %.3f seconds\n",argv[3],frames/44100.0);return 0;}
