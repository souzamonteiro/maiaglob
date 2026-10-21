/* Invalid API inputs, caller ownership and precise storage bounds. */
int probe_contract_tests(void){
    MaiaGlobProgram *pp=probe_program_data();MaiaGlobStorage *sp=probe_storage_data();
    MaiaGlobProgram p,original;MaiaGlobStorage s;MaiaGlobLimits l;MaiaGlobWorkspace w;
    MaiaGlobInstruction instructions[3];MaiaGlobRange r[1];unsigned char a[4],b[4];
    unsigned char badmask[2];int status;
    maia_glob_default_limits(&l);memset(&p,0,sizeof(MaiaGlobProgram));
    if(maia_glob_validate(&p)!=2)return 1;
    instructions[0].op=3;instructions[0].first=0u;instructions[0].count=0u;
    instructions[1].op=6;instructions[1].first=0u;instructions[1].count=0u;
    p.code=instructions;p.code_count=2u;p.ranges=r;p.range_count=0u;p.flags=3u;
    w.first=a;w.second=b;w.capacity=2u;w.steps=0u;a[2]=77u;b[2]=88u;
    if(maia_glob_match_utf8(&p,(const unsigned char*)"abc",3u,&w,&l)!=0||a[2]!=77u||b[2]!=88u)return 2;
    w.capacity=1u;if(maia_glob_match_utf8(&p,0,0u,&w,&l)!=8)return 3;w.capacity=2u;
    w.second=a;if(maia_glob_match_utf8(&p,0,0u,&w,&l)!=2)return 4;w.second=b;
    l.max_input_bytes=1u;if(maia_glob_match_utf8(&p,(const unsigned char*)"abc",3u,&w,&l)!=3)return 5;
    maia_glob_default_limits(&l);l.max_steps=0u;if(maia_glob_match_utf8(&p,0,0u,&w,&l)!=2)return 6;maia_glob_default_limits(&l);
    instructions[0].op=4;instructions[0].first=0u;instructions[0].count=1u;
    if(maia_glob_validate(&p)!=2)return 7;
    p.range_count=1u;r[0].first=98u;r[0].last=97u;if(maia_glob_validate(&p)!=2)return 8;
    r[0].first=97u;r[0].last=98u;if(maia_glob_validate(&p)!=0)return 9;
    instructions[0].first=4294967295u;if(maia_glob_validate(&p)!=2)return 10;
    memcpy(pattern,"a",1u);if(probe_compile(1u,0u)!=0)return 11;
    memcpy(&s,sp,sizeof(MaiaGlobStorage));memcpy(&original,pp,sizeof(MaiaGlobProgram));memcpy(&p,&original,sizeof(MaiaGlobProgram));s.code_capacity=1u;
    status=maia_glob_compile_utf8(pattern,1u,0,&s,&l,&p);
    if(status!=8)return 120+status;
    if(p.code_count!=original.code_count)return 130;
    if(p.code!=original.code)return 131;
    memcpy(&s,sp,sizeof(MaiaGlobStorage));s.compiler_workspace_bytes=1u;if(maia_glob_compile_utf8(pattern,1u,0,&s,&l,&p)!=8)return 13;
    memcpy(&s,sp,sizeof(MaiaGlobStorage));s.compiler_workspace=(unsigned char*)s.compiler_workspace+1u;
    if(maia_glob_compile_utf8(pattern,1u,0,&s,&l,&p)!=2)return 14;
    badmask[0]=1u;badmask[1]=0u;memcpy(&s,sp,sizeof(MaiaGlobStorage));
    if(maia_glob_compile_utf8((const unsigned char*)"\303\251",2u,badmask,&s,&l,&p)!=2)return 15;
    if(maia_glob_compile_utf8(pattern,257u,0,&s,&l,&p)!=3)return 16;
    return 0;
}
