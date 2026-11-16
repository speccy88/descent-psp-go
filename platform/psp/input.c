/* PSP controls: flight actions are computed from a complete pad snapshot. */
#include <pspctrl.h>
#include <pspkernel.h>
#include <psputility.h>
#include <SDL/SDL.h>
#include <string.h>
#include <stdio.h>
#include "kconfig.h"
#include "game.h"
#include "window.h"
#include "newmenu.h"
#include "gr.h"
#include "key.h"
#include "timer.h"
extern int Automap_active;
extern void key_handler(SDL_KeyboardEvent*);
extern int HandleSystemKey(int);
static SceCtrlData pad;
int32_t psp_input_probe[12];
static unsigned previous,edges,blocked;
void psp_input_clear(void);
extern int psp_aspect_mode,psp_frame_cap;
static int deadzone=24,sensitivity=100,invert_pitch=0,invert_yaw=0;
enum{NONE,FWD,REV,LEFT,RIGHT,UP,DOWN,ROLL_L,ROLL_R,PRIMARY,SECONDARY,FLARE,CYCLE_P,CYCLE_S,BOMB,REAR};
static unsigned bits[]={PSP_CTRL_TRIANGLE,PSP_CTRL_CROSS,PSP_CTRL_SQUARE,PSP_CTRL_CIRCLE,PSP_CTRL_RTRIGGER,PSP_CTRL_UP,PSP_CTRL_DOWN,PSP_CTRL_LEFT,PSP_CTRL_RIGHT};
static int bindings[2][9]={{FWD,REV,LEFT,RIGHT,PRIMARY,FLARE,CYCLE_S,CYCLE_P,CYCLE_P},{UP,DOWN,ROLL_L,ROLL_R,SECONDARY,BOMB,REAR,CYCLE_S,CYCLE_S}};
static const char*actions[]={"None","Forward","Reverse","Slide left","Slide right","Slide up","Slide down","Roll left","Roll right","Primary fire","Secondary fire","Flare","Cycle primary","Cycle secondary","Bomb","Rear view"};
static const char*buttons[]={"Triangle","Cross","Square","Circle","R","Up","Down","Left","Right"};
static void save_settings(void){FILE*f=fopen("userdata/psp-controls.tmp","w");if(!f)return;fprintf(f,"1 %d %d %d %d\n",deadzone,sensitivity,invert_pitch,invert_yaw);for(int l=0;l<2;l++)for(int i=0;i<9;i++)fprintf(f,"%d\n",bindings[l][i]);fprintf(f,"%d %d\n",psp_aspect_mode,psp_frame_cap);if(fclose(f))return;remove("userdata/psp-controls.bak");rename("userdata/psp-controls.cfg","userdata/psp-controls.bak");rename("userdata/psp-controls.tmp","userdata/psp-controls.cfg");}
void psp_input_init(void){sceCtrlSetSamplingCycle(0);sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);FILE*f=fopen("userdata/psp-controls.cfg","r");if(!f)f=fopen("userdata/psp-controls.bak","r");if(f){int v,d,s,p,y,b[2][9],ok=1;if(fscanf(f,"%d%d%d%d%d",&v,&d,&s,&p,&y)!=5||v!=1||d<0||d>100||s<25||s>200||p<0||p>1||y<0||y>1)ok=0;for(int l=0;l<2;l++)for(int i=0;i<9;i++)if(fscanf(f,"%d",&b[l][i])!=1||b[l][i]<0||b[l][i]>REAR)ok=0;int aspect=0,cap=60;if(fscanf(f,"%d%d",&aspect,&cap)==2&&aspect>=0&&aspect<=2&&(cap==30||cap==60)){psp_aspect_mode=aspect;psp_frame_cap=cap;}fclose(f);if(ok){deadzone=d;sensitivity=s;invert_pitch=p;invert_yaw=y;memcpy(bindings,b,sizeof b);}}}
static void key(SDLKey code){SDL_KeyboardEvent e;memset(&e,0,sizeof e);e.keysym.sym=code;e.type=SDL_KEYDOWN;e.state=SDL_PRESSED;key_handler(&e);e.type=SDL_KEYUP;e.state=SDL_RELEASED;key_handler(&e);}
static void bindings_menu(void){int choice=0;for(;;){newmenu_item m[18];char names[18][60];memset(m,0,sizeof m);for(int i=0;i<18;i++){snprintf(names[i],60,"%s%s: %s",i>=9?"L + ":"",buttons[i%9],actions[bindings[i/9][i%9]]);m[i].type=NM_TYPE_MENU;m[i].text=names[i];}choice=newmenu_do1("PSP bindings","Cross cycles action; Circle returns",18,m,0,0,choice);if(choice<0)break;bindings[choice/9][choice%9]=(bindings[choice/9][choice%9]+1)%(REAR+1);}save_settings();}
static void settings(void){newmenu_item m[7];memset(m,0,sizeof m);m[0].type=NM_TYPE_SLIDER;m[0].text="Analog dead zone";m[0].min_value=0;m[0].max_value=8;m[0].value=deadzone/8;m[1].type=NM_TYPE_SLIDER;m[1].text="Aim speed";m[1].min_value=1;m[1].max_value=8;m[1].value=sensitivity/25;m[2].type=NM_TYPE_CHECK;m[2].text="Invert pitch";m[2].value=invert_pitch;m[3].type=NM_TYPE_CHECK;m[3].text="Invert yaw";m[3].value=invert_yaw;m[4].type=NM_TYPE_MENU;m[4].text="Remap buttons";m[5].type=NM_TYPE_NUMBER;m[5].text="View 0:4/3 1:raw 2:wide";m[5].min_value=0;m[5].max_value=2;m[5].value=psp_aspect_mode;m[6].type=NM_TYPE_CHECK;m[6].text="30 FPS fallback (off=60)";m[6].value=psp_frame_cap==30;int n=newmenu_do("PSP controls",0,7,m,0,0);psp_aspect_mode=m[5].value;psp_frame_cap=m[6].value?30:60;deadzone=m[0].value*8;sensitivity=m[1].value*25;invert_pitch=m[2].value;invert_yaw=m[3].value;if(n==4)bindings_menu();save_settings();}
static void quick_menu(void){const char*t[]={"Resume","Game options","PSP controls","Cockpit mode","Drop bomb","Save game","Load game","Controller help","End game"};newmenu_item m[9];memset(m,0,sizeof m);for(int i=0;i<9;i++){m[i].type=NM_TYPE_MENU;m[i].text=(char*)t[i];}int n=newmenu_do("Descent PSP Go","Paused",9,m,0,0);switch(n){case 1:HandleSystemKey(KEY_F2);break;case 2:settings();break;case 3:HandleSystemKey(KEY_F3);break;case 4:Controls.drop_bomb_count++;break;case 5:HandleSystemKey(KEY_ALTED+KEY_F2);break;case 6:HandleSystemKey(KEY_ALTED+KEY_F3);break;case 7:nm_messagebox("Controller",1,"OK","Analog: yaw / pitch\nTriangle / Cross: thrust\nSquare / Circle: slide\nR: fire; L+R: missile\nL+Triangle/Cross: up/down\nL+Square/Circle: roll\nD-pad: weapons / flare\nSelect: map; Start: menu\nSquare in text field: keyboard");break;case 8:HandleSystemKey(KEY_ESC);break;}psp_input_clear();}
void psp_poll(void){sceCtrlPeekBufferPositive(&pad,1);blocked&=pad.Buttons;pad.Buttons&=~blocked;edges|=pad.Buttons&~previous;unsigned pressed=pad.Buttons&~previous;previous=pad.Buttons;
 int flying=Game_wind && window_get_front()==Game_wind;
 if(flying){if(pressed&PSP_CTRL_START)quick_menu();return;}
 if(Automap_active){if(pressed&(PSP_CTRL_SELECT|PSP_CTRL_CIRCLE|PSP_CTRL_START)){key(SDLK_ESCAPE);psp_input_clear();}return;}
 if(pressed&PSP_CTRL_UP)key(SDLK_UP);if(pressed&PSP_CTRL_DOWN)key(SDLK_DOWN);if(pressed&PSP_CTRL_LEFT)key(SDLK_LEFT);if(pressed&PSP_CTRL_RIGHT)key(SDLK_RIGHT);if(pressed&PSP_CTRL_CROSS)key(SDLK_RETURN);if(pressed&PSP_CTRL_CIRCLE)key(SDLK_ESCAPE);if(pressed&PSP_CTRL_SQUARE)key(SDLK_F4);if(pressed&PSP_CTRL_TRIANGLE)key(SDLK_SPACE);if(pressed&PSP_CTRL_START)key(SDLK_PAUSE);
 edges=0;
}
static int axis(int v){v-=128;if(v>deadzone)v-=deadzone;else if(v< -deadzone)v+=deadzone;else return 0;return v*sensitivity/(128-deadzone);}
void psp_controls(int automap){int a[REAR+1]={0},hit[REAR+1]={0};int layer=!!(pad.Buttons&PSP_CTRL_LTRIGGER);for(int i=0;i<9;i++){int n=bindings[layer][i];a[n]|=!!(pad.Buttons&bits[i]);hit[n]|=!!(edges&bits[i]);}
 Controls.pitch_time=(int64_t)axis(pad.Ly)*FrameTime*(invert_pitch?-1:1)/200;
 Controls.heading_time=(int64_t)axis(pad.Lx)*FrameTime*(invert_yaw?-1:1)/100;
 Controls.forward_thrust_time=(a[FWD]-a[REV])*FrameTime;Controls.sideways_thrust_time=(a[RIGHT]-a[LEFT])*FrameTime;Controls.vertical_thrust_time=(a[UP]-a[DOWN])*FrameTime;Controls.bank_time=(a[ROLL_L]-a[ROLL_R])*FrameTime;
 Controls.fire_primary_state=a[PRIMARY]&&!a[SECONDARY];Controls.fire_secondary_state=a[SECONDARY];Controls.fire_primary_count+=hit[PRIMARY]&&!a[SECONDARY];Controls.fire_secondary_count+=hit[SECONDARY];Controls.fire_flare_count+=hit[FLARE];Controls.drop_bomb_count+=hit[BOMB];Controls.cycle_primary_count+=hit[CYCLE_P];Controls.cycle_secondary_count+=hit[CYCLE_S];Controls.rear_view_state=a[REAR];Controls.automap_state=!!(pad.Buttons&PSP_CTRL_SELECT);Controls.automap_count+=!!(edges&PSP_CTRL_SELECT);edges=0;
 int32_t v[11]={Controls.pitch_time,Controls.heading_time,Controls.forward_thrust_time,Controls.sideways_thrust_time,Controls.vertical_thrust_time,Controls.bank_time,Controls.fire_primary_state,Controls.fire_secondary_state,pad.Buttons,layer,FrameTime};memcpy(psp_input_probe,v,sizeof v);psp_input_probe[11]++;

}
void psp_input_clear(void){memset(&Controls,0,sizeof Controls);edges=0;sceCtrlPeekBufferPositive(&pad,1);previous=pad.Buttons;blocked=pad.Buttons;pad.Buttons=0;pad.Lx=pad.Ly=128;}
extern void psp_osk_frame(int);
void psp_text_entry(char*text,int maxlen){unsigned short in[128]={0},out[128]={0},desc[]={ 'P','i','l','o','t',' ','/',' ','s','a','v','e',' ','n','a','m','e',0};if(maxlen>127)maxlen=127;for(int i=0;text[i]&&i<maxlen;i++)in[i]=(unsigned char)text[i];SceUtilityOskData d;memset(&d,0,sizeof d);d.language=PSP_UTILITY_OSK_LANGUAGE_ENGLISH;d.lines=1;d.inputtype=PSP_UTILITY_OSK_INPUTTYPE_LATIN_LOWERCASE|PSP_UTILITY_OSK_INPUTTYPE_LATIN_DIGIT;d.desc=desc;d.intext=in;d.outtext=out;d.outtextlength=128;d.outtextlimit=maxlen;SceUtilityOskParams p;memset(&p,0,sizeof p);p.base.size=sizeof p;p.base.language=PSP_SYSTEMPARAM_LANGUAGE_ENGLISH;p.base.buttonSwap=PSP_UTILITY_ACCEPT_CROSS;p.base.graphicsThread=17;p.base.accessThread=19;p.base.fontThread=18;p.base.soundThread=16;p.datacount=1;p.data=&d;if(sceUtilityOskInitStart(&p)<0)return;int done=0;while(!done){int st=sceUtilityOskGetStatus();psp_osk_frame(st==PSP_UTILITY_DIALOG_VISIBLE);if(st==PSP_UTILITY_DIALOG_QUIT)sceUtilityOskShutdownStart();else if(st==PSP_UTILITY_DIALOG_NONE)done=1;}if(d.result!=PSP_UTILITY_OSK_RESULT_CANCELLED){int i;for(i=0;i<maxlen&&out[i];i++)text[i]=(char)out[i];text[i]=0;}psp_input_clear();}
