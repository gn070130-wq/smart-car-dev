"""Read actual bundled source or copy an artifact offline; never execute upstream content."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path,PurePosixPath
import stat
import sys
import zipfile
import urllib.request
import urllib.parse
import urllib.error
import tempfile
import shutil

ROOT=Path(__file__).resolve().parents[1]


def source_catalog():
    return json.loads((ROOT/'references/source-assets.json').read_text(encoding='utf-8'))


def source_entry(repo_id):
    catalog=source_catalog()
    entry=next((item for item in catalog['repositories'] if item['id']==repo_id),None)
    if entry is None:raise ValueError(f'Unknown source ID: {repo_id}')
    with gzip.open(ROOT/entry['file_manifest'],'rt',encoding='utf-8') as handle:
        manifest=json.load(handle)
    return entry,manifest


def read_bytes(repo_id,path):
    requested=PurePosixPath(path.replace('\\','/'))
    if requested.is_absolute() or '..' in requested.parts or not requested.parts or ':' in requested.parts[0]:
        raise ValueError('Source path must be relative and cannot contain traversal.')
    entry,manifest=source_entry(repo_id)
    key=requested.as_posix()
    record=next((item for item in manifest['files'] if item['path']==key),None)
    if record is None:raise ValueError(f'No tracked source path {key} in {repo_id}; use --list.')
    expanded=ROOT/entry.get('expanded_root',f'assets/upstream/{repo_id}')/key
    location=None
    if expanded.is_file() and not expanded.is_symlink():
        raw=expanded.read_bytes();location=str(expanded)
    else:
        archive=ROOT/'assets/upstream-archives'/entry['archive_file']
        if archive.is_file():
            with zipfile.ZipFile(archive) as package: raw=package.read(key)
            location=str(archive)+'!/'+key
        else:
            release=source_catalog()['github_release']
            location='https://raw.githubusercontent.com/'+release['repository']+'/main/assets/upstream/'+repo_id+'/'+urllib.parse.quote(key)
            try:
                with urllib.request.urlopen(location,timeout=40) as response: raw=response.read()
            except urllib.error.HTTPError as error:
                if error.code!=404: raise
                location='https://github.com/'+release['repository']+'/releases/download/'+release['tag']+'/'+entry['archive_file']
                with tempfile.TemporaryDirectory(prefix='smart-car-source-') as temporary:
                    folder=Path(temporary).resolve()
                    assert folder.parent==Path(tempfile.gettempdir()).resolve()
                    downloaded=folder/'snapshot.zip'
                    with urllib.request.urlopen(location,timeout=60) as response,downloaded.open('wb') as output:
                        shutil.copyfileobj(response,output,1024*1024)
                    with downloaded.open('rb') as handle:
                        if hashlib.file_digest(handle,'sha256').hexdigest()!=entry['archive_sha256']: raise ValueError('Remote archive checksum mismatch')
                    with zipfile.ZipFile(downloaded) as package: raw=package.read(key)
                location+='!/'+key
    if hashlib.sha256(raw).hexdigest()!=record['sha256']:
        raise ValueError('Source checksum mismatch. Keep reference snapshots unchanged; edit a working project copy.')
    return raw,record,location


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo-id',required=True)
    parser.add_argument('--path')
    parser.add_argument('--list',action='store_true')
    parser.add_argument('--prefix',default='')
    parser.add_argument('--limit',type=int,default=50)
    parser.add_argument('--start',type=int,default=1)
    parser.add_argument('--lines',type=int,default=80)
    parser.add_argument('--output',help='Copy the verified file bytes to a new file without running them.')
    args=parser.parse_args()
    if min(args.limit,args.start,args.lines)<1:parser.error('Limits and line numbers must be positive.')
    try:
        if args.list:
            entry,manifest=source_entry(args.repo_id)
            rows=[item for item in manifest['files'] if item['path'].startswith(args.prefix)]
            for item in rows[:args.limit]:print(f"{item['bytes']:>12}  {item['path']}")
            print(f'Showing {min(len(rows),args.limit)} of {len(rows)} matching files; revision {entry.get("revision","user-supplied")}.')
            return
        if not args.path:parser.error('Specify --path or --list.')
        raw,record,location=read_bytes(args.repo_id,args.path)
        if args.output:
            target=Path(args.output)
            if target.exists():raise ValueError('Output already exists; choose a new working-copy path.')
            if record.get('mode')=='120000':raise ValueError('Refusing to materialize a symlink as an executable file.')
            target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(raw)
            print(f'Copied {len(raw)} verified bytes to {target.resolve()}')
            return
        print(location)
        print('SHA256 '+record['sha256'])
        if len(raw)>2_000_000 or b'\0' in raw[:8192]:
            print(f'Binary or large artifact ({len(raw)} bytes); use --output to create a working copy.')
            return
        try:text=raw.decode('utf-8-sig')
        except UnicodeError:text=raw.decode('gb18030',errors='replace')
        lines=text.splitlines()
        for index in range(args.start-1,min(len(lines),args.start-1+args.lines)):
            print(f'L{index+1}: {lines[index]}')
    except (OSError,ValueError,KeyError,zipfile.BadZipFile) as exc:parser.error(str(exc))


if __name__=='__main__':
    if hasattr(sys.stdout,'reconfigure'):sys.stdout.reconfigure(encoding='utf-8')
    main()
