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
echo ��װ��ɣ������ڿ����Զ���̨���У��������ֶ����У�
echo �������Ƿ�������ʹ����������������ҽ���CpuUsageAnalyser��
echo ��־�ļ�Ϊ��ǰĿ¼��CpuUsage.log
echo ж�س���������uninstall.bat
pause