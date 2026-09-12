#!/usr/bin/env python3
"""Headless utility-AI tuning harness for Dread Meridian (stdlib only, Python 3.12+).

Alternates phases between the enemy profiles (even phases: Gunman, Bruiser, Lookout, Bomber, GangBoss) and the
investigator bot profiles (odd phases: Sapper, Photographer, Medium, Smuggler). Each phase is a small (1+lambda)
search over the KNOBS table below: every generation evaluates the incumbent (generation 0 only) plus
`--candidates - 1` mutations, each on every seed, through UnrealEditor-Cmd in smuggler-soak mode. A run is scored
from the DREAD_AI_RESULT line ADMCombatGameMode logs at completion (victory/defeat) or at the 1800-tick soak
timeout. Overrides use the -DMAIWeights JSON shape documented in Source/DreadMeridian/Public/DMAIProfile.h.

Outputs under --out: runs.csv (one row per run), best.json (current best overrides, rewritten after every
generation), state.json (last completed phase, used by --resume), candidates/ and logs/ per run, report.md.
Sandbox tuning only: nothing here is approved balance.
"""
from __future__ import annotations

import argparse
import concurrent.futures
import copy
import csv
import datetime as dt
import json
import math
import random
import re
import subprocess
import sys
import threading
import time
from dataclasses import dataclass, replace
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
MAP = "/Game/DreadMeridian/Maps/L_CombatSandbox"
ENEMY_PROFILES = ["Gunman", "Bruiser", "Lookout", "Bomber", "GangBoss"]
BOT_PROFILES = ["Sapper", "Photographer", "Medium", "Smuggler"]
RESULT_RE = re.compile(r"DREAD_AI_RESULT (\{.*\})")
METRICS = ["tick", "investigators_standing", "enemies_standing", "enemy_total", "investigator_total",
           "damage_to_investigators", "damage_to_enemies", "investigator_downs", "enemy_kills", "revives",
           "signatures", "q_casts", "w_casts", "e_casts", "r_casts", "pings"]
RUN_TIMEOUT_SECONDS = 900
MUTATION_SIGMA = 0.15  # default for --sigma: gaussian step as a fraction of the knob range
STALE_GENERATIONS = 2  # default for --stale: stop a phase after this many generations without improvement


@dataclass(frozen=True)
class Knob:
    path: str        # dotted path inside a profile section, e.g. Actions.Engage.Weight
    lo: float
    hi: float
    default: float   # assumed C++ default (DMUtilityAI::DefaultWeights); the incumbent starts with no overrides, so the true baseline is measured regardless
    integer: bool = False


def weight(action: str, lo: float = 0.2, hi: float = 2.0, default: float = 1.0) -> Knob:
    return Knob(f"Actions.{action}.Weight", lo, hi, default)


def signature(base_default: float, base_hi: float) -> list[Knob]:
    return [weight("Signature"), Knob("Abilities.Signature.Base", 1, base_hi, base_default), Knob("KCooldown", 0, 3, 1)]


def base_q(action: str, base_default: float, base_hi: float) -> list[Knob]:
    return [weight(action), Knob(f"Abilities.{action}.Base", 1, base_hi, base_default)]


# Knob selection principle: with rank ordering and one option per channel, an action's Weight only matters when it
# competes inside the same rank and channel, so enemy Engage/Signature/Strafe and companion BasicAttack weights never
# change a decision and are not tuned. What steers behaviour is the latch thresholds, runtime/cooldown caps, ranges,
# worth multipliers (which gate casts through the conservation threshold) and, for companions, ping compliance.
RANGED = [Knob("KeepDistanceEnter", 0.15, 0.8, 0.5), Knob("KeepDistanceExit", 0.4, 1.0, 0.7),
          Knob("KeepDistanceStep", 100, 600, 300), Knob("Actions.KeepDistance.MaxRuntimeTicks", 5, 80, 40, True),
          Knob("Actions.KeepDistance.DecisionCooldownTicks", 0, 60, 30, True)]
CAMP = [Knob("LeashRange", 1200, 2600, 1800), Knob("SightRange", 400, 900, 600), Knob("TargetCommitment", 0, 300, 75),
        Knob("MarkedWorth", 0, 1, 0.25), Knob("EliteWorth", 0, 1, 0.5)]
