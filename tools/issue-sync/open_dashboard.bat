@echo off
REM CoA Issue Analyzer - serves the dashboard and opens it in the browser.
REM Re-run tools\issue-sync\analyze_issues.py after every issue-sync to refresh data.
cd /d "%~dp0"
start "" http://localhost:8765/index.html
python -m http.server 8765 --directory dashboard
