# Smoke test: custom titlebar mode — default frameless, close via titlebar button.
# [SKIP] Border drag-resize: verify manually by dragging window edges after switching to custom titlebar.
param(
    [Parameter(Mandatory = $true)]
    [string]$SampleExe,
    [int]$StartupTimeoutSec = 20
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $SampleExe)) {
    Write-Error "Sample not found: $SampleExe"
}

. "$PSScriptRoot\_ui_helpers.ps1"

Get-Process sample -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 500

$proc = Start-Process -FilePath $SampleExe -PassThru
try {
    $closeBtn = $null
    for ($i = 0; $i -lt $StartupTimeoutSec; $i++) {
        Start-Sleep -Seconds 1
        if ($proc.HasExited) {
            throw "sample.exe exited during startup (code $($proc.ExitCode))"
        }
        $closeBtn = Find-InProcess $proc 'Close'
        if ($closeBtn) { break }
    }
    if (-not $closeBtn) {
        throw "Titlebar close button (Close) not found within ${StartupTimeoutSec}s — expected default frame: false"
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
