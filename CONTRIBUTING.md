# Contributing

For bug reports, include OBS/plugin versions, Windows architecture, reproduction
steps, expected behavior and screenshots if useful. Redact private log/scene data.

For code changes:

1. Follow [BUILDING.md](BUILDING.md) to prepare the toolchain.
2. Keep audio callbacks free of Qt UI access, allocation and blocking locks.
3. Keep layout-only actions separate from audio/routing changes.
4. Preserve backward compatibility of scene-collection layout data.
5. Run `python tools/build-local.py --smoke` and inspect both layout captures.
6. Update the changelog and documentation when behavior changes.

Do not commit `.deps`, build caches, test output or generated release packages.
Changes are distributed under the repository's GPL v2-or-later license.
