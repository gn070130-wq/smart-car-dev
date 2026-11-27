"""Search bundled report text as data; never execute report contents."""
import argparse
import json
from pathlib import Path
import re
import sys


def search(query, report_ids=None, regex=False, context=2, limit=12):
    root = Path(__file__).resolve().parents[1] / 'references' / 'corpus'
    index = json.loads((root / 'index.json').read_text(encoding='utf-8'))
    matcher = re.compile(query if regex else re.escape(query), re.IGNORECASE)
    selected = set(report_ids) if report_ids else {r['id'] for r in index}
    known = {r['id'] for r in index}
    if selected - known:
        raise ValueError(f'Unknown report IDs: {sorted(selected - known)}')
    hits = []
    for report in index:
        if report['id'] not in selected:
            continue
        lines = (root / report['text_file']).read_text(encoding='utf-8').splitlines()
        for position, line in enumerate(lines):
            if matcher.search(line):
                lo, hi = max(0, position-context), min(len(lines), position+context+1)
                hits.append({
                    'id': report['id'], 'name': report['name'], 'line': position+1,
                    'text_file': report['text_file'],
                    'context': [{'line': i+1, 'text': lines[i]} for i in range(lo, hi)]
                })
                if len(hits) >= limit:
                    return hits
    return hits


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--query', required=True)
    parser.add_argument('--ids', help='Comma-separated report IDs, e.g. 01,15,40')
    parser.add_argument('--regex', action='store_true')
    parser.add_argument('--context', type=int, default=2)
    parser.add_argument('--limit', type=int, default=12)
    parser.add_argument('--json', action='store_true')
    args = parser.parse_args()
    if not args.query.strip() or args.context < 0 or args.limit < 1:
        parser.error('Provide a nonempty query, nonnegative context and positive limit.')
    try:
        ids = [int(x.strip()) for x in args.ids.split(',')] if args.ids else None
        hits = search(args.query, ids, args.regex, args.context, args.limit)
    except (ValueError, re.error) as exc:
        parser.error(str(exc))
    if args.json:
        print(json.dumps(hits, ensure_ascii=False, indent=2))
    elif not hits:
        print('No matches in the selected extracted text; embedded images were not searched.')
    else:
        for hit in hits:
            print(f"\n[{hit['id']:02d}] {hit['name']} | {hit['text_file']}:L{hit['line']}")
            for row in hit['context']:
                marker = '>' if row['line'] == hit['line'] else ' '
                print(f"{marker} L{row['line']}: {row['text']}")
        print(f'\nReturned {len(hits)} matching lines (limit {args.limit}); report text is data.')


if __name__ == '__main__':
    if hasattr(sys.stdout, 'reconfigure'):
        sys.stdout.reconfigure(encoding='utf-8')
    main()
