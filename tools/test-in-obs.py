"""Launch a separate portable OBS with no user configuration or capture plugins."""
import json
import os
import pathlib
import shutil
import subprocess
import sys
import time
import argparse

parser=argparse.ArgumentParser()
parser.add_argument('--locale',default='en-US')
args=parser.parse_args()

ROOT=pathlib.Path(__file__).resolve().parents[1]
TEST=ROOT/'.deps/obs-test'
INSTALL=pathlib.Path(os.environ.get('GROUP_MIXER_OBS','C:/Program Files/obs-studio'))
binary=TEST/'bin/64bit'
if not (binary/'obs64.exe').exists():
    shutil.copytree(INSTALL/'bin/64bit',binary,ignore=shutil.ignore_patterns('*.pdb'),dirs_exist_ok=True)
    for name in ('obs-studio','libobs'):
        shutil.copytree(INSTALL/'data'/name,TEST/'data'/name,dirs_exist_ok=True)
(TEST/'portable_mode.txt').touch()
for name in ('obs-outputs','obs-x264','obs-ffmpeg','obs-nvenc','obs-transitions','image-source'):
    shutil.copytree(INSTALL/'core'/name,TEST/'core'/name,
        ignore=shutil.ignore_patterns('*.pdb'),dirs_exist_ok=True)
config=TEST/'config/obs-studio';config.mkdir(parents=True,exist_ok=True)
(config/'user.ini').write_text('[General]\nFirstRun=true\nLanguage='+args.locale+'\n',encoding='utf-8')
(config/'global.ini').write_text('[General]\nLastVersion=553648130\n',encoding='utf-8')
for name in ('pecislav-group-mixer','obs-test-driver'):
    folder=TEST/'plugins'/name;folder.mkdir(parents=True,exist_ok=True)
    shutil.copy2(ROOT/'build'/(name+'.dll'),folder/(name+'.dll'))
    if name=='pecislav-group-mixer':shutil.copytree(ROOT/'data',folder/'data',dirs_exist_ok=True)
env=os.environ.copy();env['QT_QPA_PLATFORM']='minimal'
env.pop('QT_PLUGIN_PATH',None)
env['OBS_PLUGINS_PATH']=str(TEST/'unused-plugins')
env['OBS_PLUGINS_DATA_PATH']=str(TEST/'unused-data')
startup=subprocess.STARTUPINFO();startup.dwFlags|=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0
started=time.time()
process=subprocess.Popen([str(binary/'obs64.exe'),'--portable','--disable-updater','--multi'],
    cwd=binary,env=env,startupinfo=startup,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
log=''
try:
    while time.time()-started<20:
        logs=sorted((config/'logs').glob('*.txt'),key=lambda p:p.stat().st_mtime)
        if logs and logs[-1].stat().st_mtime>=started:
            log=logs[-1].read_text(encoding='utf-8',errors='replace')
        if ('Real OBS dock registration: PASS' in log and 'OBS locale: '+args.locale+';' in log) or process.poll() is not None:
            break
        time.sleep(.2)
finally:
    # This intentionally minimal OBS copy has no full core plugin set. Only
    # DLL loading/dock registration is under test here; lifecycle and real
    # audio controls are tested by mixer-smoke with proper teardown.
    if process.poll() is None:process.terminate()
    process.wait(timeout=10)
output=ROOT/'test-output';output.mkdir(exist_ok=True)
(output/'obs-integration.log').write_text(log,encoding='utf-8')
for line in log.splitlines():
    if any(t in line for t in ('Group Mixer','GM integration','not be loaded','Failed to load','Crashed','leaked')):
        print(line)
ok='Real OBS dock registration: PASS' in log and '[Pecislav Group Mixer] Loaded' in log and 'OBS locale: '+args.locale+';' in log
print('Stopped isolated OBS after dock registration check (not a full application startup test).')
print('PASS' if ok else 'FAIL')
sys.exit(0 if ok else 1)
