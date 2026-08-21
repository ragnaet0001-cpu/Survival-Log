@echo off
cd /d %~dp0
echo ============================================
echo  SurvivalLogMod build + deploy
echo ============================================
echo [1/3] dotnet build ...
dotnet build -v:q -nologo
if errorlevel 1 (
    echo.
    echo BUILD FAILED - 请把上方错误发给我喵
    pause
    exit /b 1
)
echo [2/3] copy DLL to game ...
copy /y "bin\SurvivalLogMod.dll" "E:\Program Files (x86)\Survival Log\BepInEx\plugins\SurvivalLogMod\SurvivalLogMod.dll" >nul
if errorlevel 1 (
    echo COPY FAILED - 游戏还在运行? 请先完全退出游戏再试
    pause
    exit /b 1
)
echo [3/3] done.
echo.
echo 已编译并部署到游戏目录，重启游戏生效。
pause
