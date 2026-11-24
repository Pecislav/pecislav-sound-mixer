"""Exercise the private, non-admin installer only inside the project test area."""
import hashlib
import ctypes
import pathlib
import os
import subprocess
import uuid

ROOT=pathlib.Path(__file__).resolve().parents[1]
AREA=ROOT/'.deps/installer-check'
SETUP=AREA/'SoundMixer-TestSetup.exe'
TARGET=AREA/('test-'+uuid.uuid4().hex)
AREA.mkdir(exist_ok=True)
startup=subprocess.STARTUPINFO();startup.dwFlags|=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0
common=[str(SETUP),'/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/LANG=english','/DIR='+str(TARGET)]
obsRoot='/OBSROOT='+os.environ.get('GROUP_MIXER_OBS','C:/Program Files/obs-studio')
# The test-only switch is absent from production setup. It permits isolated
# payload checks while the user's real OBS remains open, without touching it.
invalid=subprocess.run(common+['/OBSROOT='+str(AREA/'missing-obs'),'/TESTIGNOREOBS=1','/LOG='+str(AREA/'invalid.log')],startupinfo=startup,timeout=40)
assert invalid.returncode!=0,'Installer accepted missing OBS'
assert not (TARGET/'pecislav-group-mixer.dll').exists(),'Rejected setup wrote plugin files'
valid=subprocess.run(common+[obsRoot,'/TESTIGNOREOBS=1','/LOG='+str(AREA/'valid.log')],startupinfo=startup,timeout=40)
assert valid.returncode==0,valid.returncode
expected=hashlib.file_digest((ROOT/'build/pecislav-group-mixer.dll').open('rb'),'sha256').hexdigest()
actual=hashlib.file_digest((TARGET/'pecislav-group-mixer.dll').open('rb'),'sha256').hexdigest()
assert actual==expected,'Installer payload differs from built plugin'
assert (TARGET/'data/locale/cs-CZ.ini').exists(),'Locale data missing'
assert (TARGET/'docs/LICENSE').exists(),'License missing'
print('PASS: installer rejects absent OBS and installs exact DLL/data into isolated, non-admin test directory')
kernel=ctypes.windll.kernel32
kernel.OpenMutexW.argtypes=[ctypes.c_uint,ctypes.c_bool,ctypes.c_wchar_p]
kernel.OpenMutexW.restype=ctypes.c_void_p
kernel.CloseHandle.argtypes=[ctypes.c_void_p]
handle=kernel.OpenMutexW(0x100000,False,'OBSStudioCore')
if handle:
    kernel.CloseHandle(handle)
    running=subprocess.run(common+[obsRoot,'/LOG='+str(AREA/'running.log')],startupinfo=startup,timeout=40)
    assert running.returncode!=0,'Installer allowed updating while OBS was running'
    log=(AREA/'running.log').read_text(encoding='utf-8',errors='replace')
    assert 'Close OBS Studio before installing' in log,'Running-process check failed for another reason'
    print('PASS: installer refuses an update while the real OBS process is running')
