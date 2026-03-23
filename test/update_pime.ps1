# 更新 PIMELauncher 脚本
$source = "e:\GitHub3\cpp\PIME_CN\bin\PIMELauncher.exe"
$dest = "C:\Program Files (x86)\PIME\PIMELauncher.exe"

# 结束正在运行的进程
Get-Process PIMELauncher -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1

# 复制文件
Copy-Item $source $dest -Force
Write-Host "PIMELauncher.exe 已更新到: $dest"

# 启动新的进程
Start-Process $dest
Write-Host "PIMELauncher 已启动"
