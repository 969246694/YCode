# YCode API Key 管理脚本

$Script:Root = Split-Path -Parent $MyInvocation.MyCommand.Path

function Show-Menu {
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "YCode DeepSeek API Key 管理" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "1. 查看当前 API Key"
    Write-Host "2. 设置/修改 API Key (当前会话)"
    Write-Host "3. 设置 API Key (永久，用户环境变量)"
    Write-Host "4. 启动 YCode"
    Write-Host "5. 退出"
    Write-Host ""
    $choice = Read-Host "请选择 (1-5)"
    return $choice
}

function Get-ApiKey {
    $key = $env:DEEPSEEK_API_KEY
    if ($key) {
        $masked = $key.Substring(0, [Math]::Min(10, $key.Length)) + "..." + $key.Substring([Math]::Max(0, $key.Length - 5))
        Write-Host "当前 API Key: $masked" -ForegroundColor Green
    }
    else {
        Write-Host "未设置 API Key" -ForegroundColor Yellow
    }
}

function Set-ApiKeySession {
    $key = Read-Host "请输入 DeepSeek API Key"
    $env:DEEPSEEK_API_KEY = $key
    Write-Host "API Key 已为当前会话设置" -ForegroundColor Green
    Write-Host "当前进程 API Key: $($key.Substring(0, [Math]::Min(10, $key.Length)))..." -ForegroundColor Green
}

function Set-ApiKeyPermanent {
    $key = Read-Host "请输入 DeepSeek API Key"
    [Environment]::SetEnvironmentVariable("DEEPSEEK_API_KEY", $key, [EnvironmentVariableTarget]::User)
    Write-Host "API Key 已设置到用户环境变量" -ForegroundColor Green
    Write-Host "请重启应用程序以使用新的 API Key" -ForegroundColor Yellow
}

function Start-YCode {
    $exe = Join-Path $Script:Root "YZCodex\build\msvc2022_64\Release\YCode.exe"
    
    if (-not (Test-Path $exe)) {
        Write-Host "错误: 找不到应用程序: $exe" -ForegroundColor Red
        return
    }
    
    $apiKey = $env:DEEPSEEK_API_KEY
    if (-not $apiKey) {
        Write-Host "警告: 未设置 DEEPSEEK_API_KEY 环境变量" -ForegroundColor Yellow
        $confirm = Read-Host "继续启动应用? (y/n)"
        if ($confirm -ne 'y') {
            return
        }
    }
    
    Push-Location -Path $Script:Root
    Start-Process -FilePath $exe
    Pop-Location
    Write-Host "YCode 已启动" -ForegroundColor Green
}

# 主循环
while ($true) {
    $choice = Show-Menu
    
    switch ($choice) {
        "1" {
            Get-ApiKey
        }
        "2" {
            Set-ApiKeySession
        }
        "3" {
            Set-ApiKeyPermanent
        }
        "4" {
            Start-YCode
            break
        }
        "5" {
            break
        }
        default {
            Write-Host "无效选择" -ForegroundColor Red
        }
    }
    
    Write-Host ""
    Read-Host "按 Enter 继续"
    Clear-Host
}
