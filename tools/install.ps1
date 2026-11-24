$ErrorActionPreference = 'Stop'
$pluginName = 'pecislav-group-mixer'
$payloadPath = Join-Path $PSScriptRoot $pluginName
$sourceDll = Join-Path $payloadPath ($pluginName + '.dll')
if (-not (Test-Path -LiteralPath $sourceDll)) {
    throw 'V instalacnim balicku chybi DLL pluginu. Nejprve rozbal cely ZIP.'
}
if (Get-Process obs64 -ErrorAction SilentlyContinue) {
    throw 'Pred instalaci ukonci OBS a spust instalaci znovu.'
}
$pluginRoot = [System.IO.Path]::GetFullPath((Join-Path $env:ProgramData 'obs-studio/plugins'))
$targetPath = [System.IO.Path]::GetFullPath((Join-Path $pluginRoot $pluginName))
if ([System.IO.Path]::GetDirectoryName($targetPath) -ne $pluginRoot) {
    throw 'Neplatna cilova cesta.'
}
$installedDll = Join-Path $targetPath ($pluginName + '.dll')
if (Test-Path -LiteralPath $installedDll) {
    $backupPath = Join-Path $targetPath 'backup'
    New-Item -ItemType Directory -Path $backupPath -Force | Out-Null
    Copy-Item -LiteralPath $installedDll -Destination $backupPath -Force
}
New-Item -ItemType Directory -Path $targetPath -Force | Out-Null
Copy-Item -LiteralPath $sourceDll -Destination $installedDll -Force
Copy-Item -LiteralPath (Join-Path $payloadPath 'data') -Destination $targetPath -Recurse -Force
Write-Host 'Plugin je nainstalovany.'
Write-Host 'Spust OBS a v nabidce Panely / Docks zapni Zvukovy mixer.'
Write-Host $targetPath
