"""Adversarial arena metadata / lifecycle checks."""
import copy
import unittest
import test_capture


def arena_fixture():
    events = test_capture.fixture()
    meta = events[0]['data']
    meta.update(scenario_id='test-arena-v1', run_kind='test_arena', investigator_slots=1,
                initial_bot_count=0, controlled_investigator='Sapper', arena_companions=[], arena_config_version=1)
    placement = {'type': 'Gunman', 'x': 500, 'y': 0, 'batch': 0}
    for index, (action, phase) in enumerate([('restored', 'Setup'), ('phase_changed', 'Fighting'), ('completed', 'Victory')], 2):
        events[index]['event_type'] = 'arena.setup'
        events[index]['data'] = {'action': action, 'phase': phase, 'player': 'Sapper', 'seed': 1927,
                                 'tick': index - 2, 'placements': [placement.copy()]}
    return events


class ArenaCaptureTests(unittest.TestCase):
    validate = test_capture.CaptureTests.validate

    def test_valid_local_scope(self):
        result = self.validate(arena_fixture())
        self.assertEqual(result['evidence_scope'], 'local_test_arena_only')
        self.assertTrue(result['provenance_complete'])
        self.assertFalse(result['supports_gameplay_balance_claims'])

    def test_rejects_false_party_metadata(self):
        for key, value in [('initial_bot_count', 3), ('investigator_slots', 4),
                           ('arena_companions', ['Sapper']), ('controlled_investigator', 'Unknown')]:
            events = arena_fixture()
            events[0]['data'][key] = value
            with self.subTest(key=key), self.assertRaises(ValueError):
                self.validate(events)

    def test_rejects_invalid_placement_and_lifecycle(self):
        base = arena_fixture()
        for key, value in [('type', 'Shub'), ('x', float('nan')), ('y', 3000), ('batch', -1)]:
            events = copy.deepcopy(base)
            events[2]['data']['placements'][0][key] = value
            with self.subTest(key=key), self.assertRaises(ValueError):
                self.validate(events)
        for index, key, value in [(2, 'phase', 'Fighting'), (3, 'placements', []), (4, 'phase', 'Paused'), (4, 'tick', -1)]:
            events = copy.deepcopy(base)
            events[index]['data'][key] = value
            with self.subTest(index=index, key=key), self.assertRaises(ValueError):
                self.validate(events)

    def test_incomplete_setup_cannot_claim_victory(self):
        events = arena_fixture()
        events[4]['data'].update(action='reset', phase='Fighting')
        with self.assertRaises(ValueError):
            self.validate(events)
        events[-1]['data']['outcome'] = 'aborted'
        self.assertEqual(self.validate(events)['outcome'], 'aborted')
