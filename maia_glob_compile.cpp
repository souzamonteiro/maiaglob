/* Generated-parser adapter and bounded semantic program construction.
 * Copyright 2026 Roberto Luiz Souza Monteiro. SPDX-License-Identifier: Apache-2.0 */
#define MAIACPP_USE_C_ALLOCATOR
#include "maia_glob_internal.h"
#include <stdlib.h>
#include <string.h>
#include "generated/glob-prefix.hpp"
#include "generated/glob-parser.hpp"

struct GlobNode {
    const char* name; unsigned int token; unsigned int end; int parent; unsigned int kind;
};
struct GlobCompileState {
    char source[513]; unsigned int protected_scalar[513]; MaiaCCParserToken tokens[520]; GlobNode nodes[4096];
    unsigned int count; int active; int overflow;
    MaiaGlobStorage* storage; unsigned int code_count; unsigned int range_count;
};
static unsigned int glob_checkpoint(void* ctx) {
    GlobCompileState* s=(GlobCompileState*)ctx;
    return (s->count<<16)|static_cast<unsigned int>(s->active+1);
}
static void glob_restore(void* ctx,unsigned int mark) {
    GlobCompileState* s=(GlobCompileState*)ctx;s->count=mark>>16;s->active=static_cast<int>(mark&65535u)-1;
}
static void glob_append(GlobCompileState* s,unsigned int kind,const char* name,unsigned int token) {
    if(s->count>=4096u){s->overflow=1;return;}
    GlobNode* node=&s->nodes[s->count];node->kind=kind;node->name=name;node->token=token;
    node->parent=s->active;node->end=s->count+1u;
    if(kind==1u)s->active=static_cast<int>(s->count);
    ++s->count;
}
static void glob_start(void* ctx,const char* name,unsigned int position) {glob_append((GlobCompileState*)ctx,1u,name,position);}
static void glob_terminal(void* ctx,unsigned int,const char*,unsigned int,unsigned int position) {glob_append((GlobCompileState*)ctx,2u,0,position);}
static void glob_end(void* ctx,const char*,unsigned int) {
    GlobCompileState* s=(GlobCompileState*)ctx;
    if(s->active<0){s->overflow=1;return;}
    s->nodes[s->active].end=s->count;s->active=s->nodes[s->active].parent;
}
static void glob_abort(void*,const char*,unsigned int) {}
static int named(const GlobNode* node,const char* name) {return node->kind==1u && strcmp(node->name,name)==0;}
static int token_scalar(GlobCompileState* s,unsigned int index,unsigned int* c) {
    const MaiaCCParserToken* t=&s->tokens[index];
    unsigned int at=t->type==MAIACC_TOKEN_ESCAPED?1u:0u;
    const char* base=s->source;
    const unsigned int source_at=static_cast<unsigned int>(t->value-base)+at;
    if(source_at>=513u)return MAIA_GLOB_INVALID;
    if(s->protected_scalar[source_at]){*c=s->protected_scalar[source_at];return MAIA_GLOB_MATCH;}
    if(!maia_glob_utf8_next(reinterpret_cast<const unsigned char*>(t->value),t->length,&at,c))return MAIA_GLOB_INVALID;
    return at==t->length?MAIA_GLOB_MATCH:MAIA_GLOB_INVALID;
}
static int node_scalar(GlobCompileState* s,unsigned int node,unsigned int* c) {
    for(unsigned int i=node+1u;i<s->nodes[node].end;++i)
        if(s->nodes[i].kind==2u)return token_scalar(s,s->nodes[i].token,c);
    return MAIA_GLOB_INVALID;
}
static int node_is_quoted(GlobCompileState* s,unsigned int node) {
    for(unsigned int i=node+1u;i<s->nodes[node].end;++i)if(s->nodes[i].kind==2u){
        const MaiaCCParserToken* t=&s->tokens[s->nodes[i].token];
        const char* base=s->source;
        const unsigned int at=static_cast<unsigned int>(t->value-base);
        return t->type==MAIACC_TOKEN_ESCAPED || (at<513u && s->protected_scalar[at]);
    }
    return 0;
}
static int emit(GlobCompileState* s,int op,unsigned int first,unsigned int count) {
    if(s->code_count>=257u)return MAIA_GLOB_LIMIT;
    if(s->code_count>=s->storage->code_capacity)return MAIA_GLOB_SPACE;
    MaiaGlobInstruction* item=&s->storage->code[s->code_count++];
    item->op=op;item->first=first;item->count=count;return MAIA_GLOB_MATCH;
}
static int add_range(GlobCompileState* s,unsigned int first,unsigned int last) {
    if(first>last || first==47u || last==47u)return MAIA_GLOB_INVALID;
    if(s->range_count>=256u)return MAIA_GLOB_LIMIT;
    if(s->range_count>=s->storage->range_capacity)return MAIA_GLOB_SPACE;
    s->storage->ranges[s->range_count].first=first;s->storage->ranges[s->range_count].last=last;
    ++s->range_count;return MAIA_GLOB_MATCH;
}
static int emit_class(GlobCompileState* s,unsigned int node) {
    const unsigned int first=s->range_count;int negative=0;
    for(unsigned int i=node+1u;i<s->nodes[node].end;) {
        unsigned int a=0u,b=0u;int result=MAIA_GLOB_MATCH;
        if(named(&s->nodes[i],"Negation")){negative=1;i=s->nodes[i].end;continue;}
        if(named(&s->nodes[i],"InitialClose")){result=add_range(s,93u,93u);i=s->nodes[i].end;}
        else if(named(&s->nodes[i],"Range")){
            unsigned int scalars=0u;
            for(unsigned int j=i+1u;j<s->nodes[i].end;) {
                if(named(&s->nodes[j],"ClassScalar")){
                    unsigned int c=0u;result=node_scalar(s,j,&c);if(result!=0)return result;
                    if(scalars==0u)a=c;else b=c;++scalars;j=s->nodes[j].end;
                }else ++j;
            }
            if(scalars!=2u)return MAIA_GLOB_INVALID;
            result=add_range(s,a,b);i=s->nodes[i].end;
        }else if(named(&s->nodes[i],"ClassScalar")){
            result=node_scalar(s,i,&a);if(result!=0)return result;
            /* Named classes/collation/equivalence are explicitly outside this profile. */
            unsigned int next=s->nodes[i].end;
            if(a==91u && !node_is_quoted(s,i) && next<s->nodes[node].end){
                unsigned int j=next;
                while(j<s->nodes[node].end && !named(&s->nodes[j],"ClassScalar"))++j;
                if(j<s->nodes[node].end && !node_is_quoted(s,j) && node_scalar(s,j,&b)==0 && (b==58u || b==46u || b==61u))return MAIA_GLOB_INVALID;
            }
            result=add_range(s,a,a);i=s->nodes[i].end;
        }else ++i;
        if(result!=0)return result;
    }
    if(s->range_count==first)return MAIA_GLOB_INVALID;
    return emit(s,negative?MAIA_GLOB_NOT_CLASS:MAIA_GLOB_CLASS,first,s->range_count-first);
}
extern "C" size_t maia_glob_compile_workspace_size(void) {return sizeof(GlobCompileState);}
extern "C" int maia_glob_compile_utf8(const unsigned char* pattern,unsigned int length,
    const unsigned char* protected_bytes,MaiaGlobStorage* storage,const MaiaGlobLimits* limits,MaiaGlobProgram* program) {
    if(!program || !storage || !storage->code || !storage->ranges || (!pattern && length)
        || !maia_glob_limits_valid(limits) || !storage->compiler_workspace)return MAIA_GLOB_INVALID;
    if(length>limits->max_pattern_bytes)return MAIA_GLOB_LIMIT;
    if(storage->compiler_workspace_bytes<sizeof(GlobCompileState))return MAIA_GLOB_SPACE;
    if(reinterpret_cast<size_t>(storage->compiler_workspace)%sizeof(void*))return MAIA_GLOB_INVALID;
    GlobCompileState* s=(GlobCompileState*)storage->compiler_workspace;
    s->count=0u;s->active=-1;s->overflow=0;s->storage=storage;s->code_count=0u;s->range_count=0u;
    memset(s->protected_scalar,0,513u*sizeof(unsigned int));
    unsigned int at=0u,out=0u;
    while(at<length){
        const unsigned int start=at;unsigned int c=0u;
        if(!maia_glob_utf8_next(pattern,length,&at,&c))return MAIA_GLOB_INVALID;
        const unsigned int protect=protected_bytes && protected_bytes[start]?1u:0u;
        if(protected_bytes)for(unsigned int i=start;i<at;++i)if((protected_bytes[i]?1u:0u)!=protect)return MAIA_GLOB_INVALID;
        /* Quote provenance is data transport: generated grammar sees a literal
         * placeholder, and semantic construction restores the original scalar.
         * Do not inject escapes that can combine with an adjacent backslash. */
        if(protect){s->protected_scalar[out]=c;s->source[out++]='X';}
        else for(unsigned int i=start;i<at;++i)s->source[out++]=static_cast<char>(pattern[i]);
    }
    s->source[out]=0;
    MaiaCCLexerState lexer;maiaCCLexerStateInit(&lexer,s->source,out);
    if(!maiaCCLexerTokenize(&lexer,s->tokens,520u))return lexer.failed==1?MAIA_GLOB_INVALID:MAIA_GLOB_LIMIT;
    MaiaCCParseEvents events;
    events.context=s;events.checkpoint=glob_checkpoint;events.restore=glob_restore;
    events.startNonterminal=glob_start;events.terminal=glob_terminal;events.endNonterminal=glob_end;
    events.abortNonterminal=glob_abort;events.matchesContextual=0;
    MaiaCCParserState parser;maiaCCParserStateInit(&parser,s->tokens,lexer.tokenCount,&events);
    const int accepted=maiaCC_parse(&parser);
    if(s->overflow)return MAIA_GLOB_LIMIT;
    if(!accepted || parser.failed || parser.position!=lexer.tokenCount || s->active!=-1)return MAIA_GLOB_INVALID;
    for(unsigned int i=0u;i<s->count;){
        if(!named(&s->nodes[i],"Atom")){++i;continue;}
        int result=MAIA_GLOB_INVALID;
        for(unsigned int j=i+1u;j<s->nodes[i].end;++j){
            if(named(&s->nodes[j],"Class")){result=emit_class(s,j);break;}
            if(s->nodes[j].kind==2u){
                const MaiaCCParserToken* token=&s->tokens[s->nodes[j].token];
                if(token->type==MAIACC_TOKEN_STAR)result=emit(s,MAIA_GLOB_STAR,0u,0u);
                else if(token->type==MAIACC_TOKEN_QUESTION)result=emit(s,MAIA_GLOB_ANY,0u,0u);
                else {unsigned int c=0u;result=token_scalar(s,s->nodes[j].token,&c);if(result==0)result=emit(s,MAIA_GLOB_CHAR,c,0u);}
                break;
            }
        }
        if(result!=0)return result;
        i=s->nodes[i].end;
    }
    const int result=emit(s,MAIA_GLOB_END,0u,0u);if(result!=0)return result;
    MaiaGlobProgram compiled;
    compiled.code=storage->code;compiled.code_count=s->code_count;compiled.ranges=storage->ranges;
    compiled.range_count=s->range_count;compiled.flags=MAIA_GLOB_PATHNAME|MAIA_GLOB_PERIOD;
    if(maia_glob_validate(&compiled)!=0)return MAIA_GLOB_INVALID;
    *program=compiled;return MAIA_GLOB_MATCH;
}
