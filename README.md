# 校园绿色物品共享与循环交易系统

基于C++和MySQL的校园二手物品交易平台Web版本。

## 🎯 系统功能

- ✅ 用户注册/登录
- ✅ 物品发布（支持图片上传）
- ✅ 物品浏览、搜索、筛选
- ✅ 交易管理（创建、确认、完成、取消）
- ✅ 买卖双方留言功能
- ✅ 物品收藏
- ✅ 个人主页（我的物品、交易、收藏）

## 🛠️ 技术栈

- **后端**: C++17, MySQL 8.0, Winsock2
- **前端**: HTML5, CSS3, JavaScript
- **数据库**: MySQL 8.0
- **编译器**: MinGW-w64 (g++)

## 📋 环境要求

- Windows 10/11
- MySQL Server 8.0
- MinGW-w64 (g++ 支持C++17)

## 🚀 快速开始

### 1. 安装MySQL

下载并安装 [MySQL Server 8.0](https://dev.mysql.com/downloads/mysql/)

### 2. 创建数据库

```bash
mysql -u root -p < database_schema.sql
```

或在MySQL命令行中：
```sql
source database_schema.sql
```

### 3. 配置数据库连接

编辑 `simple_web_server.cpp` 第29行，修改MySQL连接参数：

```cpp
MySQLManager dataManager("localhost", "root", "你的密码", "campus_trading");
```

### 4. 编译程序

```bash
compile_web_mysql.bat
```

### 5. 启动服务器

```bash
run_web.bat
```

### 6. 访问系统

在浏览器中打开：http://localhost:8080

## 📂 项目结构

```
├── simple_web_server.cpp   # Web服务器主程序
├── web_manager.h/cpp        # HTTP请求处理
├── mysql_manager.h/cpp      # MySQL数据库操作
├── user.h/cpp               # 用户类
├── item.h/cpp               # 物品类
├── transaction.h/cpp        # 交易和消息类
├── database_schema.sql      # 数据库结构
├── compile_web_mysql.bat    # 编译脚本
├── run_web.bat              # 启动脚本
├── libmysql.dll             # MySQL动态库
└── web/                     # 前端文件
    ├── index.html
    └── static/
        ├── css/style.css
        ├── js/app.js
        └── uploads/         # 图片上传目录
```

## 🗄️ 数据库表

系统使用5个数据表：

1. **users** - 用户信息（用户名、密码、邮箱、电话、学号、信誉）
2. **items** - 物品信息（标题、描述、价格、分类、状态、图片）
3. **transactions** - 交易记录（买家、卖家、物品、价格、状态）
4. **messages** - 留言记录（交易留言、已读状态）
5. **favorites** - 收藏记录（用户收藏的物品）

## 🔧 常见问题

### Q: 编译失败，找不到mysql.h
**A**: 确保MySQL安装在：`C:\Program Files\MySQL\MySQL Server 8.0`

### Q: 运行时提示"MySQL连接失败"
**A**: 
1. 检查MySQL服务是否启动
2. 确认数据库已创建
3. 检查用户名密码是否正确

### Q: 找不到libmysql.dll
**A**: 复制dll文件到项目目录：
```bash
copy "C:\Program Files\MySQL\MySQL Server 8.0\lib\libmysql.dll" .
```

### Q: 端口8080被占用
**A**: 修改 `simple_web_server.cpp` 第275行的端口号

## 🔒 安全说明

**注意**: 本系统为课程设计项目，部分安全措施已实现：

- ✅ SQL注入防护（参数化查询）
- ✅ 外键约束保证数据完整性
- ⚠️ 密码建议生产环境加密存储
- ⚠️ 建议添加HTTPS支持

## 📞 技术支持

如果遇到问题：

1. 检查MySQL服务是否启动
2. 查看控制台错误信息
3. 确认所有依赖文件齐全
4. 检查防火墙设置

## 📄 许可证

本项目为课程设计作业，仅供学习交流使用。

---

**版本**: 1.0  
**更新日期**: 2024  
**技术架构**: C++ + MySQL + Web

