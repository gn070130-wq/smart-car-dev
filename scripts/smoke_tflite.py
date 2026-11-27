"""CPU-only TFLite allocation/invoke smoke test using a synthetic zero tensor."""
import argparse
import hashlib
import importlib
import json
import math
from pathlib import Path
import sys


def run(path):
    path=Path(path)
    data=path.read_bytes()
    if len(data)<8 or data[4:8]!=b'TFL3':raise ValueError('Expected a TFLite FlatBuffer with TFL3 identifier.')
    np=importlib.import_module('numpy')
    try:interpreter_type=importlib.import_module('tflite_runtime.interpreter').Interpreter
    except ImportError:interpreter_type=importlib.import_module('tensorflow').lite.Interpreter
    interpreter=interpreter_type(model_content=data,num_threads=1)
    inputs=interpreter.get_input_details()
    for item in inputs:
        shape=[int(x) for x in item['shape']]
        if not shape or any(x<=0 for x in shape) or math.prod(shape)>1_000_000:
            raise ValueError('Current allocated input shapes must be positive and up to 1,000,000 elements; this tool does not resize dynamic inputs.')
    interpreter.allocate_tensors()
    details=[]
    for item in interpreter.get_input_details():
        interpreter.set_tensor(item['index'],np.zeros(item['shape'],dtype=item['dtype']))
        details.append({'name':item['name'],'shape':[int(x) for x in item['shape']],
            'shape_signature':[int(x) for x in item.get('shape_signature',item['shape'])],
            'dtype':str(item['dtype']),'quantization':list(item['quantization'])})
    interpreter.invoke()
    outputs=[]
    for item in interpreter.get_output_details():
        values=interpreter.get_tensor(item['index'])
        finite=bool(np.isfinite(values).all())
        outputs.append({'name':item['name'],'shape':list(values.shape),'dtype':str(values.dtype),
            'all_finite':finite,'min':float(values.min()) if values.size and finite else None,
            'max':float(values.max()) if values.size and finite else None})
    return {'model':str(path),'sha256':hashlib.sha256(data).hexdigest(),'inputs':details,'outputs':outputs,
        'scope':'CPU synthetic-zero allocation/invoke only; no image preprocessing, semantic accuracy, TFLM operator coverage or vehicle timing validation.'}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('model_file')
    args=parser.parse_args()
    try:result=run(args.model_file)
    except (ImportError,OSError,ValueError,RuntimeError) as exc:parser.error(str(exc))
    print(json.dumps(result,ensure_ascii=False,indent=2,allow_nan=False))


if __name__=='__main__':
    if hasattr(sys.stdout,'reconfigure'):sys.stdout.reconfigure(encoding='utf-8')
    main()
