#include <assert.h>
#include <stdint.h>
#include <limits.h>
#include "maths.h"
int main(void){int32_t a[]={INT_MIN,INT_MIN+1,-65537,-65536,-1,0,1,32768,65535,65536,INT_MAX};for(unsigned i=0;i<sizeof a/sizeof*a;i++)for(unsigned j=0;j<sizeof a/sizeof*a;j++){int64_t prod=(int64_t)a[i]*a[j];assert(fixmul(a[i],a[j])==(int32_t)(prod/65536));assert(fixdiv(a[i],a[j])==(a[j]?(int32_t)((int64_t)a[i]*65536/a[j]):1));quadint q={0x98765432,-123};uint64_t expected=((uint64_t)(uint32_t)q.high<<32)|q.low;expected+=(uint64_t)prod;fixmulaccum(&q,a[i],a[j]);assert(q.low==(uint32_t)expected&&(uint32_t)q.high==(uint32_t)(expected>>32));fixquadnegate(&q);expected=0-expected;assert(q.low==(uint32_t)expected&&(uint32_t)q.high==(uint32_t)(expected>>32));}assert(i2f(-1)==-65536);return 0;}
