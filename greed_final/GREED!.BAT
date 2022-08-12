@echo off
if exist setup.cfg goto :run

:setup
 setup.exe
:run 
 greed.exe

