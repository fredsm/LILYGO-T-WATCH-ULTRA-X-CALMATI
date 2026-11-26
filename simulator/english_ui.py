"""Generate English-only host copies, without editing firmware source files.

The catalogue is scoped by filename and exact C++ string literal contents.
Comments, identifiers, character literals and unmapped protocol/data strings
are preserved byte-for-byte. Escapes and printf arguments stay intact.
"""
import argparse
import json
from pathlib import Path
import re

TOKEN = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|R"(?P<delim>[^ ()\\\t\r\n]{0,16})\([\s\S]*?\)(?P=delim)"|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
FORMAT = re.compile(r'%(?:%|[-+ #0]*\d*(?:\.\d+|\.\*)?(?:hh|ll|[hljztL])?[diuoxXfFeEgGaAcsp])')

def translate(source, translations):
    def replace(match):
        token = match[0]
        if not token.startswith('"'):
            return token
        key = token[1:-1]
        if key not in translations:
            return token
        value = translations[key]
        if FORMAT.findall(key) != FORMAT.findall(value):
            raise ValueError(f'Format arguments changed: {key!r}')
        if '\n' in value or '\r' in value or re.search(r'(?<!\\)"', value):
            raise ValueError(f'Unescaped C++ translation: {value!r}')
        return '"' + value + '"'
    return TOKEN.sub(replace, source)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('source', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('catalogue', type=Path)
    args = parser.parse_args()
    catalogue = json.loads(args.catalogue.read_text(encoding='utf-8'))
    source = args.source.read_text(encoding='utf-8')
    translated = translate(source, catalogue.get(args.source.name, {}))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(translated, encoding='utf-8', newline='\n')
