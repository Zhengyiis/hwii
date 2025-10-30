/**
 * Web管理器 - HTTP请求处理与API路由
 * 
 * 负责：
 * - 解析HTTP请求
 * - 路由API调用
 * - 静态文件服务
 * - 生成HTTP响应
 */

#ifndef WEB_MANAGER_H
#define WEB_MANAGER_H

#include "mysql_manager.h"
#include <string>
#include <map>
#include <vector>

/**
 * HTTP请求结构
 * 封装客户端发送的HTTP请求信息
 */
struct HttpRequest {
    std::string method;           // 请求方法：GET, POST, PUT, DELETE
    std::string path;             // 请求路径：如 /api/user/login
    std::string body;             // 请求体：POST/PUT请求的数据
    std::map<std::string, std::string> headers;  // 请求头：如 Authorization
    std::map<std::string, std::string> params;   // URL参数：如 ?category=1
};

/**
 * HTTP响应结构
 * 封装服务器返回给客户端的响应信息
 */
struct HttpResponse {
    int statusCode;               // 状态码：200成功, 404未找到, 500错误等
    std::string contentType;      // 内容类型：application/json, text/html等
    std::string body;             // 响应体：JSON数据或HTML内容
    std::map<std::string, std::string> headers;  // 响应头
};

/**
 * Web管理器类
 * 核心路由器：处理所有HTTP请求并分发到对应的处理函数
 */
class WebManager {
private:
    MySQLManager* dataManager;
    
public:
    WebManager(MySQLManager* dm);
    
    // HTTP请求处理
    HttpResponse handleRequest(const HttpRequest& request);
    
    // 静态文件服务
    HttpResponse serveStaticFile(const std::string& filePath);
    
    // API路由处理
    HttpResponse handleApiRequest(const HttpRequest& request);
    
    // 用户相关API
    HttpResponse handleUserApi(const HttpRequest& request);
    HttpResponse handleUserLogin(const HttpRequest& request);
    HttpResponse handleUserRegister(const HttpRequest& request);
    HttpResponse handleUserProfile(const HttpRequest& request);
    HttpResponse handleUserCount(const HttpRequest& request);
    
    // 物品相关API
    HttpResponse handleItemApi(const HttpRequest& request);
    HttpResponse handleGetItems(const HttpRequest& request);
    HttpResponse handleItemCount(const HttpRequest& request);
    HttpResponse handleGetItemsByOwner(const HttpRequest& request);
    HttpResponse handleGetItem(const HttpRequest& request);
    HttpResponse handleCreateItem(const HttpRequest& request);
    HttpResponse handleUpdateItem(const HttpRequest& request);
    HttpResponse handleDeleteItem(const HttpRequest& request);
    HttpResponse handleSearchItems(const HttpRequest& request);
    
    // 交易相关API
    HttpResponse handleTransactionApi(const HttpRequest& request);
    HttpResponse handleGetTransactions(const HttpRequest& request);
    HttpResponse handleCreateTransaction(const HttpRequest& request);
    HttpResponse handleUpdateTransaction(const HttpRequest& request);
    HttpResponse handleAddMessage(const HttpRequest& request);
    
    // 收藏相关API
    HttpResponse handleFavoriteApi(const HttpRequest& request);
    HttpResponse handleGetFavorites(const HttpRequest& request);
    HttpResponse handleAddFavorite(const HttpRequest& request);
    HttpResponse handleRemoveFavorite(const HttpRequest& request);
    
    // 消息相关API
    HttpResponse handleMessageApi(const HttpRequest& request);
    
    // 工具方法
    std::string parseJsonParam(const std::string& json, const std::string& key);
    std::string createJsonResponse(const std::string& data, bool success = true, const std::string& message = "");
    std::string transactionToJson(const Transaction& transaction);
    std::string urlDecode(const std::string& str);
    std::string urlEncode(const std::string& str);
    std::map<std::string, std::string> parseQueryParams(const std::string& query);
    
    // 数据转换
    std::string userToJson(const User& user);
    std::string escapeJsonString(const std::string& str);
    std::string itemToJson(const Item& item);
    std::string messageToJson(const Message& message);
    
    // 验证方法
    bool validateUserSession(const HttpRequest& request);
    std::string getCurrentUser(const HttpRequest& request);
};

#endif
