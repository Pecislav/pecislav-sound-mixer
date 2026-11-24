import json
import pathlib
import subprocess
import sys
import urllib.request

root=pathlib.Path(__file__).resolve().parents[1]
deps=root/'.deps';deps.mkdir(exist_ok=True)
req=urllib.request.Request('https://aka.ms/vs/17/release/channel',headers={'User-Agent':'ObsGroupMixer-build'})
channel=json.load(urllib.request.urlopen(req))
url=next(p['url'] for item in channel['channelItems'] if item['id']=='Microsoft.VisualStudio.Manifests.VisualStudio' for p in item['payloads'])
manifest=json.load(urllib.request.urlopen(url))
(deps/'vs-manifest.json').write_text(json.dumps(manifest),encoding='utf-8')
subprocess.run([sys.executable,'-m','pip','install','--target',str(deps/'python-tools'),'cmake==4.4.5','ninja==1.13.2'],check=True)
subprocess.run([sys.executable,str(root/'tools/prepare-local.py')],check=True)
