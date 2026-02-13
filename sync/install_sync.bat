@echo off
schtasks /create /tn "MicroSlate Sync" /tr "pythonw.exe \"%~dp0microslate_sync.py\"" /sc onlogon /rl limited
echo MicroSlate Sync scheduled task created (runs on login).
pause
