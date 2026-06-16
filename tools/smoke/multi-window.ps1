# Smoke test: modal BrowserWindow — open dialog, verify parent disabled, close dialog.
param(
    [Parameter(Mandatory = $true)]
    [string]$SampleExe,
    [int]$StartupTimeoutSec = 20
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $SampleExe)) {
    Write-Error "Sample not found: $SampleExe"
}

Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class KutieWin32 {
    [DllImport("user32.dll")]
    public static extern bool IsWindowEnabled(IntPtr hWnd);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern IntPtr FindWindow(string lpClassName, string lpWindowName);
    [DllImport("user32.dll")]
    public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    public const uint WM_CLOSE = 0x0010;
}
"@

Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes

function Get-ProcessWindows($proc) {
    $root = [System.Windows.Automation.AutomationElement]::RootElement
    $cond = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::ProcessIdProperty, $proc.Id)
    return $root.FindAll([System.Windows.Automation.TreeScope]::Children, $cond)
}

function Find-InProcess($proc, [string]$name) {
    foreach ($win in Get-ProcessWindows $proc) {
        $btnCond = New-Object System.Windows.Automation.PropertyCondition(
            [System.Windows.Automation.AutomationElement]::NameProperty, $name)
        $match = $win.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $btnCond)
        if ($match) { return $match }
    }
    return $null
}

function Invoke-UiButton($element) {
    $pattern = $element.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern)
    $pattern.Invoke()
}

Get-Process sample -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 500

$proc = Start-Process -FilePath $SampleExe -PassThru
try {
    $openBtn = $null
    for ($i = 0; $i -lt $StartupTimeoutSec; $i++) {
        Start-Sleep -Seconds 1
        if ($proc.HasExited) {
            throw "sample.exe exited during startup (code $($proc.ExitCode))"
        }
        $openBtn = Find-InProcess $proc 'Open modal dialog'
        if ($openBtn) { break }
    }
    if (-not $openBtn) {
        throw "Open modal dialog button not found within ${StartupTimeoutSec}s"
    }

    $proc.Refresh()
    $mainHwnd = $proc.MainWindowHandle
    if ($mainHwnd -eq [IntPtr]::Zero) {
        throw "Main window handle not available"
    }

    Invoke-UiButton $openBtn
    Start-Sleep -Seconds 2

    if ($proc.HasExited) {
        throw "sample.exe crashed after opening modal"
    }

    if ([KutieWin32]::IsWindowEnabled($mainHwnd)) {
        throw "Main window should be disabled while modal is open"
    }

    $modalHwnd = [IntPtr]::Zero
    for ($i = 0; $i -lt 10; $i++) {
        Start-Sleep -Seconds 1
        $modalHwnd = [KutieWin32]::FindWindow([NullString]::Value, 'Modal Dialog')
        if ($modalHwnd -ne [IntPtr]::Zero) { break }
    }
    if ($modalHwnd -eq [IntPtr]::Zero) {
        throw "Modal window (title 'Modal Dialog') not found"
    }

    [void][KutieWin32]::PostMessage($modalHwnd, [KutieWin32]::WM_CLOSE, [IntPtr]::Zero, [IntPtr]::Zero)
    Start-Sleep -Seconds 1

    if (-not [KutieWin32]::IsWindowEnabled($mainHwnd)) {
        throw "Main window should be re-enabled after modal closes"
    }

    Write-Host "smoke multi-window: passed" -ForegroundColor Green
}
finally {
    if (-not $proc.HasExited) {
        Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
    }
}
