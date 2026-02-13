@echo off
schtasks /delete /tn "MicroSlate Sync" /f
echo MicroSlate Sync scheduled task removed.
pause
