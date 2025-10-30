@echo off
chcp 65001 > nul
echo =========================================
echo    校园绿色物品共享与循环交易系统
echo            Web版本服务器
echo =========================================
echo.

REM 检查Web服务器程序是否存在
if not exist "campus_trading_web_server.exe" (
    echo 错误: 未找到Web服务器程序！
    echo 请先运行 compile_web.bat 编译程序
    pause
    exit /b 1
)

echo 正在启动Web服务器...
echo.
echo 服务器信息:
echo - 端口: 8080
echo - 访问地址: http://localhost:8080
echo - 按 Ctrl+C 停止服务器
echo.
echo =========================================
echo.

REM 启动Web服务器
campus_trading_web_server.exe

echo.
echo 服务器已停止
pause
