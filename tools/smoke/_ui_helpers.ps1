Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes

function Get-ProcWindow($proc) {
    $root = [System.Windows.Automation.AutomationElement]::RootElement
    $cond = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::ProcessIdProperty, $proc.Id)
    return $root.FindFirst([System.Windows.Automation.TreeScope]::Children, $cond)
}

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
