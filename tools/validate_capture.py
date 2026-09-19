"""Validate foundation-harness and local test-arena evidence. Python 3.12+, stdlib only.

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
    arena = meta.get('scenario_id') == 'test-arena-v1' and meta.get('run_kind') == 'test_arena'
    if not arena:
        require(meta.get('scenario_id') == 'foundation-harness' and meta.get('run_kind') == 'foundation_harness',
                'This validator supports only foundation harness and test arena captures')
        require(meta.get('production_bots') == 0 and meta.get('investigator_slots') == 4,
                'Harness roster metadata is invalid')
    require(integer(meta.get('seed')) and -(2**31) <= meta['seed'] < 2**31, 'Invalid seed')
    require(meta.get('rng_schema_version') in (1, 2), 'Unsupported RNG schema')
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

    if arena:
        return validate_arena_events(events, provenance_complete)

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


def validate_arena_events(events: list[dict], provenance_complete: bool) -> dict:
    """Validate arena setup/lifecycle evidence, not reproduce combat or AI outcomes."""
    meta = events[0]['data']
    heroes = {'Sapper', 'Photographer', 'Medium', 'Smuggler'}
    types = {'Gunman', 'Bruiser', 'Lookout', 'Bomber', 'Gang Boss',
             'Crawler', 'Lurker', 'Spitter', 'Grasper', 'Old Thing'}
    player, companions = meta.get('controlled_investigator'), meta.get('arena_companions')
    require(player in heroes and isinstance(companions, list) and all(isinstance(c, str) and c in heroes for c in companions),
            'Invalid arena party')
    require(len(set([player] + companions)) == 1 + len(companions) <= 4, 'Duplicate or oversized arena party')
    require(meta.get('investigator_slots') == 1 + len(companions)
            and meta.get('initial_bot_count') == len(companions), 'Incorrect arena roster metadata')
    require(meta.get('arena_config_version') == 1, 'Unsupported arena configuration')
    phase, restored, combat_tick = 'Setup', False, -1
    transitions = {'Setup': {'Fighting'}, 'Fighting': {'Paused', 'Victory', 'Defeat'}, 'Paused': {'Fighting'}}
    for event in events[1:-1]:
        if event['event_type'] != 'arena.setup':
            continue
        data = event['data']
        tick = data.get('tick')
        require(integer(tick) and tick >= combat_tick, 'Invalid arena combat tick')
        combat_tick = tick
        require(data.get('player') == player and data.get('seed') == meta['seed'], 'Setup disagrees with run metadata')
        placements = data.get('placements')
        require(isinstance(placements, list) and len(placements) <= 32, 'Invalid arena placements')
        for p in placements:
            require(isinstance(p, dict) and p.get('type') in types, 'Unknown arena enemy')
            require(integer(p.get('batch')) and p['batch'] >= 0, 'Invalid placement batch')
            for axis, extent in (('x', 2800), ('y', 2300)):
                value = p.get(axis)
                require(type(value) in (int, float) and math.isfinite(value) and abs(value) <= extent,
                        'Placement outside arena')
        action, target = data.get('action'), data.get('phase')
        if action == 'restored':
            require(not restored and target == 'Setup' and tick == 0, 'Invalid arena startup')
            restored = True
        else:
            require(restored, 'Arena action before restored setup')
            if action in ('phase_changed', 'completed'):
                require(target in transitions.get(phase, set()), 'Illegal arena transition')
                require(bool(placements), 'Fight without enemies')
                require((action == 'completed') == (target in ('Victory', 'Defeat')), 'Incorrect completion action')
                phase = target
            else:
                require(target == phase, 'Setup action changed phase')
                require(action in ('placed', 'undone', 'cleared', 'placement_failed', 'reset', 'probe_complete'), 'Unknown arena action')
                if action in ('undone', 'cleared'):
                    require(phase == 'Setup', 'Removal after combat began')
                if action in ('placed', 'placement_failed'):
                    require(phase in ('Setup', 'Paused'), 'Placement during live combat')
    require(restored, 'Missing arena setup')
    outcome = events[-1]['data'].get('outcome')
    require((outcome == 'victory' and phase == 'Victory') or (outcome == 'defeat' and phase == 'Defeat')
            or (outcome == 'aborted' and phase in ('Setup', 'Fighting', 'Paused')), 'Arena outcome contradicts phase')
    return {'contract': 'dread-meridian.test-arena-capture.v1', 'run_id': events[0]['run_id'],
            'outcome': outcome, 'event_count': len(events), 'provenance_complete': provenance_complete,
            'working_tree_dirty': meta['working_tree_dirty'], 'evidence_scope': 'local_test_arena_only',
            'supports_gameplay_balance_claims': False,
            'limitations': ['Validates setup and lifecycle; does not replay or validate combat outcomes.',
                            'No multiplayer, deterministic gameplay replay, or Play Trace ingestion claim.']}


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
