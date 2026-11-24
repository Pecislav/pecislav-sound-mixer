import hashlib
import pathlib
import shutil
import zipfile

ROOT=pathlib.Path(__file__).resolve().parents[1]
DIST=ROOT/'dist';DIST.mkdir(exist_ok=True)
PACKAGE=DIST/'Pecislav-Sound-Mixer-Windows-x64'
PAYLOAD=PACKAGE/'pecislav-group-mixer';PAYLOAD.mkdir(parents=True,exist_ok=True)
shutil.copy2(ROOT/'build/pecislav-group-mixer.dll',PAYLOAD/'pecislav-group-mixer.dll')
shutil.copytree(ROOT/'data',PAYLOAD/'data',dirs_exist_ok=True)
for name in ('install.ps1','Instalovat.cmd'):shutil.copy2(ROOT/'tools'/name,PACKAGE/name)
for name in ('README.md','README.cs.md','LICENSE','CHANGELOG.md','RELEASE-NOTES.md','BUILDING.md','CONTRIBUTING.md','GITHUB-PUBLISH.cs.md','CHECKS.cs.md'):
    shutil.copy2(ROOT/name,PACKAGE/name)
shutil.copytree(ROOT/'docs',PACKAGE/'docs',dirs_exist_ok=True)
binary_zip=DIST/(PACKAGE.name+'.zip')
with zipfile.ZipFile(binary_zip,'w',zipfile.ZIP_DEFLATED) as archive:
    for path in sorted(PACKAGE.rglob('*')):
        if path.is_file():archive.write(path,path.relative_to(DIST))
# Explicit inclusion keeps downloaded tools, caches, logs and OBS profiles out.
source_files=[path for path in ROOT.iterdir() if path.is_file() and
              (path.suffix=='.md' or path.name in {'.gitignore','CMakeLists.txt','LICENSE'})]
for directory in ('src','data','tools','tests','docs','.github','installer'):
    source_files.extend(path for path in (ROOT/directory).rglob('*') if path.is_file()
                        and '__pycache__' not in path.parts and path.suffix!='.pyc')
source_zip=DIST/'Pecislav-Sound-Mixer-source.zip'
github_zip=DIST/'Pecislav-Sound-Mixer-github.zip'
for destination,prefix in ((source_zip,pathlib.Path('ObsGroupMixer')),(github_zip,pathlib.Path('.'))):
    with zipfile.ZipFile(destination,'w',zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(source_files):archive.write(path,prefix/path.relative_to(ROOT))
hashes=[]
checksum_files=[binary_zip,source_zip,github_zip,PAYLOAD/'pecislav-group-mixer.dll']
setup=DIST/'Pecislav-Sound-Mixer-Setup.exe'
if setup.exists():checksum_files.append(setup)
for path in checksum_files:
    digest=hashlib.file_digest(path.open('rb'),'sha256').hexdigest();hashes.append(digest+'  '+str(path.relative_to(DIST)))
(DIST/'SHA256SUMS.txt').write_text('\n'.join(hashes)+'\n',encoding='utf-8')
print(binary_zip)
print(source_zip)
print(github_zip)
