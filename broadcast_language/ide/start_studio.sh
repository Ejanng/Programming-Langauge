#!/usr/bin/env bash
# Run this file to open Broadcast Studio on Ubuntu (needs Python 3 with tkinter).
#   chmod +x start_studio.sh    (once)
#   ./start_studio.sh
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if ! command -v python3 >/dev/null 2>&1; then
  echo "Python 3 was not found. Install it with:  sudo apt install python3 python3-tk"
  exit 1
fi
if ! python3 -c "import tkinter" >/dev/null 2>&1; then
  echo "tkinter is missing. Install it with:  sudo apt install python3-tk"
  exit 1
fi

nohup python3 "$here/broadcast_ide.py" >/dev/null 2>&1 &
