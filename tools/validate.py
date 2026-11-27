"""Offline integrity and behavior checks; run from any working directory."""
from pathlib import Path
import hashlib
import importlib.util
import json
import math
import re
import sys
import tempfile
import gzip

sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1]


def is_upstream_data(path):
    relative=path.relative_to(ROOT).parts
    return len(relative)>1 and relative[0]=='assets' and relative[1] in ('upstream','upstream-archives','upstream-manifests')


def load(name):
    spec=importlib.util.spec_from_file_location(name,ROOT/'scripts'/f'{name}.py')
    module=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main():
    skill=(ROOT/'SKILL.md').read_text(encoding='utf-8-sig')
    assert skill.startswith('---\n')
    assert re.search(r'^name: smart-car-dev$',skill,re.M)
    assert re.search(r'^description: .+',skill,re.M)
    for path in ROOT.rglob('*.json'):
        if '.git' in path.parts or is_upstream_data(path):continue
        data=json.loads(path.read_text(encoding='utf-8-sig'))
        assert isinstance(data,(dict,list)),path
        assert not re.search(r'"(?:original_local_path|local_snapshot_path)"\s*:',path.read_text(encoding='utf-8-sig')),path
    for path in ROOT.rglob('*.md'):
        if '.git' in path.parts or is_upstream_data(path):continue
        content=path.read_text(encoding='utf-8-sig')
        assert '[TODO:' not in content,path
        for target in re.findall(r'\]\(([^)]+)\)',content):
            if '://' in target or target.startswith('#'):continue
            relative=target.split('#',1)[0]
            assert (path.parent/relative).exists(),(path,target)
    for path in ROOT.rglob('*'):
        if not path.is_file() or '.git' in path.parts:continue
        if path.suffix not in {'.md','.json','.py','.yaml','.csv','.txt'}:continue
        assert '\ufffd' not in path.read_text(encoding='utf-8-sig',errors='replace'),path
    corpus=ROOT/'references/corpus'
    index=json.loads((corpus/'index.json').read_text(encoding='utf-8'))
    assert [row['id'] for row in index]==list(range(1,54))
    for row in index:
        raw=(corpus/row['text_file']).read_bytes()
        assert hashlib.sha256(raw).hexdigest()==row['text_sha256'],row['id']
        text=raw.decode('utf-8').replace('\r\n','\n')
        assert len(text.splitlines())==row['lines'],row['id']
        assert len(text)==row['characters'],row['id']
    catalog=json.loads((ROOT/'references/repos/catalog.json').read_text(encoding='utf-8'))['repositories']
    assert len(catalog)==13
    files_total=symbols_total=0
    for row in catalog:
        folder=ROOT/'references/repos'/row['id']
        manifest=json.loads((folder/'manifest.json').read_text(encoding='utf-8'))
        symbols=json.loads((folder/'symbols.json').read_text(encoding='utf-8'))['symbols']
        assert manifest['revision']==row['revision']
        assert len(manifest['files'])==row['files'],row['id']
        assert len(symbols)==row['symbols'],row['id']
        assert (ROOT/'references/repos'/row['review']).exists(),row['id']
        known={item['path'] for item in manifest['files']}
        assert all(item['path'] in known and item['line']>0 for item in symbols)
        files_total+=len(manifest['files'])
        symbols_total+=len(symbols)
    validation=json.loads((ROOT/'references/provenance.json').read_text(encoding='utf-8'))['multi_repo_validation']
    assert files_total==validation['file_or_archive_member_records']
    assert symbols_total==validation['definition_candidates']
    search=load('search_reports')
    assert search.search('LADRC',[40],limit=1)[0]['id']==40
    lookup=load('lookup_open_source')
    assert lookup.lookup('main_run')[0]['path']=='project/src/start.cpp'
    assert lookup.lookup('YoloDetector::load_model',repo_id='smart2026')
    if (ROOT/'references/source-assets.json').exists():
        sources=json.loads((ROOT/'references/source-assets.json').read_text(encoding='utf-8'))
        assert sources['counts']['unique_upstream_snapshots']==13
        assert sources['counts']['original_reports']==53
        count=0
        for entry in sources['repositories']:
            with gzip.open(ROOT/entry['file_manifest'],'rt',encoding='utf-8') as handle:
                manifest=json.load(handle)
            for item in manifest.get('expanded_files',[]):
                source=ROOT/entry['expanded_root']/item['path']
                assert hashlib.sha256(source.read_bytes()).hexdigest()==item['sha256'],source
                count+=1
        assert count==sources['counts']['expanded_files']
        actual=lookup.lookup('YoloDetector::load_model',repo_id='smart2026',limit=1)
        assert actual[0].get('local_source') and actual[0].get('verified_local_context')
    try:
        lookup.lookup('main',repo_id='../outside')
        raise AssertionError('Path-like repository ID accepted')
    except ValueError:pass
    inspect=load('inspect_ncnn_contract')
    telemetry=load('analyze_telemetry')
    with tempfile.TemporaryDirectory(prefix='smart-car-validate-') as temporary:
        temp=Path(temporary)
        model=temp/'sample.param'
        model.write_text('7767517\n2 2\nInput in 0 1 in0\nSoftmax out 1 1 in0 out0\n',encoding='utf-8')
        assert not inspect.inspect(model,['in0'],['out0'])['errors']
        assert inspect.inspect(model,['in0'],['out1'])['errors']
        log=temp/'run.csv'
        log.write_text('timestamp_ms,left_target_mps,left_speed_mps\n0,1,0.8\n5,1,0.8\n10,1,0.8\n',encoding='utf-8')
        analysis=telemetry.analyze(log,5)
        assert analysis['record_interval_ms']['mean']==5
        assert math.isclose(analysis['left_speed_error_mps']['rms'],.2)
        log.write_text('timestamp_ms,lateral_error\n0,nan\n0,inf\n5,1\n',encoding='utf-8')
        analysis=telemetry.analyze(log)
        assert analysis['nonpositive_intervals']==1
        assert analysis['lateral_error']['n']==1
    print('PASS: portable skill, 53 corpus hashes, 13 repository indexes, encoding guards, link and offline tool behavior.')


if __name__=='__main__':
    main()
