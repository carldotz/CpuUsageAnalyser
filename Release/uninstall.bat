@echo off
echo Windows Registry Editor Version 5.00 >>uninstall.reg 
echo [HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Run] >>uninstall.reg 
echo "CpuUsageAnalyser"=- >>uninstall.reg 
C:\Windows\regedit /s uninstall.reg 
del /q uninstall.reg
echo ж����ɣ����ֶ�ɾ����ǰĿ¼���ļ�
pause