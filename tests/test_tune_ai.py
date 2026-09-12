"""Checks that the tuning harness describes the AI the game actually ships; no Unreal install needed."""
import re
import sys
from pathlib import Path
import unittest

ROOT = Path(__file__).parents[1]
# Imported by path rather than loaded from a spec: tune_ai uses dataclasses, which resolve field types through
# sys.modules and fail for a module that was executed without being registered there.
sys.path.insert(0, str(ROOT / 'tools'))
import tune_ai  # noqa: E402

CURVES = {'Linear', 'Quadratic', 'InverseQuadratic', 'Logistic', 'Step', 'Bell'}
EMPLACE = re.compile(
    r'W\.FocusTerms\.Emplace\(\s*EDMAIInput::(\w+)\s*,\s*'
    r'FDMAICurveSpec\(\s*EDMAICurve::(\w+)\s*,\s*([-\d.f]+)\s*,\s*([-\d.f]+)\s*'
    r'(?:,\s*([-\d.f]+)\s*)?(?:,\s*([-\d.f]+)\s*)?\)\s*,\s*([-\d.f]+)\s*\)')


def number(text, default):
    return default if text is None else float(text.rstrip('f'))


def shipped_terms():
    """The focus terms DMUtilityAI::DefaultWeights builds, in order."""
    source = (ROOT / 'Source' / 'DreadMeridian' / 'Private' / 'DMUtilityAI.cpp').read_text(encoding='utf-8')
    terms = []
    for match in EMPLACE.finditer(source):
        curve = match.group(2)
        assert curve in CURVES, curve
        terms.append({
            'Input': match.group(1), 'Curve': curve,
            'Min': number(match.group(3), 0), 'Max': number(match.group(4), 1),
            # FDMAICurveSpec's own defaults, which the C++ relies on when it passes fewer arguments.
            'Exponent': number(match.group(5), 2.0), 'Midpoint': number(match.group(6), 0.5),
            'Weight': number(match.group(7), 0),
        })
    return terms


class FocusTermTemplate(unittest.TestCase):
    """tune_ai rewrites FocusTerm.<i>.Weight knobs into a whole FocusTerms array, replacing what the profile
    holds. If its template drifts from the shipped terms, a campaign silently tunes a different curve from the
    one that ships and bakes the result back as though it were the same thing."""

    def test_source_is_parseable(self):
        self.assertTrue(shipped_terms(), 'no FocusTerms found in DMUtilityAI.cpp; the regex has gone stale')

    def test_template_matches_shipped_defaults(self):
        shipped = shipped_terms()
        template = tune_ai.FOCUS_TERMS
        self.assertEqual(len(template), len(shipped), 'tune_ai.FOCUS_TERMS has a different number of terms')
        for index, (mine, theirs) in enumerate(zip(template, shipped)):
            self.assertEqual(mine['Input'], theirs['Input'], f'term {index} input')
            for field in ('Curve', 'Min', 'Max', 'Exponent', 'Midpoint'):
                self.assertEqual(mine['Curve'][field], theirs[field], f'term {index} curve {field}')
            self.assertEqual(mine['Weight'], theirs['Weight'], f'term {index} default weight')

    def test_every_focus_knob_addresses_a_real_term(self):
        for profile, knobs in tune_ai.KNOBS.items():
            for knob in knobs:
                if not knob.path.startswith('FocusTerm.'):
                    continue
                index = int(knob.path.split('.')[1])
                self.assertLess(index, len(tune_ai.FOCUS_TERMS), f'{profile}: {knob.path} is out of range')
                self.assertEqual(knob.path.split('.')[2], 'Weight', f'{profile}: only weights are tunable')


class FocusTermExpansion(unittest.TestCase):
    def test_expansion_produces_the_full_array(self):
        doc = {'Sapper': {'FocusTerm': {'1': {'Weight': 7.5}}, 'PositionStep': 300}}
        out = tune_ai.expand_focus_terms(doc)['Sapper']
        self.assertNotIn('FocusTerm', out, 'the indexed form must not reach the engine')
        self.assertEqual(out['PositionStep'], 300, 'other knobs pass through untouched')
        # The engine replaces the whole array, so every term must be present, not just the mutated one.
        self.assertEqual(len(out['FocusTerms']), len(tune_ai.FOCUS_TERMS))
        self.assertEqual(out['FocusTerms'][1]['Weight'], 7.5)
        self.assertEqual(out['FocusTerms'][0]['Weight'], tune_ai.FOCUS_TERMS[0]['Weight'])
        self.assertEqual(out['FocusTerms'][2]['Input'], tune_ai.FOCUS_TERMS[2]['Input'])

    def test_expansion_does_not_mutate_the_template(self):
        before = tune_ai.FOCUS_TERMS[0]['Weight']
        tune_ai.expand_focus_terms({'Sapper': {'FocusTerm': {'0': {'Weight': 1.0}}}})
        self.assertEqual(tune_ai.FOCUS_TERMS[0]['Weight'], before)

    def test_profiles_without_focus_knobs_are_untouched(self):
        doc = {'Gunman': {'LeashRange': 1500}}
        self.assertEqual(tune_ai.expand_focus_terms(doc), doc)


if __name__ == '__main__':
    unittest.main()
