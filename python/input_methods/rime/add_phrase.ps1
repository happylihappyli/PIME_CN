param (
    [string]$UserDir
)

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

$form = New-Object System.Windows.Forms.Form
$form.Text = "添加词汇"
$form.Size = New-Object System.Drawing.Size(300, 250)
$form.StartPosition = "CenterScreen"
$form.Topmost = $true
$form.FormBorderStyle = [System.Windows.Forms.FormBorderStyle]::FixedDialog
$form.MaximizeBox = $false

$labelPhrase = New-Object System.Windows.Forms.Label
$labelPhrase.Location = New-Object System.Drawing.Point(10, 20)
$labelPhrase.Size = New-Object System.Drawing.Size(280, 20)
$labelPhrase.Text = "词汇 (Phrase):"
$form.Controls.Add($labelPhrase)

$txtPhrase = New-Object System.Windows.Forms.TextBox
$txtPhrase.Location = New-Object System.Drawing.Point(10, 40)
$txtPhrase.Size = New-Object System.Drawing.Size(260, 20)
$form.Controls.Add($txtPhrase)

$labelCode = New-Object System.Windows.Forms.Label
$labelCode.Location = New-Object System.Drawing.Point(10, 70)
$labelCode.Size = New-Object System.Drawing.Size(280, 20)
$labelCode.Text = "拼音 (Code):"
$form.Controls.Add($labelCode)

$txtCode = New-Object System.Windows.Forms.TextBox
$txtCode.Location = New-Object System.Drawing.Point(10, 90)
$txtCode.Size = New-Object System.Drawing.Size(260, 20)
$form.Controls.Add($txtCode)

$labelWeight = New-Object System.Windows.Forms.Label
$labelWeight.Location = New-Object System.Drawing.Point(10, 120)
$labelWeight.Size = New-Object System.Drawing.Size(280, 20)
$labelWeight.Text = "权重 (Weight, default 100):"
$form.Controls.Add($labelWeight)

$txtWeight = New-Object System.Windows.Forms.TextBox
$txtWeight.Location = New-Object System.Drawing.Point(10, 140)
$txtWeight.Size = New-Object System.Drawing.Size(260, 20)
$txtWeight.Text = "100"
$form.Controls.Add($txtWeight)

$btnOk = New-Object System.Windows.Forms.Button
$btnOk.Location = New-Object System.Drawing.Point(50, 180)
$btnOk.Size = New-Object System.Drawing.Size(80, 25)
$btnOk.Text = "添加"
$btnOk.DialogResult = [System.Windows.Forms.DialogResult]::OK
$form.Controls.Add($btnOk)

$btnCancel = New-Object System.Windows.Forms.Button
$btnCancel.Location = New-Object System.Drawing.Point(150, 180)
$btnCancel.Size = New-Object System.Drawing.Size(80, 25)
$btnCancel.Text = "取消"
$btnCancel.DialogResult = [System.Windows.Forms.DialogResult]::Cancel
$form.Controls.Add($btnCancel)

$form.AcceptButton = $btnOk
$form.CancelButton = $btnCancel

$result = $form.ShowDialog()

if ($result -eq [System.Windows.Forms.DialogResult]::OK) {
    $phrase = $txtPhrase.Text.Trim()
    $code = $txtCode.Text.Trim()
    $weight = $txtWeight.Text.Trim()

    if (-not [string]::IsNullOrEmpty($phrase) -and -not [string]::IsNullOrEmpty($code)) {
        if ([string]::IsNullOrEmpty($UserDir)) {
            $UserDir = "$env:APPDATA\PIME\Rime"
        }
        
        if (-not (Test-Path $UserDir)) {
            New-Item -ItemType Directory -Force -Path $UserDir
        }

        $customPhraseFile = Join-Path $UserDir "custom_phrase.txt"
        
        # Format: text code weight
        $line = "$phrase`t$code`t$weight"
        
        try {
            Add-Content -Path $customPhraseFile -Value $line -Encoding UTF8
            [System.Windows.Forms.MessageBox]::Show("已添加: $phrase ($code)`n请重新部署输入法以生效。", "成功", [System.Windows.Forms.MessageBoxButtons]::OK, [System.Windows.Forms.MessageBoxIcon]::Information)
        }
        catch {
             [System.Windows.Forms.MessageBox]::Show("写入文件失败: $_", "错误", [System.Windows.Forms.MessageBoxButtons]::OK, [System.Windows.Forms.MessageBoxIcon]::Error)
        }
    }
}
