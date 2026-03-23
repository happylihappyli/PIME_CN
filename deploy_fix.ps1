# 此脚本用于将修复后的文件部署到 PIME 安装目录
# 请右键点击此文件，选择“使用 PowerShell 运行” (Run with PowerShell)，或者在管理员 PowerShell 中运行

# Check for Administrator privileges and self-elevate if necessary
if (!([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Host "Requesting Administrator privileges..."
    Start-Process powershell.exe "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`"" -Verb RunAs
    exit
}

$PimeInstallDir = "C:\Program Files (x86)\PIME"
$SourceDir = "e:\GitHub3\cpp\PIME_CN"

Write-Host "正在停止 PIME 相关进程..."
Stop-Process -Name "PIMELauncher" -ErrorAction SilentlyContinue -Force
Stop-Process -Name "CppCalculator" -ErrorAction SilentlyContinue -Force
Stop-Process -Name "python" -ErrorAction SilentlyContinue -Force
taskkill /F /IM CppCalculator.exe /T 2>$null
taskkill /F /IM PIMELauncher.exe /T 2>$null
Start-Sleep -Seconds 3

Write-Host "创建目录..."
if (-not (Test-Path "$PimeInstallDir\CppCalculator")) {
    New-Item -ItemType Directory -Force -Path "$PimeInstallDir\CppCalculator"
}

Write-Host "复制 C++ 计算器..."
Copy-Item -Force "$SourceDir\CppCalculator\CppCalculator.exe" "$PimeInstallDir\CppCalculator\"

# Copy rime_ime.py
Copy-Item -Force "$SourceDir\python\input_methods\rime\rime_ime.py" "$PimeInstallDir\python\input_methods\rime\"

# Copy launcher_config.ps1
Copy-Item -Force "$SourceDir\python\input_methods\rime\launcher_config.ps1" "$PimeInstallDir\python\input_methods\rime\"

# Copy add_phrase.ps1
Copy-Item -Force "$SourceDir\python\input_methods\rime\add_phrase.ps1" "$PimeInstallDir\python\input_methods\rime\"

# Copy settings.ps1
Copy-Item -Force "$SourceDir\python\input_methods\rime\settings.ps1" "$PimeInstallDir\python\input_methods\rime\"

# Clean __pycache__ to ensure new code is loaded
$PyCacheDir = "$PimeInstallDir\python\input_methods\rime\__pycache__"
if (Test-Path $PyCacheDir) {
    Remove-Item -Recurse -Force $PyCacheDir
}

# Copy essay.txt (Rime frequency data)
# Note: Based on directory structure, it should be in brise folder
Copy-Item -Force "$SourceDir\python\input_methods\rime\brise\essay.txt" "$PimeInstallDir\python\input_methods\rime\brise\"
# Also copy to data just in case
if (-not (Test-Path "$PimeInstallDir\python\input_methods\rime\data\")) {
    New-Item -ItemType Directory -Force -Path "$PimeInstallDir\python\input_methods\rime\data\"
}
Copy-Item -Force "$SourceDir\python\input_methods\rime\brise\essay.txt" "$PimeInstallDir\python\input_methods\rime\data\"

# Copy linet_pinyin.dict.yaml (Modified weights for 'd' and 'dou')
Copy-Item -Force "$SourceDir\python\input_methods\rime\brise\preset\luna_pinyin.dict.yaml" "$PimeInstallDir\python\input_methods\rime\brise\preset\"

# ==========================================================
# Force Update User Data (Crucial for sorting fix)
# ==========================================================
$RimeUserDir = "$env:APPDATA\PIME\Rime"
if (Test-Path $RimeUserDir) {
    Write-Host "正在更新用户 Rime 数据..."
    
    # Copy modified dictionary to User Dir to override shared settings
    Copy-Item -Force "$SourceDir\python\input_methods\rime\brise\preset\luna_pinyin.dict.yaml" "$RimeUserDir\"
    
    # Copy modified essay.txt to User Dir
    Copy-Item -Force "$SourceDir\python\input_methods\rime\brise\essay.txt" "$RimeUserDir\"
    
    # Clean User Build Cache
    $UserBuildDir = "$RimeUserDir\build"
    if (Test-Path $UserBuildDir) {
        Write-Host "清理用户 Rime 构建缓存..."
        Remove-Item -Recurse -Force $UserBuildDir
    }
}

# Clean build directory in Install Dir (just in case)
$BuildDir = "$PimeInstallDir\python\input_methods\rime\build"
if (Test-Path $BuildDir) {
    Write-Host "清理安装目录 Rime 构建缓存..."
    Remove-Item -Recurse -Force $BuildDir
}

Write-Host "部署完成！正在重启 PIME..."

# Ensure PIMELauncher is in Startup (HKLM to support all users and ensure reliability)
$LauncherPath = "$PimeInstallDir\PIMELauncher.exe"
$RunKey = "HKLM:\Software\Microsoft\Windows\CurrentVersion\Run"
try {
    Write-Host "正在添加 PIME 到开机启动..."
    Set-ItemProperty -Path $RunKey -Name "PIMELauncher" -Value $LauncherPath -ErrorAction Stop
} catch {
    Write-Host "警告: 无法写入启动项到 HKLM (可能需要更高权限)。尝试写入 HKCU..." -ForegroundColor Yellow
    try {
        $RunKeyUser = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Run"
        Set-ItemProperty -Path $RunKeyUser -Name "PIMELauncher" -Value $LauncherPath -ErrorAction Stop
    } catch {
        Write-Host "错误: 无法设置开机启动。请手动将 PIMELauncher.exe 添加到启动文件夹。" -ForegroundColor Red
    }
}

# Re-register PIMETextService.dll (Fixes TSF profile/registration issues)
$DllPath = "$PimeInstallDir\PIMETextService.dll"
if (Test-Path $DllPath) {
    Write-Host "正在注册 PIMETextService.dll..."
    $reg = Start-Process -FilePath "regsvr32.exe" -ArgumentList "/s `"$DllPath`"" -PassThru -Wait
    if ($reg.ExitCode -eq 0) {
        Write-Host "DLL 注册成功。" -ForegroundColor Green
    } else {
        Write-Host "DLL 注册失败 (ExitCode: $($reg.ExitCode))。" -ForegroundColor Red
    }
} else {
    Write-Host "警告: 未找到 PIMETextService.dll，跳过注册。" -ForegroundColor Yellow
}

Start-Process "$PimeInstallDir\PIMELauncher.exe"

Write-Host "--------------------------------------------------"
Write-Host "重要提示："
Write-Host "1. 输入 ujs 后，应该会弹出一个简洁的 GUI 计算器窗口。"
Write-Host "2. 为了使词频调整生效（'d' -> '的'），你可能需要手动执行 Rime 部署："
Write-Host "   - 右键点击任务栏输入法图标"
Write-Host "   - 选择 '部署' (Deploy) 或 '重新部署'"
Write-Host "--------------------------------------------------"
Read-Host "按回车键退出..."
