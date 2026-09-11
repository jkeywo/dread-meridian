"""Validate the v1 foundation-harness evidence contract. Python 3.12+, stdlib only.

This is a game-owned export check, not a Play Trace ingestion adapter.
"""
from __future__ import annotations

import argparse
import json
import math
import re
from collections import Counter
from pathlib import Path
from uuid import UUID

STAGES = ('Incipient', 'Stirring', 'Intrusion', 'Convergence', 'Apocalypse')
ENVELOPE = {'schema_version', 'run_id', 'sequence', 'event_type', 'elapsed_seconds',
            'authority', 'visibility', 'data'}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def integer(value: object) -> bool:
    return type(value) is int


def validate_capture(path: Path) -> dict:
    events = []
    with path.open(encoding='utf-8') as stream:
        for line_number, line in enumerate(stream, 1):
            require(bool(line.strip()), f'Blank event at line {line_number}')
            try:
                events.append(json.loads(line))
            except json.JSONDecodeError as error:
                raise ValueError(f'Invalid/truncated JSON at line {line_number}: {error}') from error
    require(len(events) >= 3, 'Capture is empty or incomplete')
    run_id = None
    elapsed = -1.0
    for sequence, event in enumerate(events):
        require(isinstance(event, dict) and set(event) == ENVELOPE, f'Invalid envelope at {sequence}')
        require(integer(event['schema_version']) and event['schema_version'] == 1, 'Unsupported schema')
        require(integer(event['sequence']) and event['sequence'] == sequence, 'Non-contiguous sequence')
        require(event['authority'] == 'server' and event['visibility'] == 'developer', 'Invalid evidence scope')
        require(isinstance(event['data'], dict), 'Event data must be an object')
        require(isinstance(event['event_type'], str), 'Event type must be a string')
        require(isinstance(event['run_id'], str), 'Run ID must be a UUID string')
        try:
            UUID(event['run_id'])
        except ValueError as error:
            raise ValueError('Invalid run UUID') from error
        run_id = run_id or event['run_id']
        require(event['run_id'] == run_id, 'Mixed runs in one capture')
        seconds = event['elapsed_seconds']
        require(type(seconds) in (int, float) and math.isfinite(seconds) and seconds >= 0 and seconds >= elapsed,
                'Invalid/non-monotonic elapsed time')
        elapsed = seconds

    require(events[0]['event_type'] == 'run.started', 'First event must be run.started')
    require(events[-1]['event_type'] == 'run.ended', 'Capture is incomplete: no final run.ended')
    meta = events[0]['data']
    require(meta.get('project_id') == 'dread-meridian', 'Wrong project')
    require(meta.get('scenario_id') == 'foundation-harness' and meta.get('run_kind') == 'foundation_harness',
            'This validator supports only the foundation harness')
    require(meta.get('production_bots') == 0 and meta.get('investigator_slots') == 4,
            'Harness roster metadata is invalid')
    require(integer(meta.get('seed')) and -(2**31) <= meta['seed'] < 2**31, 'Invalid seed')
    require(meta.get('rng_schema_version') == 1, 'Unsupported RNG schema')
    tuning = meta.get('ritual_points_per_stage')
    require(integer(tuning) and 0 < tuning < 2**31, 'Invalid ritual tuning')
    require(type(meta.get('working_tree_dirty')) is bool, 'Missing working tree status')
    for key in ('engine_version', 'capture_version', 'started_at_utc'):
        require(isinstance(meta.get(key), str) and bool(meta[key]), f'Missing {key}')
    provenance_complete = True
    for key, pattern in (('GameRevision', r'[0-9a-f]{40}|[0-9a-f]{64}'),
                         ('SourceDigest', r'[0-9a-f]{64}'), ('GDDDigest', r'[0-9a-f]{64}')):
        value = meta.get(key)
        require(isinstance(value, str) and (value == 'unrecorded' or re.fullmatch(pattern, value) is not None),
                f'Invalid {key}')
        provenance_complete &= value != 'unrecorded'

    phase, stage, progress = 'Briefing', 0, 0
    for event in events[1:-1]:
        kind, data = event['event_type'], event['data']
        if kind == 'run.state_changed':
            target = data.get('phase')
            legal = ((phase == 'Briefing' and target == 'Expedition')
                     or (phase in ('Expedition', 'Apocalypse') and target == 'Defeat')
                     or (phase == 'Apocalypse' and target == 'Victory'))
            require(legal, f'Illegal phase transition: {phase} -> {target}')
            phase = target
        elif kind == 'ritual.advanced':
            points = data.get('points')
            require(phase == 'Expedition' and integer(points) and 0 < points < 2**31, 'Illegal ritual input')
            stage += (progress + points) // tuning
            progress = (progress + points) % tuning
            if stage >= 4:
                phase, stage, progress = 'Apocalypse', 4, 0
        elif kind == 'ritual.summoned':
            require(phase == 'Expedition', 'Summon outside expedition')
            phase, stage, progress = 'Apocalypse', 4, 0
        else:
            raise ValueError(f'Unsupported or misplaced event: {kind}')
        require(data.get('phase') == phase and data.get('ritual_stage') == STAGES[stage]
                and integer(data.get('ritual_progress')) and data['ritual_progress'] == progress,
                f'Event state disagrees with accepted inputs at sequence {event["sequence"]}')

    outcome = events[-1]['data'].get('outcome')
    require(outcome in ('victory', 'defeat', 'aborted'), 'Invalid outcome')
    require((outcome == 'victory' and phase == 'Victory') or (outcome == 'defeat' and phase == 'Defeat')
            or (outcome == 'aborted' and phase in ('Expedition', 'Apocalypse')), 'Outcome contradicts run state')
    return {
        'contract': 'dread-meridian.foundation-capture.v1',
        'run_id': run_id, 'outcome': outcome, 'event_count': len(events),
        'event_counts': dict(Counter(event['event_type'] for event in events)),
        'provenance_complete': provenance_complete,
        'working_tree_dirty': meta['working_tree_dirty'],
        'evidence_scope': 'foundation_harness_only',
        'supports_gameplay_balance_claims': False,
        'limitations': ['No production bots, combat, objectives, or boss encounter.',
                        'Recorded source digests identify inputs; retain those inputs separately.',
                        'Wall-clock event timing is not deterministic simulation time.'],
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    parser.add_argument('--require-provenance', action='store_true')
    args = parser.parse_args()
    try:
        summary = validate_capture(args.capture)
        require(not args.require_provenance or summary['provenance_complete'], 'Provenance is unrecorded; launch via Unreal.ps1')
    except (OSError, ValueError) as error:
        parser.exit(1, f'Capture rejected: {error}\n')
    print(json.dumps(summary, indent=2))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
