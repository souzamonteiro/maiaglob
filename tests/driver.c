/* Raw-byte test transport; all pattern syntax goes through the generated frontend. */
#include "../maia_glob.h"
#include <string.h>
#include <stdlib.h>
#ifdef MAIA_GLOB_PROBE_NATIVE
#include <stdio.h>
#endif
static unsigned char pattern[256],name[4096],mask[256],row_a[257],row_b[257];
static MaiaGlobInstruction code[257]; static MaiaGlobRange ranges[256];
/* Harness-owned aligned allocations persist across calls and are reused. */
static void *probe_areas[5];
static void *probe_area(unsigned int index,unsigned int bytes){
    if(!probe_areas[index])probe_areas[index]=malloc(bytes);
    return probe_areas[index];
}
static MaiaGlobStorage *probe_storage_data(void){return (MaiaGlobStorage*)probe_area(0u,sizeof(MaiaGlobStorage));}
static MaiaGlobProgram *probe_program_data(void){return (MaiaGlobProgram*)probe_area(1u,sizeof(MaiaGlobProgram));}
static MaiaGlobLimits *probe_limits_data(void){return (MaiaGlobLimits*)probe_area(2u,sizeof(MaiaGlobLimits));}
static MaiaGlobWorkspace *probe_workspace_data(void){return (MaiaGlobWorkspace*)probe_area(3u,sizeof(MaiaGlobWorkspace));}
unsigned char *probe_pattern(void){return pattern;}
unsigned char *probe_name(void){return name;}
unsigned char *probe_mask(void){return mask;}
unsigned int probe_code_count(void){MaiaGlobProgram *p=probe_program_data();return p->code_count;}
unsigned int probe_range_count(void){MaiaGlobProgram *p=probe_program_data();return p->range_count;}
unsigned int probe_steps(void){MaiaGlobWorkspace *w=probe_workspace_data();return w->steps;}
unsigned int probe_memory(void){return (unsigned int)maia_glob_compile_workspace_size();}
int probe_op(unsigned int i){return code[i].op;}
unsigned int probe_first(unsigned int i){return code[i].first;}
unsigned int probe_count(unsigned int i){return code[i].count;}
unsigned int probe_range_first(unsigned int i){return ranges[i].first;}
unsigned int probe_range_last(unsigned int i){return ranges[i].last;}
int probe_compile(unsigned int n,unsigned int masked){
    MaiaGlobProgram *p=probe_program_data();MaiaGlobWorkspace *w=probe_workspace_data();MaiaGlobStorage *s=probe_storage_data();
    memset(p,0,sizeof(MaiaGlobProgram));w->steps=0u;
    maia_glob_default_limits(probe_limits_data());s->code=code;s->code_capacity=257u;
    s->ranges=ranges;s->range_capacity=256u;
    s->compiler_workspace=probe_area(4u,131072u);s->compiler_workspace_bytes=131072u;
    w->first=row_a;w->second=row_b;w->capacity=257u;
    return maia_glob_compile_utf8(pattern,n,masked?mask:0,probe_storage_data(),probe_limits_data(),probe_program_data());
}
int probe_match(unsigned int n,unsigned int flags,unsigned int max_steps){
    MaiaGlobProgram *p=probe_program_data();MaiaGlobLimits *l=probe_limits_data();
    p->flags=flags;l->max_steps=max_steps;
    return maia_glob_match_utf8(probe_program_data(),name,n,probe_workspace_data(),probe_limits_data());
}
#include "walker.c"
#include "contracts.c"
#ifdef MAIA_GLOB_PROBE_NATIVE
#include <stdio.h>
#include <stdlib.h>
static int digit(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;return -1;}
static int unhex(const char *hex,unsigned char *out,unsigned int capacity){
    unsigned int i,n;int a,b;if(strcmp(hex,"-")==0)return 0;
    n=(unsigned int)strlen(hex);if(n%2u || n/2u>capacity)return -1;
    for(i=0u;i<n;i+=2u){a=digit(hex[i]);b=digit(hex[i+1u]);if(a<0||b<0)return -1;out[i/2u]=(unsigned char)((a<<4)|b);}return (int)(n/2u);
}
int main(void){
    char line[11000],p[513],n[8193],m[513];unsigned int flags,steps,i;int pn,nn,mn,compiled,matched;
    if(probe_walk_tests()!=0){fprintf(stderr,"Walker failure %d\n",probe_walk_tests());return 4;}
    if(probe_walk_case_count()!=18u||probe_walk_negative_control()!=6){fprintf(stderr,"Walker coverage/negative control failure\n");return 6;}
    if(probe_contract_tests()!=0){fprintf(stderr,"Contract failure %d\n",probe_contract_tests());return 5;}
    while(fgets(line,sizeof(line),stdin)){
        if(sscanf(line,"%512s %8192s %512s %u %u",p,n,m,&flags,&steps)!=5)return 2;
        pn=unhex(p,pattern,256u);nn=unhex(n,name,4096u);mn=unhex(m,mask,256u);
        if(pn<0||nn<0||mn<0||(mn&&mn!=pn))return 3;
        compiled=probe_compile((unsigned int)pn,mn?1u:0u);
        matched=compiled==0?probe_match((unsigned int)nn,flags,steps):-1;
        printf("%d %d %u %u %u",compiled,matched,(*probe_workspace_data()).steps,(*probe_program_data()).code_count,(*probe_program_data()).range_count);
        if(compiled==0){
            for(i=0u;i<(*probe_program_data()).code_count;++i)printf(" %d,%u,%u",code[i].op,code[i].first,code[i].count);
            for(i=0u;i<(*probe_program_data()).range_count;++i)printf(" %u,%u",ranges[i].first,ranges[i].last);
        }
        printf("\n");
    }return 0;
}
#endif