COMMANDER = [Knob("AllyRadius", 500, 1400, 900)]
# Where to stand and which target to pick: the knobs added by the 2026-09-12 tactics work, all hand-picked.
# Every one of them changes which option wins or which target is chosen, which is the bar for a slot here.
POSITION = [Knob("PositionStep", 150, 600, 320), Knob("PositionDangerRadius", 400, 1100, 750),
            Knob("PositionDangerWeight", 0, 3, 1.4), Knob("PositionHazardWeight", 1, 8, 4),
            Knob("PositionRangeWeight", 0, 3, 1.6), Knob("PositionAllyWeight", 0, 2, 0.45),
            Knob("PositionClumpRadius", 60, 400, 170), Knob("PositionClumpWeight", 0, 2, 0.8),
            Knob("PositionGroundRadius", 150, 800, 420), Knob("PositionGroundWeight", 0, 2, 0.7),
            Knob("PositionTravelWeight", 0, 2, 0.55),
            Knob("Actions.Reposition.Weight", 0.1, 1.0, 0.6),
            Knob("Actions.Reposition.MaxRuntimeTicks", 5, 80, 25, True),
            Knob("Actions.Reposition.DecisionCooldownTicks", 0, 80, 35, True)]
# Indices match FOCUS_TERMS: finish the wounded, peel for an ally in peril, join a pair, press the suppressed.
FOCUS = [Knob("FocusTerm.0.Weight", 0, 600, 260), Knob("FocusTerm.1.Weight", 0, 700, 320),
         Knob("FocusTerm.2.Weight", 0, 400, 150), Knob("FocusTerm.3.Weight", 0, 400, 110)]

COMPANION = [Knob("FleeEnter", 0.05, 0.5, 0.25), Knob("FleeExit", 0.3, 0.8, 0.4), Knob("FleeEnemyRadius", 200, 800, 400),
             Knob("FleeSafeRadius", 300, 1200, 600), Knob("FleeDistance", 150, 700, 400), Knob("EvadeMargin", 20, 200, 100),
             weight("Flee"), weight("EvadeHazard"), Knob("KCooldown", 0, 3, 1), Knob("TargetCommitment", 0, 300, 75),
             Knob("MarkedWorth", 0, 1, 0.25), Knob("EliteWorth", 0, 1, 0.5), Knob("PingCompliance", 0, 1, 1),
             Knob("PingEnemyScore", 0, 800, 250)]
# Baked into DMUtilityAI::DefaultWeights from Saved/AITuning/final/best.json (2026-09-11). Keep in step with
# ApplyTunedDefaults in Source/DreadMeridian/Private/DMUtilityAI.cpp so the harness's assumed defaults match the C++ ones.
BAKED_DEFAULTS: dict[str, dict[str, float]] = {
    "Gunman": {"Actions.KeepDistance.MaxRuntimeTicks": 27, "SightRange": 612.3128},
    "Bruiser": {"LeashRange": 1492.3444},
    "Lookout": {"AllyRadius": 1011.3852, "EliteWorth": 0.9737, "TargetCommitment": 13.7513},
    "Bomber": {"TargetCommitment": 52.5924},
    "Sapper": {"Actions.EvadeHazard.Weight": 1.0801, "Actions.SeekPickup.Weight": 0.9233, "EliteWorth": 0.5328,
               "FleeDistance": 185.6959, "FleeEnter": 0.1843, "KCooldown": 0.7768, "KStock": 2.7864, "MarkedWorth": 0.36,
               "PingEnemyScore": 291.5309, "TargetCommitment": 46.0947},
    "Photographer": {"Abilities.Frame.Base": 28.6499, "Actions.EvadeHazard.Weight": 1.6028, "Actions.Flee.Weight": 1.5853,
                     "EliteWorth": 0.5724, "EvadeMargin": 63.5246, "FleeEnemyRadius": 478.3227, "FleeEnter": 0.2556,
                     "FleeExit": 0.3522, "KCooldown": 1.6185, "TargetCommitment": 4.8771},
    "Medium": {"Abilities.BindSpirit.Base": 14.5827, "Actions.EvadeHazard.Weight": 0.6048, "Actions.Flee.Weight": 0.8656,
               "EvadeMargin": 131.5508, "FleeEnter": 0.1375, "KCooldown": 1.1811, "MarkedWorth": 0.2811,
               "PingCompliance": 0.8623, "ThreatenedAllyHealth": 51.7847},
    "Smuggler": {"Abilities.Clinch.Base": 17.307, "Actions.Flee.Weight": 0.835, "EliteWorth": 0.6516, "FleeExit": 0.3853,
                 "KCooldown": 0.8498, "MarkedWorth": 0.4809, "PingCompliance": 1, "PingEnemyScore": 347.1017},
}


def with_baked_defaults(profile: str, knobs: list[Knob]) -> list[Knob]:
    baked = BAKED_DEFAULTS.get(profile, {})
    missing = set(baked) - {knob.path for knob in knobs}
    if missing:
        raise ValueError(f"BAKED_DEFAULTS[{profile}] names knobs that are not tuned: {sorted(missing)}")
    return [replace(knob, default=baked.get(knob.path, knob.default)) for knob in knobs]


