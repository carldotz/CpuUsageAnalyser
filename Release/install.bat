@echo off
set filename=CpuUsageAnalyser.exe
set path=%~dp0%filename%
set path=%path:\=\\%
echo Windows Registry Editor Version 5.00>>install.reg 
echo. 
echo [HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Run]>>install.reg 
echo "CpuUsageAnalyser"="\"%path%\"">>install.reg 
C:\Windows\regedit /s install.reg 
del /q install.reg
echo 安装完成，程序将在开机自动后台运行！本次请手动运行！
echo 检查程序是否运行请使用任务管理器，查找进程CpuUsageAnalyser。
echo 日志文件为当前目录下CpuUsage.log
echo 卸载程序请运行uninstall.bat
pause