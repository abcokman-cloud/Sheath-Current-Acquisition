@echo off
cd /d "e:\desktop20260319\GIUTHUB\1_SheathCirculatingCurrent\Sheath-Current-Acquisition"
echo Starting Dash app on http://127.0.0.1:8050/
echo Keep this window open while using the page.
python ".\2_Python_Companion\circulating_current.py"
echo.
echo The app has stopped. Press any key to close this window.
pause >nul
