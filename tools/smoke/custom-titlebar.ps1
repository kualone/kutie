# Smoke test: custom titlebar mode — switch, close via titlebar button.
param(
    [Parameter(Mandatory = $true)]
    [string]$SampleExe,
    [int]$StartupTimeoutSec = 20
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $SampleExe)) {
    Write-Error "Sample not found: $SampleExe"
}

Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes

function Get-ProcWindow($proc) {
    $root = [System.Windows.Automation.AutomationElement]::RootElement
    $cond = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::ProcessIdProperty, $proc.Id)
    return $root.FindFirst([System.Windows.Automation.TreeScope]::Children, $cond)
}

function Find-InProcess($proc, [string]$name) {
    $win = Get-ProcWindow $proc
    if (-not $win) { return $null }
    $btnCond = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::NameProperty, $name)
    return $win.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $btnCond)
}

function Invoke-UiButton($element) {
    $pattern = $element.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern)
    $pattern.Invoke()
}

Get-Process sample -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 500

$proc = Start-Process -FilePath $SampleExe -PassThru
try {
    $customBtn = $null
    for ($i = 0; $i -lt $StartupTimeoutSec; $i++) {
        Start-Sleep -Seconds 1
        if ($proc.HasExited) {
            throw "sample.exe exited during startup (code $($proc.ExitCode))"
        }
        $customBtn = Find-InProcess $proc 'Custom titlebar'
        if ($customBtn) { break }
    }
    if (-not $customBtn) {
        throw "Custom titlebar button not found within ${StartupTimeoutSec}s"
    }

    Invoke-UiButton $customBtn
    Start-Sleep -Seconds 1

    if ($proc.HasExited) {
        throw "sample.exe crashed after switching to custom titlebar"
    }

    # Titlebar close button uses aria-label="Close" after custom mode is shown.
    $closeBtn = Find-InProcess $proc 'Close'
    if (-not $closeBtn) {
        throw "Titlebar close button (Close) not found after custom titlebar mode"
    }

    Invoke-UiButton $closeBtn
    Start-Sleep -Seconds 1

    if (-not $proc.HasExited) {
        throw "sample.exe still running after titlebar close click"
    }

    Write-Host "smoke custom-titlebar: passed" -ForegroundColor Green
}
finally {
    if (-not $proc.HasExited) {
        Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
    }
}
