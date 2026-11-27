"""Restore full source assets from this repository's release, then verify hashes."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import urllib.request
import urllib.parse

ROOT=Path(__file__).resolve().parents[1]


def digest(path):
    with path.open('rb') as handle:return hashlib.file_digest(handle,'sha256').hexdigest()


def github_environment(owner):
    env={**os.environ,'GIT_TERMINAL_PROMPT':'0','GCM_INTERACTIVE':'Never'}
    status=subprocess.run(['gh','auth','status'],capture_output=True,env=env,timeout=20)
    if status.returncode==0 or env.get('GH_TOKEN') or env.get('GITHUB_TOKEN'):return env
    credential=subprocess.run(['git','-c','credential.interactive=never','credential','fill'],
        input=f'protocol=https\nhost=github.com\nusername={owner}\n\n',capture_output=True,text=True,env=env,timeout=20)
    if credential.returncode:raise ValueError('Sign in to GitHub CLI or Git Credential Manager to download private release assets.')
    fields=dict(line.split('=',1) for line in credential.stdout.splitlines() if '=' in line)
    if not fields.get('password'):raise ValueError('No stored GitHub credential available.')
    env['GH_TOKEN']=fields['password']
    return env


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo-id',action='append',help='Restore specified sources; default is every source.')
    parser.add_argument('--verify-only',action='store_true')
    args=parser.parse_args()
    catalog=json.loads((ROOT/'references/source-assets.json').read_text(encoding='utf-8'))
    selected=[entry for entry in catalog['repositories'] if not args.repo_id or entry['id'] in args.repo_id]
    if args.repo_id and set(args.repo_id)-{entry['id'] for entry in selected}:parser.error('Unknown source ID.')
    destination=ROOT/'assets/upstream-archives'
    destination.mkdir(parents=True,exist_ok=True)
    completed=set();env=None
    try:
        for entry in selected:
            name=entry['archive_file']
            if Path(name).name!=name or '/' in name or '\\' in name:raise ValueError('Invalid archive asset name.')
            if name in completed:continue
            completed.add(name)
            target=destination/name
            if target.exists():
                if digest(target)!=entry['archive_sha256']:raise ValueError(f'Checksum mismatch: {name}; preserve and inspect it before replacing.')
                print('VERIFIED',name);continue
            if args.verify_only:raise ValueError(f'Missing archive: {name}')
            release=catalog['github_release']
            if not re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+',release['repository']):raise ValueError('Invalid GitHub repository.')
            if release.get('visibility')=='public':
                url=f"https://github.com/{release['repository']}/releases/download/{urllib.parse.quote(release['tag'])}/{urllib.parse.quote(name)}"
                partial=target.with_suffix(target.suffix+'.partial')
                request=urllib.request.Request(url,headers={'User-Agent':'smart-car-dev-source-installer'})
                with urllib.request.urlopen(request,timeout=60) as response,partial.open('wb') as output:
                    while chunk:=response.read(1024*1024):output.write(chunk)
                if digest(partial)!=entry['archive_sha256']:raise ValueError(f'Checksum mismatch after download: {name}')
                partial.replace(target)
            else:
                if env is None:env=github_environment(release['repository'].split('/')[0])
                result=subprocess.run(['gh','release','download',release['tag'],'--repo',release['repository'],
                    '--pattern',name,'--dir',str(destination)],env=env,capture_output=True,text=True,timeout=1800)
                if result.returncode:raise ValueError('Release download failed: '+result.stderr.strip())
            if digest(target)!=entry['archive_sha256']:raise ValueError(f'Checksum mismatch after download: {name}')
            print('DOWNLOADED',name)
    except (OSError,ValueError,subprocess.TimeoutExpired) as exc:parser.error(str(exc))


if __name__=='__main__':
    if hasattr(sys.stdout,'reconfigure'):sys.stdout.reconfigure(encoding='utf-8')
    main()
