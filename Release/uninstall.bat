@echo off
echo Windows Registry Editor Version 5.00 >>uninstall.reg 
echo [HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Run] >>uninstall.reg 
echo "CpuUsageAnalyser"=- >>uninstall.reg 
C:\Windows\regedit /s uninstall.reg 
del /q uninstall.reg
echo 卸载完成，请手动删除当前目录下文件
pause