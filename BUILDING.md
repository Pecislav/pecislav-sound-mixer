# Building on Windows x64

## Requirements

- Windows x64 and Python 3.11 or newer, with pip.
- A 64-bit OBS Studio 33.0.2 installation (the official portable ZIP also works).
- Internet access and several GB of free space for development dependencies.

Run commands from the repository root. Paths are derived from script locations;
the project can be stored outside its original development folder.
After moving a previously configured checkout, add `--fresh` to the build
command to discard absolute paths in the old CMake cache.

```powershell
python tools/bootstrap.py
python tools/build-local.py --smoke
python tools/build-installer.py
python tools/package.py
```

For a nonstandard OBS location:

```powershell
$env:GROUP_MIXER_OBS = 'D:\Applications\OBS'
```

The variable points to the directory containing `bin\64bit\obs64.exe`.
Build scripts only read this OBS installation; they do not install the plugin.

## Toolchain

`bootstrap.py` downloads CMake/Ninja and the official Visual Studio 17 release
manifest. `prepare-local.py` extracts a portable MSVC 14.44 compiler, Windows SDK,
OBS 33.0.2 headers and the OBS project's Qt 6.11.1 development package into `.deps`.
Microsoft payload hashes are checked against the vendor manifest. Qt release
digests are checked when supplied by GitHub. The OBS source archive is obtained
from the official tagged repository. Visual Studio is not installed system-wide.

`build-local.py` generates import libraries from the selected OBS DLL exports,
configures CMake, builds the plugin and runs model tests. `--smoke` also runs
a hidden Qt/libobs test application using generated audio. No hardware capture
sources or user OBS profiles are used. The isolated audio-only harness may log
missing frontend callbacks or graphics; test assertions and exit status determine success.

`python tools/test-in-obs.py` optionally checks loading in a separate portable OBS.
It stops its isolated process after checking dock registration and is not a full
OBS startup/shutdown test.

## Outputs

- `build/pecislav-group-mixer.dll`: plugin binary.
- `dist/Pecislav-Sound-Mixer-Setup.exe`: Windows installer with OBS/version/running-process checks and uninstaller.
- `dist/Pecislav-Sound-Mixer-Windows-x64.zip`: user installation package.
- `dist/Pecislav-Sound-Mixer-source.zip`: source archive with a project folder.
- `dist/Pecislav-Sound-Mixer-github.zip`: clean repository files at archive root.
- `dist/SHA256SUMS.txt`: checksums for generated packages and DLL.
- `test-output/`: test logs/screenshots; excluded from source archives.

Development dependencies, caches, logs, OBS binaries and previous local builds
are excluded from source archives. Build tools and caches can be recreated
with `python tools/bootstrap.py` after a local cleanup.

The GitHub workflow runs the same bootstrap, build and test commands on Windows
and uploads ZIPs for review. Future OBS/Qt upgrades should be tested together.

`build-installer.py` prepares a portable Inno Setup 7.1.1 compiler from its
official release and checks a pinned SHA256. Setup source is in `installer/`.
To test payload installation without administrator rights or touching OBS:
`python tools/build-installer.py --test-build`, then `python tests/test-installer.py`.
The private test build has a fixed project-local destination and no uninstaller
registration. Its test-only running-process override is not in production setup.
