@echo off
chcp 65001 > nul
echo =========================================
echo    校园绿色物品共享与循环交易系统
echo         Web服务器 - MySQL版本
echo =========================================
echo.

REM 检查MySQL安装
if not exist "C:\Program Files\MySQL\MySQL Server 8.0\include\mysql.h" (
    echo 错误: 未找到MySQL C API头文件！
    echo 请确保MySQL Server 8.0已正确安装
    echo 默认路径: C:\Program Files\MySQL\MySQL Server 8.0
    pause
    exit /b 1
)

REM 停止可能正在运行的旧服务器
echo 停止旧的服务器进程...
taskkill /IM campus_trading_web_server.exe /F 2>nul

echo 正在编译Web服务器（MySQL版本）...
echo.

REM 编译命令
g++ -std=c++17 -I. ^
    -I"C:\Program Files\MySQL\MySQL Server 8.0\include" ^
    -L"C:\Program Files\MySQL\MySQL Server 8.0\lib" ^
    -o campus_trading_web_server.exe ^
    simple_web_server.cpp ^
    web_manager.cpp ^
    mysql_manager.cpp ^
    item.cpp ^
    transaction.cpp ^
    user.cpp ^
    -lws2_32 -lmysql

if %errorlevel% equ 0 (
    echo.
    echo =========================================
    echo 编译成功！
    echo =========================================
    echo.
    echo 使用说明:
    echo 1. 确保MySQL服务已启动
    echo 2. 运行以下命令创建数据库:
    echo    mysql -u root -p ^< database_schema.sql
    echo 3. 运行 run_web.bat 启动服务器
    echo.
    echo 更多信息请参考: MySQL迁移指南.md
    echo =========================================
    echo.
) else (
    echo.
    echo =========================================
    echo 编译失败！
    echo =========================================
    echo.
    echo 可能的原因:
    echo 1. MySQL未正确安装
    echo 2. MySQL路径不正确（检查是否为8.0版本）
    echo 3. 缺少必要的源文件
    echo 4. g++编译器未安装或未配置
    echo.
    echo 如果MySQL安装在其他位置，请修改此脚本中的路径
    echo =========================================
    echo.
)

pause

