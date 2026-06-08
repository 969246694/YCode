# Update the YCode desktop shortcut so it points at this repository.

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$desktop = [Environment]::GetFolderPath('Desktop')
$shortcutPath = Join-Path $desktop 'YCode.lnk'
$target = Join-Path $repoRoot 'run_ycode.bat'
$workingDir = $repoRoot
$iconPath = Join-Path $repoRoot 'YCode.ico'

$wsh = New-Object -ComObject WScript.Shell
$shortcut = $wsh.CreateShortcut($shortcutPath)
$shortcut.TargetPath = $target
$shortcut.WorkingDirectory = $workingDir
$shortcut.IconLocation = "$iconPath,0"
$shortcut.Description = 'YCode - DeepSeek AI programming assistant'
$shortcut.Save()

Write-Host "YCode desktop shortcut updated." -ForegroundColor Green
Write-Host "Target: $target" -ForegroundColor Cyan
Write-Host "Working directory: $workingDir" -ForegroundColor Cyan
Write-Host "Icon: $iconPath" -ForegroundColor Cyan