HAND_KNOBS: dict[str, list[Knob]] = {
    "Gunman": RANGED + CAMP,
    "Bruiser": CAMP + [Knob("Abilities.Signature.Base", 1, 30, 5), Knob("KCooldown", 0, 3, 1)],
    "Lookout": RANGED + CAMP + COMMANDER + [Knob("Abilities.Signature.Base", 1, 20, 5), Knob("KCooldown", 0, 3, 1)],
    "Bomber": RANGED + CAMP + [Knob("Abilities.Signature.Base", 1, 30, 5), Knob("KCooldown", 0, 3, 1)],
    "GangBoss": COMMANDER + [Knob("Abilities.Signature.Base", 1, 20, 5), Knob("KCooldown", 0, 3, 1),
                 Knob("StrafeRadius", 300, 900, 650), Knob("StrafeOffset", 200, 700, 460), Knob("StrafeAngle", 10, 80, 35),
                 Knob("TargetCommitment", 0, 300, 75)],
    # The kit Bases are the cast-or-hold boundary for each named ability; DeadGround also exposes the cap that
    # keeps a 600-tick ultimate reachable at all, since that interacts with the shared KCooldown above.
    "Sapper": COMPANION + POSITION + FOCUS + [weight("SeekPickup", 0.2, 2.0, 0.9), Knob("PickupRadius", 300, 1200, 700),
               Knob("Abilities.PlaceSatchel.Base", 1, 80, 23), Knob("KStock", 0, 4, 2),
               Knob("Abilities.SuppressingFire.Base", 1, 60, 10), Knob("Abilities.Tripwire.Base", 1, 60, 16),
               Knob("Abilities.DeadGround.Base", 1, 120, 32),
               Knob("Abilities.DeadGround.ThresholdCooldownCap", 50, 600, 150, True)],
    # Develop.Radius is the Exposure a bot waits for before spending a subject, which is the real decision here.
    "Photographer": COMPANION + POSITION + FOCUS + [Knob("Abilities.Frame.Base", 1, 60, 20),
                     Knob("Abilities.Flashbulb.Base", 1, 40, 6), Knob("Abilities.Develop.Base", 1, 40, 10),
                     Knob("Abilities.Develop.Radius", 10, 90, 40),
                     Knob("Abilities.ImpossiblePhotograph.Base", 1, 60, 11.7),
                     Knob("Abilities.ImpossiblePhotograph.ThresholdCooldownCap", 50, 600, 150, True)],
    "Medium": COMPANION + POSITION + FOCUS + [Knob("Abilities.BindSpirit.Base", 1, 40, 10), Knob("ThreatenedAllyHealth", 20, 90, 50),
               Knob("Abilities.Beckon.Base", 1, 40, 8), Knob("Abilities.Intercession.Base", 1, 40, 10),
               Knob("Abilities.OpenSeance.Base", 1, 90, 27),
               Knob("Abilities.OpenSeance.ThresholdCooldownCap", 50, 600, 150, True)],
    "Smuggler": COMPANION + POSITION + FOCUS + [Knob("Abilities.Clinch.Base", 1, 40, 10),
                 Knob("Abilities.ShoulderThrough.Base", 1, 40, 6), Knob("Abilities.DigIn.Base", 1, 30, 4),
                 Knob("Abilities.DrownedMan.Base", 1, 90, 26.4),
                 Knob("Abilities.DrownedMan.ThresholdCooldownCap", 50, 600, 150, True)],
}
KNOBS: dict[str, list[Knob]] = {profile: with_baked_defaults(profile, knobs) for profile, knobs in HAND_KNOBS.items()}
LATCHES = [("FleeEnter", "FleeExit"), ("KeepDistanceEnter", "KeepDistanceExit")]  # enter must stay below exit


# ---------------------------------------------------------------------------------------------- override docs

def knob_for(profile: str, path: str) -> Knob | None:
    return next((k for k in KNOBS.get(profile, []) if k.path == path), None)


# Focus-term weights live in an array, and a knob path builds nested dicts, so the search addresses them as
# FocusTerm.<index>.Weight and expand_focus_terms() rewrites those into the array the profile loader expects.
# Inputs and curve shapes are fixed by design (see DMUtilityAI::DefaultWeights); only the weights are tuned.
# Keep this list in step with the FocusTerms built there, or a candidate will quietly tune a different curve.
FOCUS_TERMS = [
    {"Input": "TargetHealthFrac", "Curve": {"Curve": "InverseQuadratic", "Min": 0, "Max": 1, "Exponent": 2, "Midpoint": 0.5, "bInvert": False}, "Weight": 260.0},
    {"Input": "AllyInPeril", "Curve": {"Curve": "Linear", "Min": 0, "Max": 1, "Exponent": 2, "Midpoint": 0.5, "bInvert": False}, "Weight": 320.0},
    {"Input": "AlliesOnTarget", "Curve": {"Curve": "Bell", "Min": 0, "Max": 3, "Exponent": 3.2, "Midpoint": 0.4, "bInvert": False}, "Weight": 150.0},
    {"Input": "TargetSuppressed", "Curve": {"Curve": "Step", "Min": 0, "Max": 1, "Exponent": 2, "Midpoint": 0.5, "bInvert": False}, "Weight": 110.0},
]


