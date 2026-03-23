Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

$ConfigPath = "$env:APPDATA\PIME\Rime\launcher.json"

# Load Config
if (-not (Test-Path $ConfigPath)) {
    $Config = @{
        "ugoogle" = "https://www.google.com"
        "unotepad" = "notepad.exe"
        "ucalc" = "calc.exe"
    }
} else {
    $JsonContent = Get-Content -Raw $ConfigPath -Encoding UTF8
    if ($JsonContent) {
        $Config = ConvertFrom-Json $JsonContent -AsHashtable
    } else {
        $Config = @{}
    }
}

# Convert Config to DataTable for Grid
$Table = New-Object System.Data.DataTable
$Table.Columns.Add("Trigger")
$Table.Columns.Add("Command")

foreach ($key in $Config.Keys) {
    $Table.Rows.Add($key, $Config[$key])
}

# GUI Setup
$Form = New-Object System.Windows.Forms.Form
$Form.Text = "PIME 快速启动设置"
$Form.Size = New-Object System.Drawing.Size(500, 400)
$Form.StartPosition = "CenterScreen"
$Form.TopMost = $true # Ensure it appears on top

# DataGridView
$Grid = New-Object System.Windows.Forms.DataGridView
$Grid.DataSource = $Table
$Grid.Dock = "Top"
$Grid.Height = 300
$Grid.AutoSizeColumnsMode = "Fill"
$Form.Controls.Add($Grid)

# Save Button
$SaveButton = New-Object System.Windows.Forms.Button
$SaveButton.Text = "保存配置"
$SaveButton.Location = New-Object System.Drawing.Point(150, 320)
$SaveButton.Size = New-Object System.Drawing.Size(80, 30)
$SaveButton.Add_Click({
    $NewConfig = @{}
    foreach ($row in $Table) {
        if ($row.Trigger -and $row.Command) {
            $NewConfig[$row.Trigger] = $row.Command
        }
    }
    $JsonString = $NewConfig | ConvertTo-Json -Depth 10
    $JsonString | Set-Content -Path $ConfigPath -Encoding UTF8
    [System.Windows.Forms.MessageBox]::Show("保存成功！", "PIME")
    $Form.Close()
})
$Form.Controls.Add($SaveButton)

# Cancel Button
$CancelButton = New-Object System.Windows.Forms.Button
$CancelButton.Text = "取消"
$CancelButton.Location = New-Object System.Drawing.Point(250, 320)
$CancelButton.Size = New-Object System.Drawing.Size(80, 30)
$CancelButton.Add_Click({
    $Form.Close()
})
$Form.Controls.Add($CancelButton)

# Show Dialog
$Form.ShowDialog()
