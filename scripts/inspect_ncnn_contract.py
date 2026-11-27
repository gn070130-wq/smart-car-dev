"""Inspect text NCNN graph structure and required blob names without executing a model."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import sys


def inspect(path, expected_inputs=(), expected_outputs=(), weights=None):
    path=Path(path)
    data=path.read_bytes()
    lines=[line.strip() for line in data.decode('utf-8-sig').splitlines() if line.strip() and not line.lstrip().startswith('#')]
    if len(lines)<2 or lines[0]!='7767517':
        raise ValueError('Expected NCNN text .param magic 7767517; binary param is not supported.')
    counts=lines[1].split()
    if len(counts)!=2:raise ValueError('Invalid layer/blob count header.')
    declared_layers,declared_blobs=map(int,counts)
    layers=[]
    produced=set();consumed=set();inputs=[];all_blobs=set()
    for row,line in enumerate(lines[2:],3):
        tokens=line.split()
        if len(tokens)<4:raise ValueError(f'Invalid layer record at nonempty line {row}.')
        bottoms,tops=int(tokens[2]),int(tokens[3])
        if min(bottoms,tops)<0 or len(tokens)<4+bottoms+tops:
            raise ValueError(f'Invalid blob arity at nonempty line {row}.')
        bottom_names=tokens[4:4+bottoms];top_names=tokens[4+bottoms:4+bottoms+tops]
        consumed.update(bottom_names);produced.update(top_names)
        all_blobs.update(bottom_names+top_names)
        if tokens[0]=='Input':inputs.extend(top_names)
        layers.append({'type':tokens[0],'name':tokens[1],'bottoms':bottom_names,'tops':top_names,'params':tokens[4+bottoms+tops:]})
    errors=[]
    if len(layers)!=declared_layers:errors.append(f'Header declares {declared_layers} layers but parsed {len(layers)}.')
    if len(all_blobs)!=declared_blobs:errors.append(f'Header declares {declared_blobs} blobs but parsed {len(all_blobs)} unique blob names.')
    for name in expected_inputs:
        if name not in inputs:errors.append(f'Required Input blob missing: {name}')
    # ncnn allows extraction of intermediate blobs, not only terminal outputs.
    for name in expected_outputs:
        if name not in produced:errors.append(f'Required extract blob missing: {name}')
    seen=set()
    for layer in layers:
        for name in layer['bottoms']:
            if name not in seen:errors.append(f"{layer['name']} reads blob before production: {name}")
        seen.update(layer['tops'])
    weight_path=Path(weights) if weights else path.with_suffix('.bin')
    notes=['Static graph check only; does not verify operator availability, tensor shapes, weight compatibility or prediction accuracy.',
           'Terminal output detection is conservative for graphs reusing blob names in-place.']
    if not weight_path.is_file():notes.append('Companion weights are absent locally; runtime inference has not been validated.')
    return {'file':str(path),'sha256':hashlib.sha256(data).hexdigest(),'layers':len(layers),
            'blobs':len(all_blobs),'inputs':inputs,'terminal_outputs':sorted(produced-consumed),
            'layer_types':dict(Counter(layer['type'] for layer in layers)),
            'has_softmax':any(layer['type']=='Softmax' for layer in layers),
            'weights_path':str(weight_path),'weights_present':weight_path.is_file(),
            'expected_inputs':list(expected_inputs),'expected_outputs':list(expected_outputs),
            'errors':errors,'notes':notes}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('param_file')
    parser.add_argument('--input',action='append',default=[])
    parser.add_argument('--output',action='append',default=[])
    parser.add_argument('--weights')
    args=parser.parse_args()
    try:result=inspect(args.param_file,args.input,args.output,args.weights)
    except (OSError,ValueError,UnicodeError) as exc:parser.error(str(exc))
    print(json.dumps(result,ensure_ascii=False,indent=2))
    return 1 if result['errors'] else 0


if __name__=='__main__':
    if hasattr(sys.stdout,'reconfigure'):sys.stdout.reconfigure(encoding='utf-8')
    sys.exit(main())
