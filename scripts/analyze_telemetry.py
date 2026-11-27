"""Read-only CSV statistics for logged smart-car samples, not automatic PID tuning."""
import argparse
import csv
import json
import math
from pathlib import Path
import sys


def number(value):
    try:
        result = float(value)
        return result if math.isfinite(result) else None
    except (TypeError, ValueError):
        return None


def percentile(values, q):
    ordered = sorted(values)
    pos = (len(ordered)-1)*q
    lo, hi = math.floor(pos), math.ceil(pos)
    return ordered[lo] + (ordered[hi]-ordered[lo])*(pos-lo)


def stats(values):
    if not values:
        return {'n': 0}
    return {
        'n': len(values), 'min': min(values), 'mean': sum(values)/len(values),
        'p50': percentile(values, .50), 'p95': percentile(values, .95),
        'p99': percentile(values, .99), 'max': max(values)
    }


def error_stats(values):
    if not values:
        return {'n': 0}
    return {
        'n': len(values), 'bias': sum(values)/len(values),
        'rms': math.hypot(*values)/math.sqrt(len(values)),
        'max_abs': max(abs(x) for x in values)
    }


def analyze(path, expected_period_ms=None, pwm_limit=None):
    with Path(path).open(encoding='utf-8-sig', newline='') as handle:
        reader = csv.DictReader(handle)
        if not reader.fieldnames or 'timestamp_ms' not in reader.fieldnames:
            raise ValueError('CSV must include timestamp_ms in milliseconds.')
        rows = list(reader)
    if not rows:
        raise ValueError('CSV has no data rows.')
    result = {'rows': len(rows), 'warnings': [], 'units': {
        'time': 'ms', 'wheel_speed': 'm/s', 'lateral_error': 'project-defined',
        'pwm': 'same unit as declared pwm limit'
    }}
    times = [number(row.get('timestamp_ms')) for row in rows]
    invalid_timestamps = sum(t is None for t in times)
    # Never join across an invalid row to invent an apparent control interval.
    intervals = [b-a for a,b in zip(times, times[1:]) if a is not None and b is not None]
    positive = [x for x in intervals if x > 0]
    result['invalid_timestamp_rows'] = invalid_timestamps
    result['nonpositive_intervals'] = sum(x <= 0 for x in intervals)
    result['record_interval_ms'] = stats(positive)
    if invalid_timestamps:
        result['warnings'].append('Invalid timestamps excluded; intervals spanning them are not joined.')
    if result['nonpositive_intervals']:
        result['warnings'].append('Duplicate or reversed timestamps found; fix the timebase before interpreting timing.')
    if expected_period_ms is not None:
        if not math.isfinite(expected_period_ms) or expected_period_ms <= 0:
            raise ValueError('Expected period must be finite and positive.')
        result['expected_period_ms'] = expected_period_ms
        result['intervals_above_1_5x_expected'] = sum(x > 1.5*expected_period_ms for x in positive)
        result['period_error_ms'] = error_stats([x-expected_period_ms for x in positive])
    ages = []
    for row, timestamp in zip(rows, times):
        vision = number(row.get('vision_timestamp_ms'))
        if timestamp is not None and vision is not None:
            ages.append(timestamp-vision)
    result['negative_vision_age_rows'] = sum(x < 0 for x in ages)
    result['vision_age_ms'] = stats([x for x in ages if x >= 0])
    if result['negative_vision_age_rows']:
        result['warnings'].append('Negative vision age found; verify clock domains and timestamp semantics.')
    result['lateral_error'] = error_stats([
        x for row in rows if (x := number(row.get('lateral_error'))) is not None
    ])
    for side in ('left', 'right'):
        errors = []
        for row in rows:
            target = number(row.get(f'{side}_target_mps'))
            speed = number(row.get(f'{side}_speed_mps'))
            if target is not None and speed is not None:
                errors.append(target-speed)
        result[f'{side}_speed_error_mps'] = error_stats(errors)
        if pwm_limit is not None:
            if not math.isfinite(pwm_limit) or pwm_limit <= 0:
                raise ValueError('PWM limit must be finite and positive in the logged PWM unit.')
            outputs = [x for row in rows if (x := number(row.get(f'{side}_pwm'))) is not None]
            result[f'{side}_pwm_limit_hits'] = {
                'n': len(outputs), 'limit': pwm_limit,
                'count': sum(abs(x) >= pwm_limit for x in outputs),
                'fraction': sum(abs(x) >= pwm_limit for x in outputs)/len(outputs) if outputs else None
            }
    for column, output in (('state','state_transitions'), ('frame_id','repeated_frame_pairs')):
        pairs = [(a.get(column),b.get(column)) for a,b in zip(rows,rows[1:])
                 if a.get(column) not in (None,'') and b.get(column) not in (None,'')]
        result[output] = {'valid_pairs': len(pairs), 'count': sum(
            (a != b if column == 'state' else a == b) for a,b in pairs
        )}
    result['warnings'].append('Record intervals describe this CSV sampling point, not necessarily control scheduling.')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('csv_file')
    parser.add_argument('--expected-period-ms', type=float)
    parser.add_argument('--pwm-limit', type=float)
    args = parser.parse_args()
    try:
        result = analyze(args.csv_file, args.expected_period_ms, args.pwm_limit)
    except (ValueError, OSError, csv.Error) as exc:
        parser.error(str(exc))
    print(json.dumps(result, ensure_ascii=False, indent=2, allow_nan=False))


if __name__ == '__main__':
    if hasattr(sys.stdout, 'reconfigure'):
        sys.stdout.reconfigure(encoding='utf-8')
    main()
