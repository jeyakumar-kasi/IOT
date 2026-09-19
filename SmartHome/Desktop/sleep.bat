@echo off
:: Force Windows to enter low-power sleep mode instantly
rundll32.exe powrprof.dll,SetSuspendState 0,1,0