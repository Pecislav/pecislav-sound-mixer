"""Build against the installed OBS without modifying that installation."""
import os
import pathlib
import re
import subprocess
import sys

ROOT=pathlib.Path(__file__).resolve().parents[1]
DEPS=ROOT/'.deps'
BUILD=ROOT/'build'
OBS=pathlib.Path(os.environ.get('GROUP_MIXER_OBS','C:/Program Files/obs-studio'))
VC=next((DEPS/'msvc/VC/Tools/MSVC').iterdir())
SDK=DEPS/'sdk/Windows Kits/10'
VERSION=next((SDK/'Include').iterdir()).name
QT=DEPS/'qt'
TOOLS=DEPS/'python-tools'
BIN=VC/'bin/Hostx64/x64'
CMAKE=TOOLS/'cmake/data/bin/cmake.exe'
NINJA=TOOLS/'bin/ninja.exe'
env=os.environ.copy()
env['PATH']=os.pathsep.join(map(str,[BIN,SDK/'bin'/VERSION/'x64',OBS/'bin/64bit',QT/'bin',TOOLS/'bin']))+os.pathsep+env['PATH']
env['INCLUDE']=os.pathsep.join(map(str,[VC/'include',SDK/'Include'/VERSION/'ucrt',SDK/'Include'/VERSION/'shared',SDK/'Include'/VERSION/'um',SDK/'Include'/VERSION/'winrt']))
env['LIB']=os.pathsep.join(map(str,[VC/'lib/x64',VC/'lib/onecore/x64',SDK/'Lib'/VERSION/'ucrt/x64',SDK/'Lib'/VERSION/'um/x64']))
env['QT_PLUGIN_PATH']=str(QT/'plugins')

def run(args,**kwargs):
    subprocess.run(list(map(str,args)),env=env,cwd=ROOT,check=True,**kwargs)

def prepare_imports():
    imports=DEPS/'imports';imports.mkdir(exist_ok=True)
    for name in ('obs','obs-frontend-api'):
        dll=OBS/'bin/64bit'/(name+'.dll')
        listing=subprocess.check_output([str(BIN/'dumpbin.exe'),'/nologo','/exports',str(dll)],env=env,text=True,errors='replace')
        symbols=[]
        for line in listing.splitlines():
            match=re.match(r'\s+\d+\s+[0-9A-F]+\s+[0-9A-F]+\s+(\S+)',line)
            if match:symbols.append(match[1])
        if not symbols:raise RuntimeError('No DLL exports: '+name)
        definition=imports/(name+'.def')
        definition.write_text('LIBRARY '+name+'\nEXPORTS\n'+'\n'.join(symbols)+'\n')
        run([BIN/'lib.exe','/nologo','/machine:x64','/def:'+str(definition),'/out:'+str(imports/(name+'.lib'))])
    return imports

if __name__=='__main__':
    imports=prepare_imports()
    configure=[CMAKE,'-S',ROOT,'-B',BUILD,'-G','Ninja',
         '-DCMAKE_BUILD_TYPE=RelWithDebInfo','-DCMAKE_CXX_COMPILER='+(BIN/'cl.exe').as_posix(),
         '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL',
         '-DCMAKE_RC_COMPILER='+(SDK/'bin'/VERSION/'x64/rc.exe').as_posix(),
         '-DCMAKE_MT='+(SDK/'bin'/VERSION/'x64/mt.exe').as_posix(),
         '-DCMAKE_MAKE_PROGRAM='+NINJA.as_posix(),'-DCMAKE_PREFIX_PATH='+QT.as_posix(),
         '-DOBS_SOURCE_DIR='+(DEPS/'obs-source/obs-studio-33.0.2').as_posix(),
         '-DOBS_IMPORT_DIR='+imports.as_posix()]
    if '--fresh' in sys.argv:configure.append('--fresh')
    run(configure)
    run([CMAKE,'--build',BUILD,'--parallel','4'])
    run([TOOLS/'cmake/data/bin/ctest.exe','--test-dir',BUILD,'--output-on-failure'])
    if '--smoke' in sys.argv:
        env['QT_QPA_PLATFORM']='windows'
        run([BUILD/'mixer-smoke.exe'])
