"""Offline integrity and behavior checks; run from any working directory."""
from pathlib import Path
import hashlib
import importlib.util
import json
import math
import re
import sys
import tempfile

sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1]


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
        if '.git' in path.parts:continue
        data=json.loads(path.read_text(encoding='utf-8-sig'))
        assert isinstance(data,(dict,list)),path
        assert not re.search(r'"(?:original_local_path|local_snapshot_path)"\s*:',path.read_text(encoding='utf-8-sig')),path
    for path in ROOT.rglob('*.md'):
        if '.git' in path.parts:continue
        content=path.read_text(encoding='utf-8-sig')
        assert '[TODO:' not in content,path
        for target in re.findall(r'\]\(([^)]+)\)',content):
            if '://' in target or target.startswith('#'):continue
            relative=target.split('#',1)[0]
            assert (path.parent/relative).exists(),(path,target)
    corpus=ROOT/'references/corpus'
    index=json.loads((corpus/'index.json').read_text(encoding='utf-8'))
    assert [row['id'] for row in index]==list(range(1,54))
    for row in index:
        raw=(corpus/row['text_file']).read_bytes()
        assert hashlib.sha256(raw).hexdigest()==row['text_sha256'],row['id']
        assert len(raw.decode('utf-8').splitlines())==row['lines']
    catalog=json.loads((ROOT/'references/repos/catalog.json').read_text(encoding='utf-8'))['repositories']
    assert len(catalog)==13
    for row in catalog:
        folder=ROOT/'references/repos'/row['id']
        manifest=json.loads((folder/'manifest.json').read_text(encoding='utf-8'))
        symbols=json.loads((folder/'symbols.json').read_text(encoding='utf-8'))['symbols']
        assert manifest['revision']==row['revision']
        known={item['path'] for item in manifest['files']}
        assert all(item['path'] in known and item['line']>0 for item in symbols)
    search=load('search_reports')
    assert search.search('LADRC',[40],limit=1)[0]['id']==40
    lookup=load('lookup_open_source')
    assert lookup.lookup('main_run')[0]['path']=='project/src/start.cpp'
    assert lookup.lookup('YoloDetector::load_model',repo_id='smart2026')
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
    print('PASS: portable skill, 53 corpus hashes, 13 repository indexes, links and offline tool behavior.')


if __name__=='__main__':
    main()
