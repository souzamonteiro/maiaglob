import assert from 'node:assert/strict';
import fs from 'node:fs';
import {execFileSync} from 'node:child_process';
const cases=[];
const add=(pattern,name,expected,compile=0,mask=null,flags=3,limit=1048576)=>cases.push({pattern:Buffer.from(pattern),name:Buffer.from(name),expected,compile,mask,flags,limit});
// Independent iterative prefix DP for the literal, question and star subset.
function reference(p,n){p=[...p];n=[...n];const rows=Array.from({length:p.length+1},()=>Array(n.length+1).fill(false));rows[0][0]=true;
 for(let i=1;i<=p.length;i++)for(let j=0;j<=n.length;j++){
  const wildcard=p[i-1]==='*'||p[i-1]==='?';
  const blocked=j>0&&wildcard&&(n[j-1]==='/'||(n[j-1]==='.'&&(j===1||n[j-2]==='/')));
  if(p[i-1]==='*')rows[i][j]=(!blocked&&rows[i-1][j])||(j>0&&!blocked&&rows[i][j-1]);
  else rows[i][j]=j>0&&!blocked&&(p[i-1]==='?'||p[i-1]===n[j-1])&&rows[i-1][j-1];
 }return rows[p.length][n.length]?0:1;}
function words(alphabet,max){let all=[''],level=[''];for(let i=0;i<max;i++){level=level.flatMap(s=>alphabet.map(c=>s+c));all.push(...level);}return all;}
for(const p of words(['a','b','*','?'],4))for(const n of words(['a','b'],4))add(p,n,reference(p,n));
for(const [p,n,e] of [['*','.hidden',1],['.*','.hidden',0],['*.c','x.c',0],['*','a/b',1],['a/*','a/.x',1],['a/.*','a/.x',0],['*.x','.x',1],['?','é',0],['?','🌍',0],['??','é',1],['[a-c]','b',0],['[!a-c]','d',0],['[!a-c]','b',1],['[a-]','-',0],['[-a]','-',0],['[]a]',']',0],['[é-ê]','ê',0],['[^a]','^',0],['[.]','.x',1],['[.]','.',1],['\\*','*',0],['a\\ b','a b',0],['','',0],['','a',1]])add(p,n,e);
for(const p of ['[','[]','[!]','[a','[a/b]','abc\\','[z-a]','[[:alpha:]]','[[.a.]]','[[=a=]]'])add(p,'',-1,2);
for(const p of ['*','?','[a]','\\','é','a b'])add(p,p,0,0,Buffer.alloc(Buffer.byteLength(p),1));
add('é','',-1,2,Buffer.from([1,0]));
for(const bytes of [[0],[0xc0,0xaf],[0xed,0xa0,0x80],[0xf4,0x90,0x80,0x80],[0xe2,0x82],[0x80]]){add(Buffer.from(bytes),'',-1,2);add('*',Buffer.from(bytes),2);}
add('*a'.repeat(24)+'b','a'.repeat(32),1);
add('*a'.repeat(24)+'b','a'.repeat(32),3,0,null,3,100);
add('*','a/b',0,0,null,0);add('*','.x',0,0,null,0);
add('a'.repeat(256),'a'.repeat(256),0);
add('*'.repeat(256),'a'.repeat(64),0);
add('🌍'.repeat(64),'🌍'.repeat(64),0);
add('*'.repeat(256),'*'.repeat(256),0,0,Buffer.alloc(256,1));
add('*','a'.repeat(4096),0);
add('\\*','*',0,0,Buffer.from([0,1]));
add('\\*','\\abc',0,0,Buffer.from([1,0]));
add('[[:alpha:]]','[]',0,0,Buffer.from([0,1,0,0,0,0,0,0,0,0,0]));
add('[\\[:alpha:]]','[]',0);
add('[[:alpha:]]','[]',0,0,Buffer.from([0,0,1,0,0,0,0,0,0,0,0]));
const hex=b=>b.length?b.toString('hex'):'-';
const input=cases.map(c=>`${hex(c.pattern)} ${hex(c.name)} ${c.mask?hex(c.mask):'-'} ${c.flags} ${c.limit}\n`).join('');
const output=execFileSync('build/native-driver',[],{input,encoding:'utf8',maxBuffer:32*1024*1024}).trim().split('\n');assert.equal(output.length,cases.length);
for(let i=0;i<cases.length;i++){const c=cases[i],v=output[i].split(' ').slice(0,2).map(Number);assert.deepEqual(v,[c.compile,c.expected],JSON.stringify({i,p:c.pattern.toString(),n:c.name.toString()}));}
fs.writeFileSync('build/match-corpus.json',JSON.stringify({cases:cases.map(c=>({...c,pattern:hex(c.pattern),name:hex(c.name),mask:c.mask?hex(c.mask):null})),output}));
console.log(`${cases.length} native cases passed: exhaustive reference DP, Unicode, quotes, malformed encodings, classes and finite adversarial work.`);
