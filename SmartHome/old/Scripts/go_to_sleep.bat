@echo off
:: Force Windows to sleep even if background tasks are running
rundll32.exe powrprof.dll,SetSuspendState 0,1,0