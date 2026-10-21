/* Deterministic callback fixtures, not a host-filesystem integration claim. */
typedef struct GlobFixture {
    unsigned int cursor[8]; unsigned int used[8]; unsigned int directory[8];
    unsigned int live,opened,closed,block_open,block_read; int fault;
} GlobFixture;
static int fixture_open(void *context,const unsigned char *path,unsigned int length) {
    GlobFixture *f=(GlobFixture*)context;unsigned int directory,i;
    if(f->block_open){--f->block_open;return -6;}
    directory=0u;
    if((length==3u && memcmp(path,"src",3u)==0)||(length==4u && memcmp(path,"src/",4u)==0))directory=1u;
    else if(length==7u && memcmp(path,"src/sub",7u)==0)directory=2u;
    else if(!((length==1u && (path[0]=='.'||path[0]=='/'))||(length==2u && memcmp(path,"./",2u)==0)))return -2;
    for(i=0u;i<8u;++i)if(!f->used[i]){f->used[i]=1u;f->directory[i]=directory;f->cursor[i]=0u;++f->live;++f->opened;return (int)i+1;}
    return -4;
}
static int fixture_read(void *context,int handle,unsigned char *name,unsigned int capacity,unsigned int *type) {
    GlobFixture *f=(GlobFixture*)context;unsigned int at,dir,n;const char *value;
    if(handle<=0 || handle>8 || !f->used[handle-1])return -1;
    if(f->block_read){--f->block_read;return -6;}
    if(f->fault)return f->fault;
    at=f->cursor[handle-1]++;dir=f->directory[handle-1];*type=1u;value=0;
    if(dir==0u){if(at==0u)value="a.c";else if(at==1u)value="b.txt";else if(at==2u)value=".hidden";else if(at==3u){value="src";*type=2u;}else if(at==4u)value="two words.c";}
    else if(dir==1u){if(at==0u)value="x.c";else if(at==1u){value="sub";*type=2u;}}
    else if(at==0u)value="deep.c";
    if(!value)return 0;n=(unsigned int)strlen(value);if(n>=capacity)return -4;
    memcpy(name,value,n);return (int)n;
}
static int fixture_close(void *context,int handle){GlobFixture *f=(GlobFixture*)context;
    if(handle<=0||handle>8||!f->used[handle-1])return -1;
    f->used[handle-1]=0u;--f->live;++f->closed;return 0;
}
static unsigned int walker_case_count=0u;
static int walker_last_status=0;
static char walker_last_output[4096];
int probe_walk_last_status(void){return walker_last_status;}
const char *probe_walk_last_output(void){return walker_last_output;}
unsigned int probe_walk_case_count(void){return walker_case_count;}
static int walker_case(const char *text,const char *expected,int expected_status,unsigned int option) {
    MaiaGlobWalker walker;MaiaGlobFs fs;GlobFixture fixture;MaiaGlobLimits l;
    MaiaGlobProgram *p=probe_program_data();unsigned char output[1024];char joined[4096];
    unsigned int length,type,rounds,used,small;int status;
    ++walker_case_count;
    memcpy(pattern,text,strlen(text));if(probe_compile((unsigned int)strlen(text),0u)!=0)return 1;
    memset(&fixture,0,sizeof(fixture));fixture.block_open=1u;fixture.block_read=1u;
    fs.context=&fixture;fs.open=fixture_open;fs.read=fixture_read;fs.close=fixture_close;
    maia_glob_default_limits(&l);if(option==1u)l.max_entries=1u;if(option==2u)l.max_matches=1u;
    if(option==3u)l.max_depth=1u;if(option==4u)l.max_directories=1u;
    if(option==5u)l.max_result_bytes=2u;if(option==6u)l.max_walk_steps=2u;
    if(option==7u)fixture.fault=-1;if(option==8u)fixture.fault=-3;
    if(maia_glob_walk_begin(&walker,p,&fs,&l)!=0)return 2;
    rounds=0u;used=0u;small=1u;joined[0]=0;
    do{
        length=777u;type=999u;output[0]=0x55u;
        status=maia_glob_walk_step(&walker,output,small?1u:1024u,&length,&type,1u);
        if(status==8){if(length!=777u||type!=999u||output[0]!=0x55u)return 3;small=0u;continue;}
        if(status==6){if(length>=1024u||output[length]||used+length+2u>=4096u)return 4;
            memcpy(joined+used,output,length);used+=length;joined[used++]='|';joined[used]=0;small=1u;}
        if(++rounds>1000u)return 5;
    }while(status==5||status==6||status==8);
    walker_last_status=status;memcpy(walker_last_output,joined,used+1u);
    if(status!=expected_status||strcmp(joined,expected)!=0){
#ifdef MAIA_GLOB_PROBE_NATIVE
        fprintf(stderr,"%s: status %d expected %d, paths %s expected %s\n",text,status,expected_status,joined,expected);
#endif
        return 6;}
    if(fixture.live||fixture.opened!=fixture.closed)return 7;
    maia_glob_walk_cancel(&walker);maia_glob_walk_cancel(&walker);
    if(fixture.live)return 8;return 0;
}
int probe_walk_tests(void){int r;unsigned int i;MaiaGlobWalker w;MaiaGlobFs fs;MaiaGlobLimits l;GlobFixture f;
    walker_case_count=0u;
    r=walker_case("","",7,0u);if(r)return 100+r;
    r=walker_case("*.c","a.c|two words.c|",7,0u);if(r)return 200+r;
    r=walker_case(".*",".hidden|",7,0u);if(r)return 300+r;
    r=walker_case("src/*.c","src/x.c|",7,0u);if(r)return 400+r;
    r=walker_case("src/*/*.c","src/sub/deep.c|",7,0u);if(r)return 500+r;
    r=walker_case("src/","src/|",7,0u);if(r)return 600+r;
    r=walker_case("/","/|",7,0u);if(r)return 700+r;
    r=walker_case(".",".|",7,0u);if(r)return 800+r;
    r=walker_case("./","./|",7,0u);if(r)return 900+r;
    r=walker_case("missing/*","",7,0u);if(r)return 1000+r;
    r=walker_case("*","a.c|",3,1u);if(r)return 1100+r;
    r=walker_case("*.c","a.c|",3,2u);if(r)return 1200+r;
    r=walker_case("src/*","",3,3u);if(r)return 1300+r;
    r=walker_case("src/*","",3,4u);if(r)return 1400+r;
    r=walker_case("*","",3,5u);if(r)return 1500+r;
    r=walker_case("*","",3,6u);if(r)return 1600+r;
    r=walker_case("*","",4,7u);if(r)return 1700+r;
    r=walker_case("*","",7,8u);if(r)return 1800+r;
    for(i=0u;i<100u;++i){
        memcpy(pattern,"*",1u);if(probe_compile(1u,0u)!=0)return 9001;
        memset(&f,0,sizeof(f));fs.context=&f;fs.open=fixture_open;fs.read=fixture_read;fs.close=fixture_close;
        maia_glob_default_limits(&l);if(maia_glob_walk_begin(&w,probe_program_data(),&fs,&l)!=0)return 9002;
        {unsigned char out[1024];unsigned int n,t;maia_glob_walk_step(&w,out,1024u,&n,&t,1u);}
        maia_glob_walk_cancel(&w);if(f.live||f.opened!=f.closed)return 9003;
    }
    return 0;
}

int probe_walk_negative_control(void){return walker_case("*.c","WRONG",7,0u);}
