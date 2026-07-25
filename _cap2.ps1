param([string]$Out="C:\Secret_Project\Saved\shots\g.png")
Add-Type -AssemblyName System.Drawing
$sig=@'
using System;using System.Runtime.InteropServices;using System.Drawing;
public class WC{
[DllImport("user32.dll")]public static extern bool PrintWindow(IntPtr h,IntPtr dc,uint f);
[DllImport("user32.dll")]public static extern IntPtr GetWindowRect(IntPtr h,out R r);
[DllImport("user32.dll")]public static extern bool SetForegroundWindow(IntPtr h);
[DllImport("user32.dll")]public static extern bool ShowWindow(IntPtr h,int c);
[StructLayout(LayoutKind.Sequential)]public struct R{public int L,T,Rt,B;}
public static Bitmap Cap(IntPtr h){R r;GetWindowRect(h,out r);int w=r.Rt-r.L,ht=r.B-r.T;if(w<=0||ht<=0)return null;
Bitmap b=new Bitmap(w,ht);using(Graphics g=Graphics.FromImage(b)){IntPtr dc=g.GetHdc();PrintWindow(h,dc,2);g.ReleaseHdc(dc);}return b;}}
'@
Add-Type -TypeDefinition $sig -ReferencedAssemblies System.Drawing
$p=Get-Process UnrealEditor -ErrorAction SilentlyContinue | Select-Object -First 1
if(-not $p){Write-Output "NOPROC";exit 1}
[WC]::ShowWindow($p.MainWindowHandle,9)|Out-Null; [WC]::SetForegroundWindow($p.MainWindowHandle)|Out-Null
Start-Sleep -Milliseconds 800
$b=[WC]::Cap($p.MainWindowHandle)
if($b -eq $null){Write-Output "NULL";exit 1}
$d=Split-Path $Out;if(-not(Test-Path $d)){New-Item -ItemType Directory -Force $d|Out-Null}
$b.Save($Out,[System.Drawing.Imaging.ImageFormat]::Png)
Write-Output ("SAVED {0}x{1}" -f $b.Width,$b.Height)
