"""Copy Unreal packages and their dependency closure from a read-only source project into Content/.

Usage:
  python tools/migrate_presentation.py --section kits /Game/Pack/Path/NS_Effect [...]
  python tools/migrate_presentation.py --section kits --from-manifest   # re-copy the section's requested list

Dependencies are found by scanning each .uasset for /Game/... package references (ASCII and UTF-16 name
tables), so soft references and inherited Niagara parents are followed when the package exists. Files are
copied to the same relative path so package names are unchanged; byte-identical files are skipped and a
differing existing file is refused unless --force. The section of design/art/presentation/migration.json is
rewritten with the requested list, every copied file's SHA-256 and size, and references that were not found
in the source project (stale editor-only links are common and harmless when the asset loads; verify with
tools/verify_presentation_assets.py).
"""
import argparse, hashlib, json, pathlib, re, shutil, sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "design/art/presentation/migration.json"
ASCII = re.compile(rb"/Game/[A-Za-z0-9_\-./]+")
UTF16 = re.compile(rb"(?:/\x00G\x00a\x00m\x00e\x00/\x00)(?:[A-Za-z0-9_\-./]\x00)+")
DEFAULT_SKIP = ("/Game/Developers/",)


def sha256(path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def package_refs(path, skip):
    data = path.read_bytes()
    found = set()
    for m in ASCII.finditer(data):
        found.add(m.group(0).decode("ascii"))
    for m in UTF16.finditer(data):
        found.add(m.group(0).decode("utf-16-le"))
    out = set()
    for ref in found:
        ref = ref.rstrip(".")
        last = ref.rsplit("/", 1)[-1]
        if "." in last:
            ref = ref[: len(ref) - len(last)] + last.split(".", 1)[0]
        if ref.startswith(skip):
            continue
        out.add(ref)
    return out


def build_index(content):
    index = {}
    for p in content.rglob("*.uasset"):
        index["/Game/" + p.relative_to(content).with_suffix("").as_posix()] = p
    return index


def closure(requested, index, skip):
    seen, todo, missing = set(), list(requested), set()
    while todo:
        cur = todo.pop()
        if cur in seen:
            continue
        seen.add(cur)
        source = index.get(cur)
        if not source:
            missing.add(cur)
            continue
        for ref in package_refs(source, skip):
            if ref not in seen:
                todo.append(ref)
    return sorted(s for s in seen if s in index), sorted(missing)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("packages", nargs="*", help="/Game/... package paths to migrate")
    parser.add_argument("--source", default=r"C:\Users\jkeyw\Documents\Unreal Projects\HoldingProject", help="source project directory")
    parser.add_argument("--section", required=True, help="manifest section name, e.g. kits")
    parser.add_argument("--from-manifest", action="store_true", help="use the section's existing requested list")
    parser.add_argument("--skip", action="append", default=[], help="extra /Game/ prefixes to ignore")
    parser.add_argument("--force", action="store_true", help="overwrite differing files already in Content/")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    source = pathlib.Path(args.source)
    content = source / "Content"
    if not content.is_dir():
        sys.exit(f"no Content directory under {source}")
    manifest = json.loads(MANIFEST.read_text()) if MANIFEST.exists() else {}
    section = manifest.get(args.section, {})
    requested = list(section.get("requested", [])) if args.from_manifest else list(args.packages)
    if not requested:
        sys.exit("nothing requested")
    skip = tuple(DEFAULT_SKIP) + tuple(args.skip)

    index = build_index(content)
    absent = [r for r in requested if r not in index]
    if absent:
        sys.exit("not in source project: " + ", ".join(absent))
    packages, missing = closure(requested, index, skip)

    copied, identical, refused, total = [], 0, [], 0
    for package in packages:
        src = index[package]
        dst = ROOT / "Content" / src.relative_to(content)
        digest = sha256(src)
        size = src.stat().st_size
        total += size
        if dst.exists():
            if sha256(dst) == digest:
                identical += 1
            elif not args.force:
                refused.append(package)
                continue
        if not args.dry_run and not (dst.exists() and sha256(dst) == digest):
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src, dst)
        copied.append({"path": package, "sha256": digest, "bytes": size})

    if refused:
        sys.exit("existing files differ (use --force): " + ", ".join(refused))
    print(f"{len(packages)} packages, {total / 1e6:.1f} MB, {identical} already identical, {len(missing)} unresolved references")
    for m in missing:
        print("  unresolved:", m)
    if args.dry_run:
        return
    manifest[args.section] = {
        "source_project": source.name,
        "requested": requested,
        "files": copied,
        "unresolved_references": missing,
    }
    MANIFEST.write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"manifest section '{args.section}' written to {MANIFEST.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
