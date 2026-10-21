import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {execFileSync} from 'node:child_process';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const generator=path.join(root,'maiacpp/maiacc/bin/tREx.sh');
if(!fs.existsSync(generator))throw new Error('Initialize the pinned in-repository toolchain submodules first');
fs.mkdirSync(path.join(root,'generated'),{recursive:true});
for(const [language,file] of [['cpp98','glob-parser.hpp'],['javascript','glob-parser.cjs']])
 execFileSync('bash',[generator,'--ebnf','--language',language,'--to-xml',path.join(root,'generated/glob.xml'),path.join(root,'grammar/glob.ebnf'),path.join(root,'generated',file)],{stdio:'inherit',cwd:root});
// Identifier-only include adapter, not an edit to generated lexer/parser sources.
// It prevents static/type collisions when C89 compiler outputs share a link unit.
const header=fs.readFileSync(path.join(root,'generated/glob-parser.hpp'),'utf8');
const identifiers=[...new Set(header.match(/\b(?:maiaCC|MaiaCC)[A-Za-z0-9_]*\b/g))].sort();
fs.writeFileSync(path.join(root,'generated/glob-prefix.hpp'),identifiers.map(name=>`#define ${name} MaiaGlobGenerated_${name}\n`).join(''));
fs.copyFileSync(path.join(root,'maiacpp/maiaregex/maia_regex.h'),path.join(root,'generated/maia_regex.h'));
