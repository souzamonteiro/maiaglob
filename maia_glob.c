/* MaiaGlob: bounded whole-pattern NFA execution and callback traversal.
 * Copyright 2026 Roberto Luiz Souza Monteiro. SPDX-License-Identifier: Apache-2.0 */
#include "maia_glob_internal.h"
#include <string.h>

static int scalar(unsigned int c) { return c > 0u && c <= 1114111u && !(c >= 55296u && c <= 57343u); }
void maia_glob_default_limits(MaiaGlobLimits *l) {
    if (!l) return;
    l->max_pattern_bytes=256u; l->max_input_bytes=4096u; l->max_steps=1048576u;
    l->max_depth=16u; l->max_entries=16384u; l->max_directories=256u;
    l->max_matches=1024u; l->max_result_bytes=1048576u; l->max_walk_steps=1048576u;
}
int maia_glob_limits_valid(const MaiaGlobLimits *l) {
    return l && l->max_pattern_bytes && l->max_pattern_bytes<=256u && l->max_input_bytes
        && l->max_steps && l->max_depth && l->max_depth<=16u && l->max_entries
        && l->max_directories && l->max_matches && l->max_result_bytes && l->max_walk_steps;
}
int maia_glob_utf8_next(const unsigned char *s, unsigned int n, unsigned int *offset, unsigned int *value) {
    unsigned int at,c,count,min,i,x;
    if (!s || !offset || !value || *offset>=n) return 0;
    at=*offset; c=s[at++]; count=0u; min=0u;
    if (c<128u) { if (!c) return 0; }
    else if (c>=194u && c<=223u) { count=1u; c&=31u; min=128u; }
    else if (c>=224u && c<=239u) { count=2u; c&=15u; min=2048u; }
    else if (c>=240u && c<=244u) { count=3u; c&=7u; min=65536u; }
    else return 0;
    if (count>n-at) return 0;
    for (i=0u;i<count;++i) { x=s[at++]; if (x<128u || x>191u) return 0; c=(c<<6)|(x&63u); }
    if (c<min || !scalar(c)) return 0;
    *offset=at; *value=c; return 1;
}
int maia_glob_validate(const MaiaGlobProgram *p) {
    unsigned int i,j;
    const MaiaGlobInstruction *op;
    if (!p || !p->code || !p->code_count || p->code_count>257u || p->range_count>256u
        || (p->range_count && !p->ranges) || (p->flags&~3u)) return MAIA_GLOB_INVALID;
    for (j=0u;j<p->range_count;++j) if (!scalar(p->ranges[j].first) || !scalar(p->ranges[j].last)
        || p->ranges[j].first>p->ranges[j].last) return MAIA_GLOB_INVALID;
    for (i=0u;i<p->code_count;++i) {
        op=&p->code[i];
        if (op->op==MAIA_GLOB_END) { if (i+1u!=p->code_count || op->first || op->count) return MAIA_GLOB_INVALID; }
        else {
            if (i+1u==p->code_count) return MAIA_GLOB_INVALID;
            if (op->op==MAIA_GLOB_CHAR) { if (!scalar(op->first) || op->count) return MAIA_GLOB_INVALID; }
            else if (op->op==MAIA_GLOB_ANY || op->op==MAIA_GLOB_STAR) { if (op->first || op->count) return MAIA_GLOB_INVALID; }
            else if (op->op==MAIA_GLOB_CLASS || op->op==MAIA_GLOB_NOT_CLASS) {
                if (!op->count || op->first>p->range_count || op->count>p->range_count-op->first) return MAIA_GLOB_INVALID;
            } else return MAIA_GLOB_INVALID;
        }
    }
    return MAIA_GLOB_MATCH;
}
static int tick(MaiaGlobWorkspace *w, const MaiaGlobLimits *l) {
    if (w->steps>=l->max_steps) return 0;
    ++w->steps; return 1;
}
int maia_glob_match_utf8(const MaiaGlobProgram *p, const unsigned char *s, unsigned int n,
    MaiaGlobWorkspace *w, const MaiaGlobLimits *l) {
    unsigned int i,j,at,c,previous,leading,hit;
    unsigned char *a,*b,*swap;
    const MaiaGlobInstruction *op;
    if (maia_glob_validate(p)!=MAIA_GLOB_MATCH || !maia_glob_limits_valid(l) || !w || (!s && n)
        || !w->first || !w->second || w->first==w->second) return MAIA_GLOB_INVALID;
    if (n>l->max_input_bytes) return MAIA_GLOB_LIMIT;
    if (w->capacity<p->code_count) return MAIA_GLOB_SPACE;
    a=w->first; b=w->second; w->steps=0u;
    memset(a,0,p->code_count); memset(b,0,p->code_count); a[0]=1u;
    at=0u; previous=47u;
    while (at<n) {
        if (!maia_glob_utf8_next(s,n,&at,&c)) return MAIA_GLOB_INVALID;
        leading=(p->flags&MAIA_GLOB_PERIOD) && c==46u && (previous==47u || at==1u);
        for (i=0u;i<p->code_count;++i) {
            if (!tick(w,l)) return MAIA_GLOB_LIMIT;
            if (a[i] && p->code[i].op==MAIA_GLOB_STAR && !leading) a[i+1u]=1u;
        }
        memset(b,0,p->code_count);
        for (i=0u;i+1u<p->code_count;++i) {
            if (!tick(w,l)) return MAIA_GLOB_LIMIT;
            if (!a[i]) continue;
            op=&p->code[i];
            if (leading && (op->op!=MAIA_GLOB_CHAR || op->first!=46u)) continue;
            if ((p->flags&MAIA_GLOB_PATHNAME) && c==47u && (op->op!=MAIA_GLOB_CHAR || op->first!=47u)) continue;
            if (op->op==MAIA_GLOB_CHAR) { if (op->first==c) b[i+1u]=1u; }
            else if (op->op==MAIA_GLOB_STAR) b[i]=1u;
            else if (op->op==MAIA_GLOB_ANY) b[i+1u]=1u;
            else {
                hit=0u;
                for (j=0u;j<op->count;++j) {
                    if (!tick(w,l)) return MAIA_GLOB_LIMIT;
                    if (c>=p->ranges[op->first+j].first && c<=p->ranges[op->first+j].last) hit=1u;
                }
                if ((hit && op->op==MAIA_GLOB_CLASS) || (!hit && op->op==MAIA_GLOB_NOT_CLASS)) b[i+1u]=1u;
            }
        }
        swap=a; a=b; b=swap; previous=(p->flags&MAIA_GLOB_PATHNAME)?c:0u;
    }
    for (i=0u;i+1u<p->code_count;++i) {
        if (!tick(w,l)) return MAIA_GLOB_LIMIT;
        if (a[i] && p->code[i].op==MAIA_GLOB_STAR) a[i+1u]=1u;
    }
    return a[p->code_count-1u]?MAIA_GLOB_MATCH:MAIA_GLOB_NOMATCH;
}