def expand_focus_terms(doc: dict) -> dict:
    """Rewrite each profile's FocusTerm.<i>.Weight scalars as the full FocusTerms array the engine reads."""
    out = {}
    for profile, values in doc.items():
        if not isinstance(values, dict) or "FocusTerm" not in values:
            out[profile] = values
            continue
        values = dict(values)
        indexed = values.pop("FocusTerm")
        terms = [dict(term, Curve=dict(term["Curve"])) for term in FOCUS_TERMS]
        for key, fields in (indexed or {}).items():
            index = int(key)
            if 0 <= index < len(terms) and isinstance(fields, dict) and "Weight" in fields:
                terms[index]["Weight"] = fields["Weight"]
        values["FocusTerms"] = terms
        out[profile] = values
    return out


def get_value(doc: dict, profile: str, path: str, default: float) -> float:
    node = doc.get(profile, {})
    for part in path.split("."):
        if not isinstance(node, dict) or part not in node:
            return default
        node = node[part]
    return node if isinstance(node, (int, float)) and not isinstance(node, bool) else default


def set_value(doc: dict, profile: str, path: str, value: float) -> None:
    node = doc.setdefault(profile, {})
    parts = path.split(".")
    for part in parts[:-1]:
        node = node.setdefault(part, {})
    node[parts[-1]] = value


def flatten(doc: dict) -> dict[str, float]:
    out: dict[str, float] = {}

    def walk(prefix: str, node) -> None:
        if isinstance(node, dict):
            for key, value in node.items():
                walk(f"{prefix}.{key}" if prefix else key, value)
        else:
            out[prefix] = node

    walk("", doc)
    return out


def repair_latches(doc: dict, profile: str) -> None:
    for enter_path, exit_path in LATCHES:
        enter, exit_ = knob_for(profile, enter_path), knob_for(profile, exit_path)
        if not enter or not exit_:
            continue
        enter_value = get_value(doc, profile, enter_path, enter.default)
        exit_value = get_value(doc, profile, exit_path, exit_.default)
        if enter_value >= exit_value - 0.05:
            set_value(doc, profile, enter_path, round(max(enter.lo, exit_value - 0.05), 4))


def mutate(doc: dict, profiles: list[str], rng: random.Random, sigma: float = MUTATION_SIGMA) -> tuple[dict, list[tuple[str, str, float, float]]]:
    child = copy.deepcopy(doc)
    pool = [(profile, knob) for profile in profiles for knob in KNOBS[profile]]
    changes = []
    for profile, knob in rng.sample(pool, k=min(len(pool), rng.randint(2, 4))):
        old = get_value(child, profile, knob.path, knob.default)
        new = min(knob.hi, max(knob.lo, old + rng.gauss(0, sigma * (knob.hi - knob.lo))))
        new = int(round(new)) if knob.integer else round(new, 4)
        set_value(child, profile, knob.path, new)
        changes.append((profile, knob.path, old, new))
    for profile in profiles:
        repair_latches(child, profile)
    return child, changes


# ---------------------------------------------------------------------------------------------- fitness

def enemy_fitness(r: dict) -> float:
    return (r["damage_to_investigators"] + 150 * r["investigator_downs"] + (2000 if r["outcome"] == "defeat" else 0)
            - 0.5 * r["damage_to_enemies"] - 0.2 * r["tick"])


def bot_fitness(r: dict) -> float:
    return (r["damage_to_enemies"] + 300 * r["enemy_kills"] + (3000 if r["outcome"] == "victory" else 0)
            - r["damage_to_investigators"] - 200 * r["investigator_downs"] - 0.5 * r["tick"])


PHASES = {  # phase parity -> (team, profiles, fitness)
    0: ("enemy", ENEMY_PROFILES, enemy_fitness),
    1: ("bot", BOT_PROFILES, bot_fitness),
}


# ---------------------------------------------------------------------------------------------- runs

def read_log(path: Path) -> str:
    data = path.read_bytes()
    if data.startswith(b"\xff\xfe") or data.startswith(b"\xfe\xff"):
        return data.decode("utf-16", errors="replace")
    return data.decode("utf-8-sig", errors="replace")


def parse_result(path: Path) -> dict | None:
    if not path.exists():
        return None
    matches = RESULT_RE.findall(read_log(path))
    if not matches:
        return None
    try:
        parsed = json.loads(matches[-1])
    except json.JSONDecodeError:
        return None
    return parsed if isinstance(parsed, dict) and "outcome" in parsed else None


