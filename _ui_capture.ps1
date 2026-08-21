Add-Type @"
using System;
using System.Runtime.InteropServices;
public class W {
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
    public struct RECT { public int L, T, R, B; }
}
"@
$env:DEEPSEEK_API_KEY = "sk-test-dummy-key-00000000000000000000000000"
$p = Start-Process -FilePath "F:\YiyangzaiCode\YZCodex\build\msvc2022_64\Release\YCode.exe" -PassThru
Start-Sleep -Seconds 5
$p.Refresh()
$hwnd = $p.MainWindowHandle
if ($hwnd -eq 0) { Write-Output "no main window handle"; Stop-Process -Id $p.Id -Force; exit 1 }
[W]::ShowWindow($hwnd, 9) | Out-Null
[W]::SetForegroundWindow($hwnd) | Out-Null
Start-Sleep -Milliseconds 500
$r = New-Object W+RECT
[W]::GetWindowRect($hwnd, [ref]$r) | Out-Null
$rw = $r.R - $r.L
$rh = $r.B - $r.T
Write-Output "window rect: L=$($r.L) T=$($r.T) W=$rw H=$rh"
Add-Type -AssemblyName System.Drawing
$bmp = New-Object System.Drawing.Bitmap($rw, $rh)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($r.L, $r.T, 0, 0, (New-Object System.Drawing.Size($rw, $rh)))
$bmp.Save("F:\YiyangzaiCode\_ui_check.png", [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose()
$bmp.Dispose()
Stop-Process -Id $p.Id -Force
Write-Output "captured"
