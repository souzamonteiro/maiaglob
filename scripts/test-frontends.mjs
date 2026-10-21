import assert from 'node:assert/strict';
import fs from 'node:fs';
import {createRequire} from 'node:module';
const require=createRequire(import.meta.url),Parser=require('../generated/glob-parser.cjs');
const {ParseTreeCollector}=require('../maiacpp/maiacc/parser-generator/parse-tree-collector.js');
const {cases,output}=JSON.parse(fs.readFileSync('build/match-corpus.json','utf8'));
const terminals=n=>n.kind==='terminal'?[n]:(n.children||[]).flatMap(terminals);
const value=n=>{const t=terminals(n)[0];return (t.token==='ESCAPED'?t.value.slice(1):t.value).codePointAt(0);};
let checked=0;
for(let i=0;i<cases.length;i++){
 const c=cases[i];if(c.compile!==0)continue;
 const p=c.pattern==='-'?Buffer.alloc(0):Buffer.from(c.pattern,'hex'),mask=c.mask?Buffer.from(c.mask,'hex'):null;
 let source='',offset=0;const protectedScalars=new Map();for(const scalar of p.toString('utf8')){
  if(mask?.[offset]){protectedScalars.set(source.length,scalar);source+='X';}else source+=scalar;offset+=Buffer.byteLength(scalar);
 }
 const tree=new ParseTreeCollector(),parser=new Parser(source,tree),terminal=tree.terminal.bind(tree);
 tree.terminal=(type,text,position)=>{const escaped=type==='ESCAPED',at=parser.tokens[position].start+(escaped?1:0);const original=protectedScalars.get(at);terminal(type,original?(escaped?'\\':'')+original:text);};
 tree.parse(parser);
 const code=[],ranges=[];
 const classEntries=n=>{
  if(n.name==='Range'){const scalars=n.children.filter(x=>x.name==='ClassScalar');ranges.push(scalars.map(value));}
  else if(n.name==='ClassScalar')ranges.push([value(n),value(n)]);
  else if(n.name==='InitialClose')ranges.push([93,93]);
  else for(const child of n.children||[])classEntries(child);
 };
 for(const atom of tree.root.children.filter(n=>n.name==='Atom')){
  const node=atom.children[0];
  if(node.name==='Class'){const start=ranges.length;classEntries(node);code.push([node.children.some(x=>x.name==='Negation')?5:4,start,ranges.length-start]);}
  else if(node.token==='STAR')code.push([3,0,0]);
  else if(node.token==='QUESTION')code.push([2,0,0]);
  else code.push([1,value(node),0]);
 }
 code.push([6,0,0]);const actual=output[i].split(' ');
 assert.deepEqual(actual.slice(3),[String(code.length),String(ranges.length),...code.map(x=>x.join(',')),...ranges.map(x=>x.join(','))],`JS/C++98 semantic program ${i}`);checked++;
}
console.log(`${checked} generated JavaScript/C++98 frontend cases have identical semantic programs.`);
