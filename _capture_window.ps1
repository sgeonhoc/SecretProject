# UE -game 창을 PrintWindow(PW_RENDERFULLCONTENT)로 잡아 PNG 저장.
# 사용: powershell -File _capture_window.ps1 -Out C:\...\shot.png [-TitleLike "Secret_Project"]
param(
  [string]$Out = "C:\Secret_Project\Saved\shots\game.png",
  [string]$TitleLike = ""
)
Add-Type -AssemblyName System.Drawing
$sig = @'
using System;
using System.Runtime.InteropServices;
using System.Drawing;
public class WinCap {
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdc, uint flags);
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hwnd, out RECT r);
  [DllImport("user32.dll")] public static extern IntPtr GetWindowRect(IntPtr hwnd, out RECT r);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
  public static Bitmap Cap(IntPtr hwnd) {
    RECT r; GetWindowRect(hwnd, out r);
    int w = r.Right - r.Left, h = r.Bottom - r.Top;
    if (w <= 0 || h <= 0) return null;
    Bitmap bmp = new Bitmap(w, h);
    using (Graphics g = Graphics.FromImage(bmp)) {
      IntPtr hdc = g.GetHdc();
      PrintWindow(hwnd, hdc, 2); // PW_RENDERFULLCONTENT
      g.ReleaseHdc(hdc);
    }
    return bmp;
  }
}
'@
Add-Type -TypeDefinition $sig -ReferencedAssemblies System.Drawing

# UE 게임 창 찾기: 제목에 프로젝트명 또는 "(64-bit"이 든 최상위 창
$procs = Get-Process | Where-Object { $_.MainWindowHandle -ne 0 -and $_.MainWindowTitle -ne "" }
$target = $null
foreach ($p in $procs) {
  $t = $p.MainWindowTitle
  if ($t -match "Secret_Project" -or $t -match "\(64-bit" -or ($TitleLike -ne "" -and $t -match $TitleLike)) {
    if ($p.ProcessName -match "Unreal|Secret") { $target = $p; break }
    $target = $p
  }
}
if (-not $target) {
  Write-Output "NO_WINDOW"
  $procs | ForEach-Object { Write-Output ("  proc={0} title={1}" -f $_.ProcessName, $_.MainWindowTitle) }
  exit 1
}
Write-Output ("WINDOW proc={0} title={1}" -f $target.ProcessName, $target.MainWindowTitle)
$bmp = [WinCap]::Cap($target.MainWindowHandle)
if ($bmp -eq $null) { Write-Output "CAP_NULL"; exit 1 }
$dir = Split-Path $Out
if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
Write-Output ("SAVED {0} ({1}x{2})" -f $Out, $bmp.Width, $bmp.Height)
