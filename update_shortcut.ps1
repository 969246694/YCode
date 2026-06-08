# 更新 YCode 桌面快捷方式
# 确保快捷方式指向正确的启动脚本

$desktop = [Environment]::GetFolderPath('Desktop')
$shortcutPath = Join-Path $desktop 'YCode.lnk'
$target = 'F:\YiyangzaiCode\run_ycode.bat'
$workingDir = 'F:\YiyangzaiCode'
$iconPath = 'f:\YiyangzaiCode\YCode.ico'

$wsh = New-Object -ComObject WScript.Shell
$sc = $wsh.CreateShortcut($shortcutPath)
$sc.TargetPath = $target
$sc.WorkingDirectory = $workingDir
$sc.IconLocation = "$iconPath,0"
$sc.Description = 'YCode - DeepSeek AI 编程助手'
$sc.Save()

Write-Host "YCode 桌面快捷方式已更新" -ForegroundColor Green
Write-Host "目标: $target" -ForegroundColor Cyan
Write-Host "工作目录: $workingDir" -ForegroundColor Cyan
Write-Host "图标: $iconPath" -ForegroundColor Cyan