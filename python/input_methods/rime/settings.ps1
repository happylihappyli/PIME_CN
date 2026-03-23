param (
    [string]$UserDir
)

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

# Set DPI awareness to avoid blurry text on high DPI screens
try {
    $source = @"
using System;
using System.Runtime.InteropServices;
public class Win32DPI {
    [DllImport("user32.dll")]
    public static extern bool SetProcessDPIAware();
}
"@
    Add-Type -TypeDefinition $source -Language CSharp
    [Win32DPI]::SetProcessDPIAware()
} catch {
    # Ignore if already set or failed
}

$form = New-Object System.Windows.Forms.Form
$form.Text = "PIME 设置"
$form.Size = New-Object System.Drawing.Size(350, 300)
$form.StartPosition = "CenterScreen"
$form.Topmost = $true
$form.FormBorderStyle = [System.Windows.Forms.FormBorderStyle]::FixedDialog
$form.MaximizeBox = $false
$form.MinimizeBox = $false

# Icon (Optional, try to load from system)
# $form.Icon = [System.Drawing.Icon]::ExtractAssociatedIcon($PSCommandPath)

# Title
$labelTitle = New-Object System.Windows.Forms.Label
$labelTitle.Text = "PIME 输入法设置"
$labelTitle.Font = New-Object System.Drawing.Font("Microsoft YaHei UI", 14, [System.Drawing.FontStyle]::Bold)
$labelTitle.Location = New-Object System.Drawing.Point(0, 20)
$labelTitle.Size = New-Object System.Drawing.Size(350, 40)
$labelTitle.TextAlign = "MiddleCenter"
$form.Controls.Add($labelTitle)

# Helper to create styled buttons
function New-StyledButton {
    param ($text, $y, $action)
    $btn = New-Object System.Windows.Forms.Button
    $btn.Text = $text
    $btn.Font = New-Object System.Drawing.Font("Microsoft YaHei UI", 10)
    $btn.Location = New-Object System.Drawing.Point(50, $y)
    $btn.Size = New-Object System.Drawing.Size(240, 40)
    $btn.Cursor = [System.Windows.Forms.Cursors]::Hand
    $btn.Add_Click($action)
    return $btn
}

# Button: Add Vocabulary
$btnAdd = New-StyledButton -text "添加词汇 (Add Vocabulary)" -y 80 -action {
    $scriptPath = Join-Path (Split-Path $MyInvocation.MyCommand.Path) "add_phrase.ps1"
    Start-Process powershell.exe -ArgumentList "-ExecutionPolicy Bypass -WindowStyle Hidden -File `"$scriptPath`" -UserDir `"$UserDir`""
}
$form.Controls.Add($btnAdd)

# Button: Launcher Config
$btnLauncher = New-StyledButton -text "快速启动设置 (Custom Launcher)" -y 130 -action {
    $scriptPath = Join-Path (Split-Path $MyInvocation.MyCommand.Path) "launcher_config.ps1"
    Start-Process powershell.exe -ArgumentList "-ExecutionPolicy Bypass -WindowStyle Hidden -File `"$scriptPath`""
}
$form.Controls.Add($btnLauncher)

# Button: Original Settings
$btnOrig = New-StyledButton -text "更多设置 (More Settings)" -y 180 -action {
    $source = @"
using System;
using System.Runtime.InteropServices;
public class Win32Msg {
    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern IntPtr FindWindow(string lpClassName, string lpWindowName);
    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
}
"@
    # Only add type if not already added
    if (-not ([System.Management.Automation.PSTypeName]'Win32Msg').Type) {
        Add-Type -TypeDefinition $source -Language CSharp
    }
    
    $hwnd = [Win32Msg]::FindWindow("PIMELauncherWnd", $null)
    if ($hwnd -ne [IntPtr]::Zero) {
        [Win32Msg]::PostMessage($hwnd, 0x0111, [IntPtr]1003, [IntPtr]::Zero)
    } else {
        [System.Windows.Forms.MessageBox]::Show("未找到 PIME 主程序窗口。", "PIME", [System.Windows.Forms.MessageBoxButtons]::OK, [System.Windows.Forms.MessageBoxIcon]::Warning)
    }
    $form.Close()
}
$form.Controls.Add($btnOrig)

# Close Button (X is enough, but maybe a close button at bottom?)
# Let's keep it simple.

$form.ShowDialog()
