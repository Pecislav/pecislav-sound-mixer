"""Build a real Windows setup program using a pinned portable Inno compiler."""
import argparse
import hashlib
import os
import pathlib
import re
import subprocess
import urllib.request

ROOT=pathlib.Path(__file__).resolve().parents[1]
VERSION=re.search(r'project\(pecislav-group-mixer VERSION ([0-9.]+)',(ROOT/'CMakeLists.txt').read_text()).group(1)
INNO=ROOT/'.deps/inno/ISCC.exe'
parser=argparse.ArgumentParser()
parser.add_argument('--test-build',action='store_true',help='Build a non-admin private installer for isolated testing')
args=parser.parse_args()
if not INNO.exists():
    INNO.parent.mkdir(parents=True,exist_ok=True)
    download=ROOT/'.deps/innosetup-7.1.1-x64.exe'
    if not download.exists():
        urllib.request.urlretrieve('https://github.com/jrsoftware/issrc/releases/download/is-7_1_1/innosetup-7.1.1-x64.exe',download)
    if hashlib.file_digest(download.open('rb'),'sha256').hexdigest()!='8a5a6711c3766a6e5fb74f280ba0e3551e719ce94538d901fdbd1f8f9920b3f0':
        raise RuntimeError('Inno Setup download hash mismatch')
    startup=subprocess.STARTUPINFO();startup.dwFlags|=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0
    subprocess.run([str(download),'/PORTABLE=1','/CURRENTUSER','/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/DIR='+str(INNO.parent)],startupinfo=startup,check=True)
command=[str(INNO),'/Qp','/DAppVersion='+VERSION,'/DProjectRoot='+str(ROOT)]
if args.test_build:command.append('/DTestBuild=1')
command.append(str(ROOT/'installer/sound-mixer.iss'))
subprocess.run(command,check=True,cwd=ROOT)
output=ROOT/('.deps/installer-check/SoundMixer-TestSetup.exe' if args.test_build else 'dist/Pecislav-Sound-Mixer-Setup.exe')
print(output)