static void frame_set(MaiaGlobFrame *f, const MaiaGlobProgram *p, unsigned int begin) {
    unsigned int end;
    while (begin+1u<p->code_count && p->code[begin].op==MAIA_GLOB_CHAR && p->code[begin].first==47u) ++begin;
    end=begin;
    while (end+1u<p->code_count && !(p->code[end].op==MAIA_GLOB_CHAR && p->code[end].first==47u)) ++end;
    f->begin=begin; f->end=end; f->next=end; f->handle=-1; f->synthetic=0u;
}
void maia_glob_walk_cancel(MaiaGlobWalker *w) {
    unsigned int i;
    if (!w) return;
    if (w->active && w->filesystem.close) {
        for (i=0u;i<w->depth;++i) if (w->frames[i].handle>0) {
            w->filesystem.close(w->filesystem.context,w->frames[i].handle); w->frames[i].handle=-1;
        }
    }
    w->active=0u; w->depth=0u; w->pending=0u;
}
static int fail_walk(MaiaGlobWalker *w, int status, int provider) {
    maia_glob_walk_cancel(w); w->status=status; w->provider_status=provider; return status;
}
int maia_glob_walk_begin(MaiaGlobWalker *w, const MaiaGlobProgram *p, const MaiaGlobFs *fs, const MaiaGlobLimits *l) {
    unsigned int first;
    if (!w || maia_glob_validate(p)!=MAIA_GLOB_MATCH || !maia_glob_limits_valid(l)
        || !(p->flags&MAIA_GLOB_PATHNAME) || !fs || !fs->open || !fs->read || !fs->close) return MAIA_GLOB_INVALID;
    /* Caller must cancel a live walker before reinitializing it. */
    memset(w,0,sizeof(*w)); w->program=p; w->filesystem=*fs; w->limits=*l;
    w->active=1u; w->depth=1u; w->status=MAIA_GLOB_YIELD;
    if (p->code_count==1u) { w->active=0u; w->depth=0u; w->status=MAIA_GLOB_DONE; return MAIA_GLOB_MATCH; }
    first=0u;
    while (first+1u<p->code_count && p->code[first].op==MAIA_GLOB_CHAR && p->code[first].first==47u) ++first;
    if (first) { w->frames[0].path[0]=47u; w->frames[0].length=1u; }
    frame_set(w->frames,p,first);
    return MAIA_GLOB_MATCH;
}
static int related(int s) { return s==-2 || s==-3 || s==-9; }
static int emit_pending(MaiaGlobWalker *w, unsigned char *path, unsigned int capacity, unsigned int *length, unsigned int *type) {
    if (capacity<=w->pending_length) return MAIA_GLOB_SPACE;
    memcpy(path,w->pending_path,w->pending_length+1u);
    *length=w->pending_length; *type=w->pending_type; w->pending=0u;
    return MAIA_GLOB_ENTRY;
}
int maia_glob_walk_step(MaiaGlobWalker *w, unsigned char *path, unsigned int capacity,
    unsigned int *length, unsigned int *type, unsigned int budget) {
    MaiaGlobFrame *f,*child;
    MaiaGlobInstruction code[257]; MaiaGlobProgram part; MaiaGlobWorkspace scratch;
    unsigned int count,entry_type,at,c,i,bytes,separator,dot,terminal,trailing;
    int result,status;
    static const unsigned char current[2]={46u,0u};
    if (!w || !path || !length || !type || !budget) return MAIA_GLOB_INVALID;
    if (!w->active) return w->status;
    if (w->pending) return emit_pending(w,path,capacity,length,type);
    while (budget--) {
        if (w->work>=w->limits.max_walk_steps) return fail_walk(w,MAIA_GLOB_LIMIT,0);
        ++w->work; f=&w->frames[w->depth-1u];
        if (f->handle<0) {
            if (w->directories>=w->limits.max_directories) return fail_walk(w,MAIA_GLOB_LIMIT,0);
            result=w->filesystem.open(w->filesystem.context,f->length?f->path:current,f->length?f->length:1u);
            if (result==MAIA_GLOB_FS_WOULD_BLOCK) return MAIA_GLOB_YIELD;
            ++w->directories;
            if (result<=0) {
                if (related(result)) { --w->depth; if (!w->depth) return fail_walk(w,MAIA_GLOB_DONE,0); continue; }
                return fail_walk(w,result==-4?MAIA_GLOB_LIMIT:MAIA_GLOB_IO,result);
            }
            f->handle=result;
        }
        dot=0u; count=f->end-f->begin;
        if (!count) dot=3u;
        else if (count<=2u && w->program->code[f->begin].op==MAIA_GLOB_CHAR && w->program->code[f->begin].first==46u
            && (count==1u || (w->program->code[f->begin+1u].op==MAIA_GLOB_CHAR && w->program->code[f->begin+1u].first==46u))) dot=count;
        if (dot && !f->synthetic) {
            f->synthetic=1u; count=dot==3u?0u:dot; w->name[0]=46u; w->name[1]=46u; entry_type=2u;
        } else if (dot) result=0;
        else {
            if (w->scanned>=w->limits.max_entries) return fail_walk(w,MAIA_GLOB_LIMIT,0);
            result=w->filesystem.read(w->filesystem.context,f->handle,w->name,256u,&entry_type);
            if (result==MAIA_GLOB_FS_WOULD_BLOCK) return MAIA_GLOB_YIELD;
            if (result>0) { count=(unsigned int)result; ++w->scanned; }
        }
        if (!(dot && f->synthetic==1u)) {
            if (result<=0) {
                if (result<0 && !related(result)) return fail_walk(w,result==-4?MAIA_GLOB_LIMIT:MAIA_GLOB_IO,result);
                status=w->filesystem.close(w->filesystem.context,f->handle); f->handle=-1;
                if (status<0) return fail_walk(w,MAIA_GLOB_IO,status);
                --w->depth; if (!w->depth) return fail_walk(w,MAIA_GLOB_DONE,0);
                continue;
            }
        }
        if (dot) ++f->synthetic;
        if (count>255u || (entry_type!=1u && entry_type!=2u)) return fail_walk(w,MAIA_GLOB_IO,-1);
        at=0u;
        while (at<count) if (!maia_glob_utf8_next(w->name,count,&at,&c) || c==47u) return fail_walk(w,MAIA_GLOB_IO,-1);
        if (!dot && ((count==1u && w->name[0]==46u) || (count==2u && w->name[0]==46u && w->name[1]==46u))) continue;
        if (!dot) {
            for (i=0u;i<f->end-f->begin;++i) code[i]=w->program->code[f->begin+i];
            code[i].op=MAIA_GLOB_END; code[i].first=0u; code[i].count=0u;
            part=*w->program; part.code=code; part.code_count=i+1u;
            scratch.first=w->states_a; scratch.second=w->states_b; scratch.capacity=257u; scratch.steps=0u;
            result=maia_glob_match_utf8(&part,w->name,count,&scratch,&w->limits);
            if (result==MAIA_GLOB_NOMATCH) continue;
            if (result!=MAIA_GLOB_MATCH) return fail_walk(w,result,0);
        }
        terminal=f->end+1u==w->program->code_count; trailing=0u;
        i=f->end;
        while (i+1u<w->program->code_count && w->program->code[i].op==MAIA_GLOB_CHAR && w->program->code[i].first==47u) ++i;
        if (i+1u==w->program->code_count && !terminal) { if (entry_type!=2u) continue; terminal=1u; trailing=1u; }
        separator=f->length && f->path[f->length-1u]!=47u && count;
        if (count>1023u-f->length || separator>1023u-f->length-count) return fail_walk(w,MAIA_GLOB_LIMIT,0);
        bytes=f->length+separator+count;
        memcpy(w->pending_path,f->path,f->length);
        if (separator) w->pending_path[f->length]=47u;
        memcpy(w->pending_path+f->length+separator,w->name,count); w->pending_path[bytes]=0u;
        if (terminal) {
            if (trailing && bytes && w->pending_path[bytes-1u]!=47u) {
                if (bytes>=1023u) return fail_walk(w,MAIA_GLOB_LIMIT,0);
                w->pending_path[bytes++]=47u; w->pending_path[bytes]=0u;
            }
            /* Synthetic . or .. is validated by opening its resulting directory. */
            if (dot && dot!=3u) {
                if (w->directories>=w->limits.max_directories) return fail_walk(w,MAIA_GLOB_LIMIT,0);
                result=w->filesystem.open(w->filesystem.context,w->pending_path,bytes);
                if (result==MAIA_GLOB_FS_WOULD_BLOCK) { f->synthetic=0u; return MAIA_GLOB_YIELD; }
                ++w->directories;
                if (result<=0) { if (related(result)) continue; return fail_walk(w,result==-4?MAIA_GLOB_LIMIT:MAIA_GLOB_IO,result); }
                status=w->filesystem.close(w->filesystem.context,result);
                if (status<0) return fail_walk(w,MAIA_GLOB_IO,status);
            }
            if (w->matches>=w->limits.max_matches || bytes+1u>w->limits.max_result_bytes-w->result_bytes) return fail_walk(w,MAIA_GLOB_LIMIT,0);
            ++w->matches; w->result_bytes+=bytes+1u; w->pending_length=bytes; w->pending_type=entry_type; w->pending=1u;
            return emit_pending(w,path,capacity,length,type);
        }
        if (entry_type!=2u) continue;
        if (w->depth>=w->limits.max_depth) return fail_walk(w,MAIA_GLOB_LIMIT,0);
        child=&w->frames[w->depth]; memset(child,0,sizeof(*child));
        memcpy(child->path,w->pending_path,bytes+1u); child->length=bytes;
        frame_set(child,w->program,i); ++w->depth;
    }
    return MAIA_GLOB_YIELD;
}