def kill_tree(proc: subprocess.Popen) -> None:
    if sys.platform == "win32":
        subprocess.run(["taskkill", "/T", "/F", "/PID", str(proc.pid)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
    try:
        proc.kill()
    except OSError:
        pass
    try:
        proc.wait(timeout=30)
    except subprocess.TimeoutExpired:
        pass


@dataclass
class Job:
    phase: int
    team: str
    generation: int
    candidate: int
    seed: int
    candidate_path: Path
    log_path: Path


class Runner:
    def __init__(self, args: argparse.Namespace, fitness_fn):
        self.args = args
        self.fitness_fn = fitness_fn
        self.lock = threading.Lock()
        self.active: set[subprocess.Popen] = set()
        self.csv_path: Path = args.out / "runs.csv"

    def command(self, seed: int, candidate_path: Path, log_path: Path) -> list[str]:
        return [str(self.args.editor), str(self.args.project), MAP, "-server", "-unattended", "-nop4", "-nosplash",
                "-nullrhi", "-nosound", "-deterministic", "-fps=60", "-DMSmugglerSoak", f"-DMSeed={seed}",
                f"-DMAIWeights={candidate_path}", f"-abslog={log_path}"] + list(self.args.extra_arg)

    def run(self, job: Job) -> dict:
        cmd = self.command(job.seed, job.candidate_path, job.log_path)
        job.log_path.parent.mkdir(parents=True, exist_ok=True)
        if job.log_path.exists():
            job.log_path.unlink()
        start = time.monotonic()
        proc = subprocess.Popen(cmd, cwd=str(REPO), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        with self.lock:
            self.active.add(proc)
        timed_out = False
        try:
            code = proc.wait(timeout=RUN_TIMEOUT_SECONDS)
        except subprocess.TimeoutExpired:
            timed_out = True
            kill_tree(proc)
            code = None
        finally:
            with self.lock:
                self.active.discard(proc)
        seconds = time.monotonic() - start
        result = parse_result(job.log_path)
        record = {"phase": job.phase, "team": job.team, "generation": job.generation, "candidate": job.candidate,
                  "seed": job.seed, "exit_code": "" if code is None else code, "seconds": round(seconds, 1),
                  "log": str(job.log_path), "valid": result is not None}
        if result is None:
            # No DREAD_AI_RESULT: process timeout or crash. Zero metrics; the candidate is disqualified in selection.
            record["outcome"] = "process_timeout" if timed_out else "no_result"
            for key in METRICS:
                record[key] = 0
        else:
            record["outcome"] = str(result.get("outcome", "unknown"))
            for key in METRICS:
                record[key] = result.get(key, 0)
        record["fitness"] = round(self.fitness_fn(record), 2)
        self.append_csv(record)
        with self.lock:
            print(f"  [p{job.phase} {job.team} g{job.generation}] cand {job.candidate} seed {job.seed} -> {record['outcome']}"
                  f" tick={record['tick']} dmg_inv={record['damage_to_investigators']:.0f} dmg_en={record['damage_to_enemies']:.0f}"
                  f" downs={record['investigator_downs']} kills={record['enemy_kills']} fit={record['fitness']:.1f} ({seconds:.0f} s)", flush=True)
        return record

    def append_csv(self, record: dict) -> None:
        columns = ["phase", "team", "generation", "candidate", "seed", "outcome", "exit_code", "seconds"] + METRICS + ["fitness", "log"]
        with self.lock:
            new = not self.csv_path.exists()
            with self.csv_path.open("a", newline="", encoding="utf-8") as handle:
                writer = csv.DictWriter(handle, fieldnames=columns, extrasaction="ignore")
                if new:
                    writer.writeheader()
                writer.writerow(record)

    def kill_all(self) -> None:
        with self.lock:
            procs = list(self.active)
        for proc in procs:
            kill_tree(proc)


def evaluate(runner: Runner, phase: int, team: str, generation: int, candidates: list[tuple[int, dict]], seeds: list[int]) -> dict[int, float]:
    jobs = []
    for index, doc in candidates:
        candidate_path = runner.args.out / "candidates" / f"p{phase}_g{generation}_c{index}.json"
        candidate_path.parent.mkdir(parents=True, exist_ok=True)
        candidate_path.write_text(json.dumps(expand_focus_terms(doc), indent=2, sort_keys=True), encoding="utf-8")
        for seed in seeds:
            log_path = runner.args.out / "logs" / f"p{phase}_g{generation}_c{index}_s{seed}.log"
            jobs.append(Job(phase, team, generation, index, seed, candidate_path, log_path))
    records: dict[int, list[dict]] = {}
    try:
        with concurrent.futures.ThreadPoolExecutor(max_workers=runner.args.parallel) as pool:
            for record in pool.map(runner.run, jobs):
                records.setdefault(record["candidate"], []).append(record)
    except KeyboardInterrupt:
        runner.kill_all()
        raise
    fitness: dict[int, float] = {}
    for index, rows in records.items():
        fitness[index] = sum(r["fitness"] for r in rows) / len(rows) if all(r["valid"] for r in rows) else -math.inf
    return fitness


# ---------------------------------------------------------------------------------------------- reporting

def write_json(path: Path, doc) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(doc, indent=2, sort_keys=True), encoding="utf-8")


def fmt(value: float) -> str:
    return "-inf" if value == -math.inf else f"{value:.1f}"


def write_report(args: argparse.Namespace, phases: list[dict], incumbent: dict, finished: bool) -> None:
    lines = ["# AI tuning report", "",
             f"Generated {dt.datetime.now().isoformat(timespec='seconds')} ({'complete' if finished else 'partial'}).",
             f"Project `{args.project}`, seeds {args.seeds}, validation seeds {args.validation_seeds or 'none'}, min gain {args.min_gain}, sigma {args.sigma}, {args.candidates} candidates x {args.generations} generations per phase, parallel {args.parallel}.",
             "Fitness is the seed average of the tuned team's objective (enemy or bot formula in tune_ai.py). Sandbox tuning only, not approved balance.", ""]
    lines += ["## Phases", "", "| Phase | Team | Generations | Before | After | Delta |", "|---|---|---|---|---|---|"]
    for entry in phases:
        lines.append(f"| {entry['phase']} | {entry['team']} | {entry['generations']} | {fmt(entry['before'])} | {fmt(entry['after'])} | {fmt(entry['after'] - entry['before']) if entry['before'] != -math.inf and entry['after'] != -math.inf else 'n/a'} |")
    for entry in phases:
        lines += ["", f"### Phase {entry['phase']} ({entry['team']})", ""]
        if entry["moved"]:
            lines += ["| Knob | Before | After |", "|---|---|---|"]
            lines += [f"| {key} | {before} | {after} |" for key, before, after in entry["moved"]]
        else:
            lines.append("No knob moved (no candidate beat the incumbent).")
    lines += ["", "## Best overrides", "", "```json", json.dumps(incumbent, indent=2, sort_keys=True), "```", "",
              "## Notes", "", f"- {args.ticks_timeout_note}",
              f"- Every run is listed in `{args.out / 'runs.csv'}`; logs are under `{args.out / 'logs'}`.",
              "- Apply the result with `-DMAIWeights=<out>/best.json` or copy values into the AIP_* data assets.", ""]
    (args.out / "report.md").write_text("\n".join(lines), encoding="utf-8")


def diff_docs(before: dict, after: dict) -> list[tuple[str, str, str]]:
    a, b = flatten(before), flatten(after)
    moved = []
    for key in sorted(set(a) | set(b)):
        if a.get(key) != b.get(key):
            moved.append((key, "default" if key not in a else str(a[key]), "default" if key not in b else str(b[key])))
    return moved


# ---------------------------------------------------------------------------------------------- main

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--engine-root", default="C:/Program Files/Epic Games/UE_5.8")
    parser.add_argument("--project", default=str(REPO / "DreadMeridian.uproject"))
    parser.add_argument("--phases", type=int, default=6)
    parser.add_argument("--generations", type=int, default=4)
    parser.add_argument("--candidates", type=int, default=6, help="incumbent + mutations per generation (minimum 2)")
    parser.add_argument("--seeds", default="1927,1928,1929")
    parser.add_argument("--parallel", type=int, default=3)
    parser.add_argument("--out", default=str(REPO / "Saved" / "AITuning"))
    parser.add_argument("--ticks-timeout-note", default="The in-game DMSmugglerSoak timeout (1800 ticks) reports outcome=timeout with real metrics; "
                        f"a run killed at the {RUN_TIMEOUT_SECONDS} s process timeout is recorded as process_timeout with zero metrics and disqualifies its candidate.",
                        help="free text recorded in report.md about how timeouts are treated")
    parser.add_argument("--resume", action="store_true", help="start from <out>/best.json and continue after the phase recorded in <out>/state.json")
    parser.add_argument("--start-phase", type=int, default=None, help="first phase index to run (overrides --resume's phase)")
    parser.add_argument("--rng-seed", type=int, default=1927, help="seed for the mutation generator (not the game seed)")
    parser.add_argument("--stale", type=int, default=STALE_GENERATIONS, help="stop a phase after this many generations without improvement")
    parser.add_argument("--validation-seeds", default="", help="comma-separated held-back seeds: a generation's best candidate is accepted only if it also matches or beats the incumbent on these")
    parser.add_argument("--sigma", type=float, default=MUTATION_SIGMA, help="mutation step as a fraction of each knob's range")
    parser.add_argument("--min-gain", type=float, default=0.0, help="fraction of |incumbent fitness| a candidate must gain on the training seeds to be accepted (noise margin)")
    parser.add_argument("--extra-arg", action="append", default=[], help="extra UnrealEditor-Cmd argument (repeatable)")
    parser.add_argument("--dry-run", action="store_true", help="print the generation-0 commands of the first phase without launching")
    parser.add_argument("--evaluate", default=None, help="hold-out evaluation: run this overrides JSON (or 'none' for the C++ defaults) on --seeds and report both fitness formulas; no tuning")
    parser.add_argument("--label", default=None, help="name for the --evaluate output file (default: the JSON stem or 'defaults')")
    args = parser.parse_args()
    if args.candidates < 2:
        parser.error("--candidates must be at least 2 (incumbent plus one mutation)")
    if args.phases < 1 or args.generations < 1 or args.parallel < 1:
        parser.error("--phases, --generations and --parallel must be positive")
    args.out = Path(args.out).resolve()
    args.project = Path(args.project).resolve()
    args.editor = Path(args.engine_root) / "Engine" / "Binaries" / "Win64" / "UnrealEditor-Cmd.exe"
    args.seed_list = [int(s) for s in args.seeds.split(",") if s.strip()]
    args.validation_list = [int(s) for s in args.validation_seeds.split(",") if s.strip()]
    if not args.seed_list:
        parser.error("--seeds must list at least one integer")
    return args



def evaluate_holdout(args: argparse.Namespace) -> int:
    """Run one weights document on every seed and report both objectives (no mutation, no selection)."""
    source = args.evaluate
    doc = {} if source.lower() == "none" else json.loads(Path(source).read_text(encoding="utf-8"))
    label = args.label or ("defaults" if source.lower() == "none" else Path(source).stem)
    candidate_path = args.out / "candidates" / f"eval_{label}.json"
    candidate_path.parent.mkdir(parents=True, exist_ok=True)
    candidate_path.write_text(json.dumps(expand_focus_terms(doc), indent=2, sort_keys=True), encoding="utf-8")
    runner = Runner(args, bot_fitness)
    runner.csv_path = args.out / f"eval_{label}.csv"
    jobs = [Job(-1, "eval", 0, 0, seed, candidate_path, args.out / "logs" / f"eval_{label}_s{seed}.log") for seed in args.seed_list]
    records: list[dict] = []
    try:
        with concurrent.futures.ThreadPoolExecutor(max_workers=args.parallel) as pool:
            records = list(pool.map(runner.run, jobs))
    except KeyboardInterrupt:
        runner.kill_all()
        raise
    valid = [r for r in records if r["valid"]]
    summary = {"label": label, "source": source, "seeds": args.seed_list, "runs": len(records), "valid": len(valid),
               "victories": sum(1 for r in valid if r["outcome"] == "victory"),
               "defeats": sum(1 for r in valid if r["outcome"] == "defeat"),
               "timeouts": sum(1 for r in valid if r["outcome"] == "timeout"),
               "bot_fitness": sum(bot_fitness(r) for r in valid) / len(valid) if valid else None,
               "enemy_fitness": sum(enemy_fitness(r) for r in valid) / len(valid) if valid else None}
    for key in METRICS:
        summary["mean_" + key] = sum(float(r[key]) for r in valid) / len(valid) if valid else None
    write_json(args.out / f"eval_{label}.json", summary)
    print(json.dumps(summary, indent=2))
    return 0 if valid and len(valid) == len(records) else 1


def main() -> int:
    args = parse_args()
    if " " in str(args.out):
        print(f"error: --out must not contain spaces ({args.out}); -DMAIWeights=/-abslog= values are parsed unquoted by the engine", file=sys.stderr)
        return 2
    if not args.dry_run and not args.editor.exists():
        print(f"error: editor not found at {args.editor}; pass --engine-root", file=sys.stderr)
        return 2
    if not args.project.exists():
        print(f"error: project not found at {args.project}", file=sys.stderr)
        return 2
    args.out.mkdir(parents=True, exist_ok=True)

    incumbent: dict = {}
    start_phase = 0
    phases_report: list[dict] = []
    if args.resume:
        best_path, state_path = args.out / "best.json", args.out / "state.json"
        if best_path.exists():
            incumbent = json.loads(best_path.read_text(encoding="utf-8"))
            print(f"resumed overrides from {best_path} ({len(flatten(incumbent))} values)")
        if state_path.exists():
            state = json.loads(state_path.read_text(encoding="utf-8"))
            start_phase = int(state.get("phase", -1)) + 1
            phases_report = [dict(entry, moved=[tuple(m) for m in entry.get("moved", [])]) for entry in state.get("phases", [])]
    if args.start_phase is not None:
        start_phase = args.start_phase
    rng = random.Random(args.rng_seed)

    if args.evaluate is not None:
        return evaluate_holdout(args)
    if args.dry_run:
        team, profiles, fitness_fn = PHASES[start_phase % 2]
        runner = Runner(args, fitness_fn)
        candidates = [(0, incumbent)] + [(c, mutate(incumbent, profiles, rng, args.sigma)[0]) for c in range(1, args.candidates)]
        for index, doc in candidates:
            candidate_path = args.out / "candidates" / f"p{start_phase}_g0_c{index}.json"
            candidate_path.parent.mkdir(parents=True, exist_ok=True)
            candidate_path.write_text(json.dumps(expand_focus_terms(doc), indent=2, sort_keys=True), encoding="utf-8")
            for seed in args.seed_list:
                print(subprocess.list2cmdline(runner.command(seed, candidate_path, args.out / "logs" / f"p{start_phase}_g0_c{index}_s{seed}.log")))
        print(f"dry run: {len(candidates) * len(args.seed_list)} commands for phase {start_phase} ({team}); nothing launched")
        return 0

    print(f"tuning {args.phases} phases from phase {start_phase}: seeds {args.seed_list}, {args.candidates} candidates, "
          f"{args.generations} generations, parallel {args.parallel}, out {args.out}", flush=True)
    for phase in range(start_phase, args.phases):
        team, profiles, fitness_fn = PHASES[phase % 2]
        runner = Runner(args, fitness_fn)
        phase_start = copy.deepcopy(incumbent)
        incumbent_fit = -math.inf
        incumbent_val = None
        before = -math.inf
        stale = 0
        generations_run = 0
        print(f"== phase {phase} ({team}: {', '.join(profiles)})", flush=True)
        for generation in range(args.generations):
            candidates: list[tuple[int, dict]] = [(0, incumbent)] if generation == 0 else []
            children: dict[int, dict] = {}
            for index in range(1, args.candidates):
                child, _changes = mutate(incumbent, profiles, rng, args.sigma)
                children[index] = child
                candidates.append((index, child))
            fitness = evaluate(runner, phase, team, generation, candidates, args.seed_list)
            generations_run += 1
            if generation == 0:
                incumbent_fit = fitness.get(0, -math.inf)
                before = incumbent_fit
            best_index = max(children, key=lambda i: fitness.get(i, -math.inf))
            best_fit = fitness.get(best_index, -math.inf)
            margin = args.min_gain * abs(incumbent_fit) if incumbent_fit != -math.inf else 0.0
            accepted = best_fit > incumbent_fit + margin
            note = ""
            if accepted and args.validation_list:
                # Validation gate: the winner must also match or beat the incumbent on seeds it was not tuned on.
                if incumbent_val is None:
                    incumbent_val = evaluate(runner, phase, team + "-val", generation, [(0, incumbent)], args.validation_list).get(0, -math.inf)
                candidate_val = evaluate(runner, phase, team + "-val", generation, [(best_index, children[best_index])], args.validation_list).get(best_index, -math.inf)
                note = f"; validation {fmt(candidate_val)} vs {fmt(incumbent_val)}"
                if candidate_val < incumbent_val:
                    accepted = False
                else:
                    incumbent_val = candidate_val
            if accepted:
                incumbent = children[best_index]
                incumbent_fit = best_fit
                stale = 0
                verdict = f"improved to {fmt(best_fit)} (candidate {best_index}){note}"
            else:
                stale += 1
                verdict = f"no improvement (best candidate {fmt(best_fit)} vs incumbent {fmt(incumbent_fit)}{note})"
            write_json(args.out / "best.json", incumbent)
            print(f"-- phase {phase} generation {generation}: {verdict}", flush=True)
            if stale >= args.stale and generation < args.generations - 1:
                print(f"-- phase {phase}: stopping early after {args.stale} generations without improvement", flush=True)
                break
        entry = {"phase": phase, "team": team, "generations": generations_run, "before": before, "after": incumbent_fit,
                 "moved": diff_docs(phase_start, incumbent)}
        phases_report.append(entry)
        write_json(args.out / "state.json", {"phase": phase, "team": team, "fitness": None if incumbent_fit == -math.inf else incumbent_fit,
                                             "finished": dt.datetime.now().isoformat(timespec="seconds"), "phases": phases_report})
        write_report(args, phases_report, incumbent, finished=False)
        print(f"== phase {phase} done: {fmt(before)} -> {fmt(incumbent_fit)}, {len(entry['moved'])} knob(s) moved", flush=True)
    write_report(args, phases_report, incumbent, finished=True)
    print(f"report: {args.out / 'report.md'}; best overrides: {args.out / 'best.json'}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("interrupted", file=sys.stderr)
        sys.exit(130)
