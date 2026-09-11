"""Adversarial checks for the game-owned capture boundary; no Unreal install needed."""
import copy
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

SPEC = importlib.util.spec_from_file_location('capture', Path(__file__).parents[1] / 'tools' / 'validate_capture.py')
capture = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(capture)


def fixture():
    """Synthetic harness run: start -> advance -> summon -> victory."""
    meta = {
        'project_id': 'dread-meridian', 'scenario_id': 'foundation-harness',
        'run_kind': 'foundation_harness', 'seed': 1927, 'rng_schema_version': 1,
        'ritual_points_per_stage': 100, 'investigator_slots': 4, 'production_bots': 0,
        'engine_version': '5.8.2-test', 'capture_version': '0.1.0',
        'started_at_utc': '2026-09-10T18:00:00Z', 'working_tree_dirty': True,
        'GameRevision': 'a' * 40, 'SourceDigest': 'b' * 64, 'GDDDigest': 'c' * 64,
    }
    states = [
        ('run.started', meta),
        ('run.state_changed', {'phase': 'Expedition', 'ritual_stage': 'Incipient', 'ritual_progress': 0}),
        ('ritual.advanced', {'phase': 'Expedition', 'ritual_stage': 'Stirring', 'ritual_progress': 1, 'points': 101}),
        ('ritual.summoned', {'phase': 'Apocalypse', 'ritual_stage': 'Apocalypse', 'ritual_progress': 0}),
        ('run.state_changed', {'phase': 'Victory', 'ritual_stage': 'Apocalypse', 'ritual_progress': 0}),
        ('run.ended', {'outcome': 'victory'}),
    ]
    return [{'schema_version': 1, 'run_id': 'e9a2d251-2011-4aa2-8739-a99bdd15f5e8',
             'sequence': index, 'event_type': kind, 'elapsed_seconds': index * 0.1,
             'authority': 'server', 'visibility': 'developer', 'data': data}
            for index, (kind, data) in enumerate(states)]


class CaptureTests(unittest.TestCase):
    def validate(self, events):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'events.jsonl'
            path.write_text(''.join(json.dumps(event) + '\n' for event in events), encoding='utf-8')
            return capture.validate_capture(path)

    def test_complete_capture_preserves_scope(self):
        report = self.validate(fixture())
        self.assertTrue(report['provenance_complete'])
        self.assertEqual(report['outcome'], 'victory')
        self.assertFalse(report['supports_gameplay_balance_claims'])

    def test_unrecorded_provenance_is_not_fabricated(self):
        events = fixture()
        events[0]['data']['GameRevision'] = 'unrecorded'
        self.assertFalse(self.validate(events)['provenance_complete'])

    def test_rejects_bad_envelopes(self):
        for key, bad in [('schema_version', 2), ('sequence', 42), ('authority', 'client'),
                         ('visibility', 'public'), ('run_id', 'bad'), ('run_id', None),
                         ('elapsed_seconds', -1), ('elapsed_seconds', float('nan')),
                         ('elapsed_seconds', True), ('data', []), ('event_type', None)]:
            with self.subTest(key=key, bad=bad):
                events = fixture()
                events[2][key] = bad
                with self.assertRaises(ValueError):
                    self.validate(events)

    def test_rejects_mixed_runs_and_truncation(self):
        events = fixture()
        events[2]['run_id'] = '00000000-0000-0000-0000-000000000000'
        for broken in [events, fixture()[:-1], []]:
            with self.assertRaises(ValueError):
                self.validate(broken)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'events.jsonl'
            path.write_text('{"schema_version":', encoding='utf-8')
            with self.assertRaisesRegex(ValueError, 'truncated'):
                capture.validate_capture(path)

    def test_rejects_state_fabrication_and_wrong_result(self):
        for index, key, value in [(2, 'ritual_progress', 2), (2, 'points', -1),
                                  (2, 'phase', 'Victory'), (5, 'outcome', 'defeat'),
                                  (5, 'outcome', 'aborted')]:
            events = fixture()
            events[index]['data'][key] = value
            with self.assertRaises(ValueError):
                self.validate(events)

    def test_aborted_run_is_distinct(self):
        events = fixture()[:3]
        end = copy.deepcopy(fixture()[-1])
        end['sequence'] = 3
        end['data']['outcome'] = 'aborted'
        events.append(end)
        self.assertEqual(self.validate(events)['outcome'], 'aborted')

    def test_natural_apocalypse_and_early_wipe(self):
        events = fixture()
        events[2]['data'] = {'phase': 'Apocalypse', 'ritual_stage': 'Apocalypse', 'ritual_progress': 0, 'points': 401}
        events.pop(3)
        for index, event in enumerate(events):
            event['sequence'] = index
        self.assertEqual(self.validate(events)['outcome'], 'victory')
        events = fixture()[:2] + [
            {**fixture()[4], 'sequence': 2, 'data': {'phase': 'Defeat', 'ritual_stage': 'Incipient', 'ritual_progress': 0}},
            {**fixture()[5], 'sequence': 3, 'data': {'outcome': 'defeat'}},
        ]
        self.assertEqual(self.validate(events)['outcome'], 'defeat')


if __name__ == '__main__':
    unittest.main()
