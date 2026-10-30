@echo off
rem Double-click this file to open Broadcast Studio (needs Python 3 from python.org).
where pyw >nul 2>nul
if %errorlevel%==0 (
  start "" pyw "%~dp0broadcast_ide.py"
) else (
  start "" pythonw "%~dp0broadcast_ide.py"
)
