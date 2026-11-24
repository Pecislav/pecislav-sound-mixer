"""Prepare a portable MSVC/SDK toolchain from Microsoft's signed distribution.

No Visual Studio installation, registry change or administrator access is needed.
Downloads remain in .deps. Download digests are checked against vendor manifests.
"""
import concurrent.futures
import ctypes
import hashlib
import json
import pathlib
import subprocess
import urllib.request
import zipfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
DEPS = ROOT / '.deps'
CACHE = DEPS / 'downloads'
VC = DEPS / 'msvc'
SDK = DEPS / 'sdk'
for path in (CACHE, VC, SDK):
    path.mkdir(parents=True, exist_ok=True)

def download(url, name, digest=None):
    path = CACHE / name
    if not path.exists():
        print('Downloading', name, flush=True)
        req = urllib.request.Request(url, headers={'User-Agent': 'ObsGroupMixer-build'})
        with urllib.request.urlopen(req, timeout=120) as src, path.with_suffix(path.suffix+'.part').open('wb') as dst:
            while chunk := src.read(1024*1024):
                dst.write(chunk)
        path.with_suffix(path.suffix+'.part').replace(path)
    if digest and hashlib.file_digest(path.open('rb'), 'sha256').hexdigest().lower() != digest.lower():
        raise RuntimeError('SHA256 mismatch: '+name)
    return path

manifest = json.loads((DEPS/'vs-manifest.json').read_text())
packages = manifest['packages']

def payload_file(payload):
    return download(payload['url'], pathlib.PurePosixPath(payload['fileName']).name, payload.get('sha256'))

def prepare_vc():
    ids = [
        'Microsoft.VC.14.44.17.14.Tools.HostX64.TargetX64.base',
        'Microsoft.VC.14.44.17.14.Tools.HostX64.TargetX64.Res.base',
        'Microsoft.VC.14.44.17.14.CRT.Headers.base',
        'Microsoft.VC.14.44.17.14.CRT.x64.Desktop.base',
        'Microsoft.VC.14.44.17.14.CRT.x64.OneCore.Desktop.base',
    ]
    for id in ids:
        candidates = [p for p in packages if p['id']==id]
        package = next((p for p in candidates if p.get('language')=='en-US'), candidates[0])
        for payload in package.get('payloads', []):
            path = payload_file(payload)
            with zipfile.ZipFile(path) as z:
                for item in z.infolist():
                    name = urllib.parse.unquote(item.filename)
                    if not name.startswith('Contents/') or item.is_dir():
                        continue
                    target = VC / name[len('Contents/'):]
                    if not target.resolve().is_relative_to(VC.resolve()):
                        raise RuntimeError('Unsafe archive path')
                    if target.exists() and target.stat().st_size == item.file_size:
                        continue
                    target.parent.mkdir(parents=True, exist_ok=True)
                    target.write_bytes(z.read(item))
    print('MSVC ready', flush=True)

msi = ctypes.WinDLL('msi')
msi.MsiOpenDatabaseW.argtypes=[ctypes.c_wchar_p,ctypes.c_wchar_p,ctypes.POINTER(ctypes.c_uint)]
msi.MsiDatabaseOpenViewW.argtypes=[ctypes.c_uint,ctypes.c_wchar_p,ctypes.POINTER(ctypes.c_uint)]
msi.MsiViewExecute.argtypes=[ctypes.c_uint,ctypes.c_uint]
msi.MsiViewFetch.argtypes=[ctypes.c_uint,ctypes.POINTER(ctypes.c_uint)]
msi.MsiRecordGetStringW.argtypes=[ctypes.c_uint,ctypes.c_uint,ctypes.c_wchar_p,ctypes.POINTER(ctypes.c_uint)]
msi.MsiCloseHandle.argtypes=[ctypes.c_uint]

def cabinets(path):
    db,view=ctypes.c_uint(),ctypes.c_uint()
    assert msi.MsiOpenDatabaseW(str(path),None,ctypes.byref(db))==0
    try:
        assert msi.MsiDatabaseOpenViewW(db,'SELECT `Cabinet` FROM `Media`',ctypes.byref(view))==0
        assert msi.MsiViewExecute(view,0)==0
        result=[]
        while True:
            record=ctypes.c_uint()
            if msi.MsiViewFetch(view,ctypes.byref(record))!=0:
                break
            buf=ctypes.create_unicode_buffer(1024);size=ctypes.c_uint(1024)
            msi.MsiRecordGetStringW(record,1,buf,ctypes.byref(size))
            msi.MsiCloseHandle(record)
            if buf.value and not buf.value.startswith('#'):
                result.append(buf.value)
        return result
    finally:
        msi.MsiCloseHandle(view);msi.MsiCloseHandle(db)

def prepare_sdk():
    package=next(p for p in packages if p['id']=='Win11SDK_10.0.26100')
    payloads={pathlib.PurePosixPath(p['fileName']).name:p for p in package['payloads']}
    names=[
        'Windows SDK Desktop Headers x86-x86_en-us.msi',
        'Windows SDK Desktop Headers x64-x86_en-us.msi',
        'Windows SDK Desktop Libs x64-x86_en-us.msi',
        'Windows SDK for Windows Store Apps Headers-x86_en-us.msi',
        'Windows SDK for Windows Store Apps Libs-x86_en-us.msi',
        'Universal CRT Headers Libraries and Sources-x86_en-us.msi',
        'Windows SDK Desktop Tools x64-x86_en-us.msi',
        'Windows SDK for Windows Store Apps Tools-x86_en-us.msi',
    ]
    for name in names:
        path=payload_file(payloads[name])
        cabs=cabinets(path)
        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
            list(pool.map(lambda name:payload_file(payloads[name]),cabs))
        result=subprocess.run(['msiexec.exe','/a',str(path),'/qn','TARGETDIR='+str(SDK),'/l*v',str(DEPS/(name+'.log'))])
        if result.returncode not in (0,3010):
            raise RuntimeError(f'Administrative SDK extraction failed: {name}: {result.returncode}')
    print('Windows SDK ready', flush=True)

def prepare_obs_qt():
    qt_name='windows-deps-qt6-2026-10-23-x64-RelWithDebInfo.zip'
    request=urllib.request.Request('https://api.github.com/repos/obsproject/obs-deps/releases/tags/2026-10-23',headers={'User-Agent':'ObsGroupMixer-build'})
    release=json.load(urllib.request.urlopen(request))
    asset=next(a for a in release['assets'] if a['name']==qt_name)
    qt=download(asset['browser_download_url'],qt_name,asset.get('digest','').removeprefix('sha256:') or None)
    with zipfile.ZipFile(qt) as z:
        z.extractall(DEPS/'qt')
    source=download('https://github.com/obsproject/obs-studio/archive/refs/tags/33.0.2.zip','obs-source-33.0.2.zip')
    with zipfile.ZipFile(source) as z:
        z.extractall(DEPS/'obs-source')
    print('OBS headers and Qt ready', flush=True)

if __name__=='__main__':
    with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:
        futures=[pool.submit(fn) for fn in (prepare_vc,prepare_sdk,prepare_obs_qt)]
        for future in futures:
            future.result()
    print('All portable build dependencies ready',flush=True)
