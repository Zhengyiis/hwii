#include "web_manager.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <regex>
#include <iomanip>

WebManager::WebManager(MySQLManager* dm) : dataManager(dm) {}

// HTTP请求处理
HttpResponse WebManager::handleRequest(const HttpRequest& request) {
    HttpResponse response;
    response.statusCode = 200;
    response.contentType = "text/html; charset=utf-8";
    
    // 处理OPTIONS请求（CORS预检）
    if (request.method == "OPTIONS") {
        response.statusCode = 200;
        response.body = "";
        return response;
    }
    
    // 路由处理
    if (request.path == "/" || request.path == "/index.html") {
        return serveStaticFile("web/index.html");
    } else if (request.path.find("/api/") == 0) {
        return handleApiRequest(request);
    } else if (request.path.find("/static/") == 0) {
        return serveStaticFile("web" + request.path);
    } else {
        // 尝试作为静态文件处理
        return serveStaticFile("web" + request.path);
    }
}

// 静态文件服务
HttpResponse WebManager::serveStaticFile(const std::string& filePath) {
    HttpResponse response;
    
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        response.statusCode = 404;
        response.contentType = "text/html; charset=utf-8";
        response.body = "<html><body><h1>404 - 文件未找到</h1></body></html>";
        return response;
    }
    
    // 根据文件扩展名设置内容类型
    if (filePath.find(".html") != std::string::npos) {
        response.contentType = "text/html; charset=utf-8";
    } else if (filePath.find(".css") != std::string::npos) {
        response.contentType = "text/css";
    } else if (filePath.find(".js") != std::string::npos) {
        response.contentType = "application/javascript";
    } else if (filePath.find(".json") != std::string::npos) {
        response.contentType = "application/json";
    } else {
        response.contentType = "application/octet-stream";
    }
    
    // 读取文件内容
    std::ostringstream buffer;
    buffer << file.rdbuf();
    response.body = buffer.str();
    
    response.statusCode = 200;
    return response;
}

// API路由处理 - 根据路径分发到相应的处理函数
HttpResponse WebManager::handleApiRequest(const HttpRequest& request) {
    HttpResponse response;
    response.contentType = "application/json; charset=utf-8";
    
    // 移除路径前缀 "/api/"
    std::string path = request.path.substr(5);
    
    // 路由分发：根据API类型调用对应的处理函数
    if (path.find("user") == 0) {
        return handleUserApi(request);
    } else if (path.find("item") == 0) {
        return handleItemApi(request);
    } else if (path.find("transaction") == 0) {
        return handleTransactionApi(request);
    } else if (path.find("favorite") == 0) {
        return handleFavoriteApi(request);
    } else if (path.find("message") == 0) {
        return handleMessageApi(request);
    } else {
        response.statusCode = 404;
        response.body = createJsonResponse("", false, "API接口不存在");
        return response;
    }
}

// 用户相关API
HttpResponse WebManager::handleUserApi(const HttpRequest& request) {
    HttpResponse response;
    response.contentType = "application/json; charset=utf-8";
    
    std::string path = request.path.substr(9); // 移除 "/api/user"
    
    if (path == "/login" && request.method == "POST") {
        return handleUserLogin(request);
    } else if (path == "/register" && request.method == "POST") {
        return handleUserRegister(request);
    } else if (path == "/profile" && request.method == "GET") {
        return handleUserProfile(request);
    } else if (path == "/count" && request.method == "GET") {
        return handleUserCount(request);
    } else {
        response.statusCode = 404;
        response.body = createJsonResponse("", false, "用户API接口不存在");
        return response;
    }
}

HttpResponse WebManager::handleUserLogin(const HttpRequest& request) {
    std::string username = parseJsonParam(request.body, "username");
    std::string password = parseJsonParam(request.body, "password");
    
    if (username.empty() || password.empty()) {
        HttpResponse response;
        response.statusCode = 400;
        response.contentType = "application/json";
        response.body = createJsonResponse("", false, "用户名和密码不能为空");
        return response;
    }
    
    if (dataManager->loginUser(username, password)) {
        HttpResponse response;
        response.statusCode = 200;
        response.contentType = "application/json";
        response.body = createJsonResponse(userToJson(*dataManager->getCurrentUser()), true, "登录成功");
        return response;
    } else {
        HttpResponse response;
        response.statusCode = 401;
        response.contentType = "application/json";
        response.body = createJsonResponse("", false, "用户名或密码错误");
        return response;
    }
}

HttpResponse WebManager::handleUserRegister(const HttpRequest& request) {
    std::string username = parseJsonParam(request.body, "username");
    std::string password = parseJsonParam(request.body, "password");
    std::string email = parseJsonParam(request.body, "email");
    std::string phone = parseJsonParam(request.body, "phone");
    std::string studentId = parseJsonParam(request.body, "studentId");
    
    if (username.empty() || password.empty() || email.empty() || phone.empty() || studentId.empty()) {
        HttpResponse response;
        response.statusCode = 400;
        response.contentType = "application/json";
        response.body = createJsonResponse("", false, "所有字段都必须填写");
        return response;
    }
    
    if (dataManager->registerUser(username, password, email, phone, studentId)) {
        HttpResponse response;
        response.statusCode = 200;
        response.contentType = "application/json";
        response.body = createJsonResponse("", true, "注册成功");
        return response;
    } else {
        HttpResponse response;
        response.statusCode = 400;
        response.contentType = "application/json";
        response.body = createJsonResponse("", false, "注册失败，用户名可能已存在");
        return response;
    }
}

HttpResponse WebManager::handleUserProfile(const HttpRequest& request) {
    if (!validateUserSession(request)) {
        HttpResponse response;
        response.statusCode = 401;
        response.contentType = "application/json";
        response.body = createJsonResponse("", false, "未登录");
        return response;
    }
    
    User* user = dataManager->getCurrentUser();
    if (user) {
        HttpResponse response;
        response.statusCode = 200;
        response.contentType = "application/json";
        response.body = createJsonResponse(userToJson(*user), true, "获取用户信息成功");
        return response;
    } else {
        HttpResponse response;
        response.statusCode = 404;
        response.contentType = "application/json";
        response.body = createJsonResponse("", false, "用户不存在");
        return response;
    }
}

// 物品相关API
HttpResponse WebManager::handleItemApi(const HttpRequest& request) {
    HttpResponse response;
    response.contentType = "application/json; charset=utf-8";
    
    std::string path = request.path.substr(9); // 移除 "/api/item"
    
    if (path == "" && request.method == "GET") {
        return handleGetItems(request);
    } else if (path == "/count" && request.method == "GET") {
        return handleItemCount(request);
    } else if (path.find("/owner/") == 0 && request.method == "GET") {
        return handleGetItemsByOwner(request);
    } else if (path.find("/") == 0 && request.method == "GET") {
        return handleGetItem(request);
    } else if (path == "" && request.method == "POST") {
        return handleCreateItem(request);
    } else if (path.find("/") == 0 && request.method == "PUT") {
        return handleUpdateItem(request);
    } else if (path.find("/") == 0 && request.method == "DELETE") {
        return handleDeleteItem(request);
    } else if (path == "/search" && request.method == "GET") {
        return handleSearchItems(request);
    } else {
        response.statusCode = 404;
        response.body = createJsonResponse("", false, "物品API接口不存在");
        return response;
    }
}

HttpResponse WebManager::handleGetItems(const HttpRequest& request) {
    // 检查是否有筛选参数
    auto categoryIt = request.params.find("category");
    auto minPriceIt = request.params.find("minPrice");
    auto maxPriceIt = request.params.find("maxPrice");
    auto startTimeIt = request.params.find("startTime");
    auto endTimeIt = request.params.find("endTime");
    auto statusIt = request.params.find("status");
    
    std::vector<Item> items;
    
    if (categoryIt != request.params.end() || 
        minPriceIt != request.params.end() || 
        maxPriceIt != request.params.end() ||
        startTimeIt != request.params.end() || 
        endTimeIt != request.params.end() ||
        statusIt != request.params.end()) {
        
        // 使用筛选查询
        std::string category = (categoryIt != request.params.end()) ? categoryIt->second : "";
        double minPrice = (minPriceIt != request.params.end()) ? std::stod(minPriceIt->second) : -1;
        double maxPrice = (maxPriceIt != request.params.end()) ? std::stod(maxPriceIt->second) : -1;
        time_t startTime = (startTimeIt != request.params.end()) ? std::stoll(startTimeIt->second) : -1;
        time_t endTime = (endTimeIt != request.params.end()) ? std::stoll(endTimeIt->second) : -1;
        std::string status = (statusIt != request.params.end()) ? statusIt->second : "";
        
        items = dataManager->getItemsWithFilters(category, minPrice, maxPrice, startTime, endTime, status);
    } else {
        // 获取所有物品
        items = dataManager->getAllItems();
    }
    
    std::string jsonItems = "[";
    for (size_t i = 0; i < items.size(); i++) {
        jsonItems += itemToJson(items[i]);
        if (i < items.size() - 1) jsonItems += ",";
    }
    jsonItems += "]";
    
    HttpResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = createJsonResponse(jsonItems, true, "获取物品列表成功");
    return response;
}

HttpResponse WebManager::handleGetItemsByOwner(const HttpRequest& request) {
    if (!validateUserSession(request)) {
        HttpResponse response;
        response.statusCode = 401;
        response.contentType = "application/json";
        response.body = createJsonResponse("", false, "未登录");
        return response;
    }
    
    std::string path = request.path;
    std::string username;
    
    // 查找/owner/后的用户名
    size_t ownerPos = path.find("/owner/");
    if (ownerPos != std::string::npos) {
        username = path.substr(ownerPos + 7); // 7是"/owner/"的长度
    }
    if (username.empty()) {
        HttpResponse response;
        response.statusCode = 400;
        response.contentType = "application/json";
        response.body = createJsonResponse("", false, "用户名不能为空");
        return response;
    }
    std::vector<Item> items = dataManager->getItemsByOwner(username);
    
    std::string jsonItems = "[";
    for (size_t i = 0; i < items.size(); i++) {
        jsonItems += itemToJson(items[i]);
        if (i < items.size() - 1) jsonItems += ",";
    }
    jsonItems += "]";
    
    HttpResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = createJsonResponse(jsonItems, true, "获取我的物品成功");
    return response;
}

HttpResponse WebManager::handleGetItem(const HttpRequest& request) {
    // 从路径中提取物品ID，路径格式为 /api/item/1
    std::string path = request.path;
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash == std::string::npos || lastSlash + 1 >= path.length()) {
        HttpResponse response;
        response.statusCode = 400;
        response.contentType = "application/json";
        response.body = createJsonResponse("", false, "无效的物品ID");
        return response;
    }
    
    std::string itemIdStr = path.substr(lastSlash + 1);
    int itemId = std::stoi(itemIdStr);
    
    Item* item = dataManager->getItemById(itemId);
    if (item) {
        HttpResponse response;
        response.statusCode = 200;
        response.contentType = "application/json";
        response.body = createJsonResponse(itemToJson(*item), true, "获取物品详情成功");
        return response;
    } else {
        HttpResponse response;
        response.statusCode = 404;
        response.contentType = "application/json";
        response.body = createJsonResponse("", false, "物品不存在");
        return response;
    }
}

HttpResponse WebManager::handleCreateItem(const HttpRequest& request) {
    if (!validateUserSession(request)) {
        HttpResponse response;
        response.statusCode = 401;
        response.contentType = "application/json";
        response.body = createJsonResponse("", false, "未登录");
        return response;
    }
    
    std::string title = parseJsonParam(request.body, "title");
    std::string description = parseJsonParam(request.body, "description");
    std::string priceStr = parseJsonParam(request.body, "price");
    std::string categoryStr = parseJsonParam(request.body, "category");
    std::string condition = parseJsonParam(request.body, "condition");
    std::string imagePath = parseJsonParam(request.body, "imagePath");
    
    if (title.empty() || description.empty() || priceStr.empty() || categoryStr.empty() || condition.empty()) {
        HttpResponse response;
        response.statusCode = 400;
        response.contentType = "application/json";
        response.body = createJsonResponse("", false, "必填字段不能为空");
        return response;
    }
    
    double price = std::stod(priceStr);
    ItemCategory category = static_cast<ItemCategory>(std::stoi(categoryStr));
    
    // 获取当前用户名
    std::string username = getCurrentUser(request);
    
    int itemId = dataManager->addItem(title, description, price, category, condition, imagePath, username);
    if (itemId > 0) {
        HttpResponse response;
        response.statusCode = 200;
        response.contentType = "application/json";
        response.body = createJsonResponse(std::to_string(itemId), true, "物品发布成功");
        return response;
    } else {
        HttpResponse response;
        response.statusCode = 400;
        response.contentType = "application/json";
        response.body = createJsonResponse("", false, "物品发布失败");
        return response;
    }
}

HttpResponse WebManager::handleUpdateItem(const HttpRequest& request) {
    if (!validateUserSession(request)) {
        HttpResponse response;
        response.statusCode = 401;
        response.contentType = "application/json";
        response.body = createJsonResponse("", false, "未登录");
        return response;
    }
    
    // 从路径中提取物品ID
    std::string path = request.path;
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash == std::string::npos || lastSlash + 1 >= path.length()) {
        HttpResponse response;
        response.statusCode = 400;
        response.contentType = "application/json";
        response.body = createJsonResponse("", false, "无效的物品ID");
        return response;
    }
    
    std::string itemIdStr = path.substr(lastSlash + 1);
    int itemId = std::stoi(itemIdStr);
    
    // 检查物品是否存在
    Item* item = dataManager->getItemById(itemId);
    if (!item) {
        HttpResponse response;
        response.statusCode = 404;
        response.contentType = "application/json";
        response.body = createJsonResponse("", false, "物品不存在");
        return response;
    }
    
    std::string title = parseJsonParam(request.body, "title");
    std::string description = parseJsonParam(request.body, "description");
    std::string priceStr = parseJsonParam(request.body, "price");
    std::string categoryStr = parseJsonParam(request.body, "category");
    std::string condition = parseJsonParam(request.body, "condition");
    std::string imagePath = parseJsonParam(request.body, "imagePath");
    
    if (title.empty() || description.empty() || priceStr.empty() || categoryStr.empty() || condition.empty()) {
        HttpResponse response;
        response.statusCode = 400;
        response.contentType = "application/json";
        response.body = createJsonResponse("", false, "必填字段不能为空");
        return response;
    }
    
    double price = std::stod(priceStr);
    ItemCategory category = static_cast<ItemCategory>(std::stoi(categoryStr));
    
    // 更新物品信息
    item->setTitle(title);
    item->setDescription(description);
    item->setPrice(price);
    item->setCategory(category);
    item->setCondition(condition);
    item->setImagePath(imagePath);
    
    // 保存到MySQL数据库
    bool success = dataManager->updateItem(*item);
    delete item;  // 释放内存
    
    HttpResponse response;
    response.statusCode = success ? 200 : 500;
    response.contentType = "application/json";
    response.body = createJsonResponse("", success, success ? "物品信息更新成功" : "更新失败");
    return response;
}

HttpResponse WebManager::handleDeleteItem(const HttpRequest& request) {
    if (!validateUserSession(request)) {
        HttpResponse response;
        response.statusCode = 401;
        response.contentType = "application/json";
        response.body = createJsonResponse("", false, "未登录");
        return response;
    }
    
    // 从路径中提取物品ID
    std::string path = request.path;
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash == std::string::npos || lastSlash + 1 >= path.length()) {
        HttpResponse response;
        response.statusCode = 400;
        response.contentType = "application/json";
        response.body = createJsonResponse("", false, "无效的物品ID");
        return response;
    }
    
    std::string itemIdStr = path.substr(lastSlash + 1);
    int itemId = std::stoi(itemIdStr);
    
    // 检查物品是否存在
    Item* item = dataManager->getItemById(itemId);
    if (!item) {
        HttpResponse response;
        response.statusCode = 404;
        response.contentType = "application/json";
        response.body = createJsonResponse("", false, "物品不存在");
        return response;
    }
    
    // 删除物品
    if (dataManager->deleteItem(itemId)) {
        HttpResponse response;
        response.statusCode = 200;
        response.contentType = "application/json";
        response.body = createJsonResponse("", true, "物品删除成功");
        return response;
    } else {
        HttpResponse response;
        response.statusCode = 500;
        response.contentType = "application/json";
        response.body = createJsonResponse("", false, "物品删除失败");
        return response;
    }
}

HttpResponse WebManager::handleSearchItems(const HttpRequest& request) {
    auto it = request.params.find("keyword");
    std::string keyword = (it != request.params.end()) ? it->second : "";
    if (keyword.empty()) {
        HttpResponse response;
        response.statusCode = 400;
        response.contentType = "application/json";
        response.body = createJsonResponse("", false, "搜索关键词不能为空");
        return response;
    }
    
    std::vector<Item> items = dataManager->searchItems(keyword);
    std::string jsonItems = "[";
    for (size_t i = 0; i < items.size(); i++) {
        jsonItems += itemToJson(items[i]);
        if (i < items.size() - 1) jsonItems += ",";
    }
    jsonItems += "]";
    
    HttpResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = createJsonResponse(jsonItems, true, "搜索完成");
    return response;
}

// 交易相关API
// 交易相关API处理
HttpResponse WebManager::handleTransactionApi(const HttpRequest& request) {
    // 交易计数API是公开的，不需要登录验证（用于前端显示统计信息）
    if (request.method == "GET") {
        std::string path = request.path.substr(16); // 移除 "/api/transaction" (16个字符)
        
        if (path == "/count") {
            // 获取交易总数统计
            int transactionCount = dataManager->getTransactionCount();
            HttpResponse response;
            response.statusCode = 200;
            response.contentType = "application/json";
            response.body = createJsonResponse(std::to_string(transactionCount), true, "获取交易总数成功");
            return response;
        }
    }
    
    // 其他交易API需要登录验证（保护用户隐私）
    if (!validateUserSession(request)) {
        HttpResponse response;
        response.statusCode = 401;
        response.contentType = "application/json";
        response.body = createJsonResponse("", false, "未登录");
        return response;
    }
    
    std::string username = getCurrentUser(request);
    
    if (request.method == "GET") {
        // 获取用户的所有交易列表（作为买家或卖家）
        std::vector<Transaction> transactions = dataManager->getUserTransactions(username);
        
        std::string jsonTransactions = "[";
        for (size_t i = 0; i < transactions.size(); i++) {
            jsonTransactions += transactionToJson(transactions[i]);
            if (i < transactions.size() - 1) jsonTransactions += ",";
        }
        jsonTransactions += "]";
        
        HttpResponse response;
        response.statusCode = 200;
        response.contentType = "application/json";
        response.body = createJsonResponse(jsonTransactions, true, "获取交易列表成功");
        return response;
    } else if (request.method == "POST") {
        // 创建交易
        std::string itemIdStr = parseJsonParam(request.body, "itemId");
        if (itemIdStr.empty()) {
            HttpResponse response;
            response.statusCode = 400;
            response.contentType = "application/json";
            response.body = createJsonResponse("", false, "物品ID不能为空");
            return response;
        }
        
        int itemId = std::stoi(itemIdStr);
        Item* item = dataManager->getItemById(itemId);
        if (!item) {
            HttpResponse response;
            response.statusCode = 404;
            response.contentType = "application/json";
            response.body = createJsonResponse("", false, "物品不存在");
            return response;
        }
        
        if (item->getStatus() != ItemStatus::AVAILABLE) {
            HttpResponse response;
            response.statusCode = 400;
            response.contentType = "application/json";
            response.body = createJsonResponse("", false, "物品不可交易");
            return response;
        }
        
        if (item->getOwnerUsername() == username) {
            HttpResponse response;
            response.statusCode = 400;
            response.contentType = "application/json";
            response.body = createJsonResponse("", false, "不能购买自己的物品");
            return response;
        }
        
        int transactionId = dataManager->createTransaction(itemId, username);
        if (transactionId > 0) {
            // 更新物品状态为已预订
            dataManager->updateItemStatus(itemId, ItemStatus::RESERVED);
            
            HttpResponse response;
            response.statusCode = 200;
            response.contentType = "application/json";
            response.body = createJsonResponse("", true, "交易创建成功");
            return response;
        } else {
            HttpResponse response;
            response.statusCode = 500;
            response.contentType = "application/json";
            response.body = createJsonResponse("", false, "交易创建失败");
            return response;
        }
    } else if (request.method == "PUT") {
        // 更新交易状态：从URL中解析交易ID
        size_t pos = request.path.find("/api/transaction/");
        if (pos == std::string::npos) {
            HttpResponse response;
            response.statusCode = 400;
            response.contentType = "application/json";
            response.body = createJsonResponse("", false, "无效的路径");
            return response;
        }
        
        std::string transactionIdStr = request.path.substr(pos + 17); // 移除 "/api/transaction/"
        
        // 解析交易ID
        int transactionId;
        try {
            transactionId = std::stoi(transactionIdStr);
        } catch (const std::exception& e) {
            HttpResponse response;
            response.statusCode = 400;
            response.contentType = "application/json";
            response.body = createJsonResponse("", false, "无效的交易ID");
            return response;
        }
        
        // 从请求体中解析状态值
        std::string statusStr = parseJsonParam(request.body, "status");
        if (statusStr.empty()) {
            HttpResponse response;
            response.statusCode = 400;
            response.contentType = "application/json";
            response.body = createJsonResponse("", false, "状态不能为空");
            return response;
        }
        
        // 转换状态枚举
        TransactionStatus status;
        try {
            status = static_cast<TransactionStatus>(std::stoi(statusStr));
        } catch (const std::exception& e) {
            HttpResponse response;
            response.statusCode = 400;
            response.contentType = "application/json";
            response.body = createJsonResponse("", false, "无效的状态值");
            return response;
        }
        
        // 更新数据库中的交易状态
        if (dataManager->updateTransactionStatus(transactionId, status)) {
            // 如果交易完成，同步更新物品状态为已售出
            if (status == TransactionStatus::COMPLETED) {
                Transaction* transaction = dataManager->getTransactionById(transactionId);
                if (transaction) {
                    dataManager->updateItemStatus(transaction->getItemId(), ItemStatus::SOLD);
                }
            }
            
            HttpResponse response;
            response.statusCode = 200;
            response.contentType = "application/json";
            response.body = createJsonResponse("", true, "交易状态更新成功");
            return response;
        } else {
            HttpResponse response;
            response.statusCode = 400;
            response.contentType = "application/json";
            response.body = createJsonResponse("", false, "交易状态更新失败");
            return response;
        }
    } else {
        HttpResponse response;
        response.statusCode = 405;
        response.contentType = "application/json";
        response.body = createJsonResponse("", false, "不支持的请求方法");
        return response;
    }
}

// 收藏相关API
HttpResponse WebManager::handleFavoriteApi(const HttpRequest& request) {
    if (!validateUserSession(request)) {
        HttpResponse response;
        response.statusCode = 401;
        response.contentType = "application/json";
        response.body = createJsonResponse("", false, "未登录");
        return response;
    }
    
    std::string username = getCurrentUser(request);
    
    if (request.method == "GET") {
        // 获取收藏列表
        std::vector<Item> favorites = dataManager->getUserFavorites(username);
        
        std::string jsonItems = "[";
        for (size_t i = 0; i < favorites.size(); i++) {
            jsonItems += itemToJson(favorites[i]);
            if (i < favorites.size() - 1) jsonItems += ",";
        }
        jsonItems += "]";
        
        HttpResponse response;
        response.statusCode = 200;
        response.contentType = "application/json";
        response.body = createJsonResponse(jsonItems, true, "获取收藏列表成功");
        return response;
    } else if (request.method == "POST") {
        // 添加收藏
        std::string itemIdStr = parseJsonParam(request.body, "itemId");
        if (itemIdStr.empty()) {
            HttpResponse response;
            response.statusCode = 400;
            response.contentType = "application/json";
            response.body = createJsonResponse("", false, "物品ID不能为空");
            return response;
        }
        
        int itemId = std::stoi(itemIdStr);
        if (dataManager->addToFavorites(username, itemId)) {
            HttpResponse response;
            response.statusCode = 200;
            response.contentType = "application/json";
            response.body = createJsonResponse("", true, "添加收藏成功");
            return response;
        } else {
            HttpResponse response;
            response.statusCode = 400;
            response.contentType = "application/json";
            response.body = createJsonResponse("", false, "添加收藏失败");
            return response;
        }
    } else if (request.method == "DELETE") {
        // 移除收藏
        std::string path = request.path;
        std::string itemIdStr;
        
        // 查找最后一个/后的数字
        size_t lastSlash = path.find_last_of('/');
        if (lastSlash != std::string::npos && lastSlash + 1 < path.length()) {
            itemIdStr = path.substr(lastSlash + 1);
        }
        
        if (itemIdStr.empty()) {
            HttpResponse response;
            response.statusCode = 400;
            response.contentType = "application/json";
            response.body = createJsonResponse("", false, "物品ID不能为空");
            return response;
        }
        
        // 验证itemIdStr是否只包含数字
        bool isValidNumber = true;
        for (char c : itemIdStr) {
            if (!std::isdigit(c)) {
                isValidNumber = false;
                break;
            }
        }
        
        if (!isValidNumber) {
            HttpResponse response;
            response.statusCode = 400;
            response.contentType = "application/json";
            response.body = createJsonResponse("", false, "物品ID格式无效");
            return response;
        }
        
        int itemId = std::stoi(itemIdStr);
        
        if (dataManager->removeFromFavorites(username, itemId)) {
            HttpResponse response;
            response.statusCode = 200;
            response.contentType = "application/json";
            response.body = createJsonResponse("", true, "移除收藏成功");
            return response;
        } else {
            HttpResponse response;
            response.statusCode = 400;
            response.contentType = "application/json";
            response.body = createJsonResponse("", false, "移除收藏失败");
            return response;
        }
    } else {
        HttpResponse response;
        response.statusCode = 405;
        response.contentType = "application/json";
        response.body = createJsonResponse("", false, "不支持的请求方法");
        return response;
    }
}

// 工具方法
std::string WebManager::parseJsonParam(const std::string& json, const std::string& key) {
    // 查找键名
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) {
        return "";
    }
    
    // 查找冒号
    size_t colonPos = json.find(':', keyPos);
    if (colonPos == std::string::npos) {
        return "";
    }
    
    // 跳过空格
    size_t valueStart = colonPos + 1;
    while (valueStart < json.length() && (json[valueStart] == ' ' || json[valueStart] == '\t')) {
        valueStart++;
    }
    
    if (valueStart >= json.length()) {
        return "";
    }
    
    // 判断值的类型
    if (json[valueStart] == '"') {
        // 字符串值，需要找到结束引号（处理转义）
        valueStart++; // 跳过开始引号
        std::string result;
        bool escaped = false;
        
        for (size_t i = valueStart; i < json.length(); i++) {
            if (escaped) {
                // 处理转义字符
                if (json[i] == 'n') result += '\n';
                else if (json[i] == 't') result += '\t';
                else if (json[i] == 'r') result += '\r';
                else if (json[i] == '\\') result += '\\';
                else if (json[i] == '"') result += '"';
                else result += json[i];
                escaped = false;
            } else if (json[i] == '\\') {
                escaped = true;
            } else if (json[i] == '"') {
                // 找到结束引号
                return result;
            } else {
                result += json[i];
            }
        }
        return result;
    } else {
        // 数字或布尔值，找到下一个逗号或}
        size_t valueEnd = valueStart;
        while (valueEnd < json.length() && 
               json[valueEnd] != ',' && 
               json[valueEnd] != '}' && 
               json[valueEnd] != ']' &&
               json[valueEnd] != ' ' &&
               json[valueEnd] != '\t' &&
               json[valueEnd] != '\n' &&
               json[valueEnd] != '\r') {
            valueEnd++;
        }
        return json.substr(valueStart, valueEnd - valueStart);
    }
}

std::string WebManager::createJsonResponse(const std::string& data, bool success, const std::string& message) {
    std::ostringstream oss;
    oss << "{\"success\":" << (success ? "true" : "false") 
        << ",\"message\":\"" << message << "\"";
    
    if (!data.empty()) {
        oss << ",\"data\":" << data;
    }
    
    oss << "}";
    return oss.str();
}

std::string WebManager::transactionToJson(const Transaction& transaction) {
    std::ostringstream oss;
    
    // 获取状态名称
    std::string statusName;
    switch (transaction.getStatus()) {
        case TransactionStatus::PENDING:
            statusName = "待确认";
            break;
        case TransactionStatus::CONFIRMED:
            statusName = "已确认";
            break;
        case TransactionStatus::COMPLETED:
            statusName = "已完成";
            break;
        case TransactionStatus::CANCELLED:
            statusName = "已取消";
            break;
        default:
            statusName = "未知";
            break;
    }
    
    oss << "{"
        << "\"id\":" << transaction.getId() << ","
        << "\"itemId\":" << transaction.getItemId() << ","
        << "\"buyerUsername\":\"" << escapeJsonString(transaction.getBuyerUsername()) << "\","
        << "\"sellerUsername\":\"" << escapeJsonString(transaction.getSellerUsername()) << "\","
        << "\"status\":" << static_cast<int>(transaction.getStatus()) << ","
        << "\"statusName\":\"" << statusName << "\","
        << "\"price\":" << transaction.getPrice() << ","
        << "\"createTime\":" << transaction.getCreateTime() << ","
        << "\"updateTime\":" << transaction.getCreateTime() << ","
        << "\"message\":\"" << escapeJsonString("") << "\""
        << "}";
    return oss.str();
}

std::string WebManager::urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '%' && i + 2 < str.length()) {
            int value;
            std::istringstream is(str.substr(i + 1, 2));
            if (is >> std::hex >> value) {
                result += static_cast<char>(value);
                i += 2;
            } else {
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

std::string WebManager::urlEncode(const std::string& str) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;
    
    for (char c : str) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << int(static_cast<unsigned char>(c));
        }
    }
    
    return escaped.str();
}

std::map<std::string, std::string> WebManager::parseQueryParams(const std::string& query) {
    std::map<std::string, std::string> params;
    std::istringstream iss(query);
    std::string pair;
    
    while (std::getline(iss, pair, '&')) {
        size_t pos = pair.find('=');
        if (pos != std::string::npos) {
            std::string key = urlDecode(pair.substr(0, pos));
            std::string value = urlDecode(pair.substr(pos + 1));
            params[key] = value;
        }
    }
    
    return params;
}

// 数据转换
std::string WebManager::userToJson(const User& user) {
    std::ostringstream oss;
    oss << "{\"username\":\"" << user.getUsername() 
        << "\",\"email\":\"" << user.getEmail()
        << "\",\"phone\":\"" << user.getPhone()
        << "\",\"studentId\":\"" << user.getStudentId()
        << "\",\"creditScore\":" << user.getCreditScore() << "}";
    return oss.str();
}

std::string WebManager::escapeJsonString(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    return result;
}

std::string WebManager::itemToJson(const Item& item) {
    std::ostringstream oss;
    oss << "{\"id\":" << item.getId()
        << ",\"title\":\"" << escapeJsonString(item.getTitle())
        << "\",\"description\":\"" << escapeJsonString(item.getDescription())
        << "\",\"price\":" << item.getPrice()
        << ",\"category\":" << static_cast<int>(item.getCategory())
        << ",\"categoryName\":\"" << escapeJsonString(item.getCategoryName())
        << "\",\"status\":" << static_cast<int>(item.getStatus())
        << ",\"statusName\":\"" << escapeJsonString(item.getStatusName())
        << "\",\"ownerUsername\":\"" << escapeJsonString(item.getOwnerUsername())
        << "\",\"condition\":\"" << escapeJsonString(item.getCondition())
        << "\",\"imagePath\":\"" << escapeJsonString(item.getImagePath())
        << "\",\"publishTime\":" << item.getPublishTime() << "}";
    return oss.str();
}


std::string WebManager::messageToJson(const Message& message) {
    std::ostringstream oss;
    oss << "{\"id\":" << message.getId()
        << ",\"transactionId\":" << message.getTransactionId()
        << ",\"senderUsername\":\"" << escapeJsonString(message.getSenderUsername())
        << "\",\"receiverUsername\":\"" << escapeJsonString(message.getReceiverUsername())
        << "\",\"content\":\"" << escapeJsonString(message.getContent())
        << "\",\"sendTime\":" << message.getSendTime()
        << ",\"isRead\":" << (message.getIsRead() ? "true" : "false") << "}";
    return oss.str();
}

// 验证方法
bool WebManager::validateUserSession(const HttpRequest& request) {
    // 简单的会话验证，实际项目中应该使用更安全的方式
    auto it = request.headers.find("Authorization");
    if (it != request.headers.end()) {
        std::string username = it->second;
        // 去除前后空白字符
        username.erase(0, username.find_first_not_of(" \t\r\n"));
        username.erase(username.find_last_not_of(" \t\r\n") + 1);
        // 检查用户是否存在
        if (!username.empty()) {
            User* user = dataManager->getUserByUsername(username);
            return user != nullptr;
        }
    }
    return false;
}

// 从请求头获取当前登录用户名
std::string WebManager::getCurrentUser(const HttpRequest& request) {
    // 从Authorization请求头中获取用户名（简化版实现，实际项目中应使用JWT或Session）
    auto it = request.headers.find("Authorization");
    if (it != request.headers.end()) {
        std::string username = it->second;
        
        // 清理用户名：去除所有空白字符（防止前后端传输中的格式问题）
        username.erase(std::remove_if(username.begin(), username.end(), ::isspace), username.end());
        
        return username;
    }
    return "";
}

HttpResponse WebManager::handleUserCount(const HttpRequest& request) {
    int userCount = dataManager->getUserCount();
    HttpResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = createJsonResponse(std::to_string(userCount), true, "获取用户总数成功");
    return response;
}

HttpResponse WebManager::handleItemCount(const HttpRequest& request) {
    int itemCount = dataManager->getItemCount();
    HttpResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = createJsonResponse(std::to_string(itemCount), true, "获取物品总数成功");
    return response;
}

// 消息相关API
HttpResponse WebManager::handleMessageApi(const HttpRequest& request) {
    HttpResponse response;
    response.contentType = "application/json; charset=utf-8";
    
    // 安全地提取路径后缀
    std::string path = "";
    if (request.path.length() > 12) {
        path = request.path.substr(12); // 移除 "/api/message"
    }
    
    // GET /api/message/transaction/:id - 获取交易的所有消息
    if (request.method == "GET" && path.find("/transaction/") == 0) {
        if (!validateUserSession(request)) {
            response.statusCode = 401;
            response.body = createJsonResponse("", false, "未登录");
            return response;
        }
        
        // 提取交易ID: "/transaction/1" -> "1"
        std::string transactionIdStr = path.substr(13); // 跳过 "/transaction/"
        int transactionId = std::stoi(transactionIdStr);
        
        std::string username = getCurrentUser(request);
        Transaction* transaction = dataManager->getTransactionById(transactionId);
        
        if (!transaction) {
            response.statusCode = 404;
            response.body = createJsonResponse("", false, "交易不存在");
            return response;
        }
        
        // 验证用户是否是交易的买家或卖家
        if (transaction->getBuyerUsername() != username && 
            transaction->getSellerUsername() != username) {
            response.statusCode = 403;
            response.body = createJsonResponse("", false, "无权查看此交易的消息");
            return response;
        }
        
        std::vector<Message> messages = dataManager->getMessagesByTransaction(transactionId);
        
        // 标记接收到的消息为已读
        for (const auto& msg : messages) {
            if (msg.getReceiverUsername() == username && !msg.getIsRead()) {
                dataManager->markMessageAsRead(msg.getId());
            }
        }
        
        std::ostringstream json;
        json << "[";
        for (size_t i = 0; i < messages.size(); i++) {
            json << messageToJson(messages[i]);
            if (i < messages.size() - 1) {
                json << ",";
            }
        }
        json << "]";
        
        response.statusCode = 200;
        response.body = createJsonResponse(json.str(), true, "获取消息列表成功");
        return response;
    }
    
    // POST /api/message - 发送新消息
    if (request.method == "POST") {
        if (!validateUserSession(request)) {
            response.statusCode = 401;
            response.body = createJsonResponse("", false, "未登录");
            return response;
        }
        
        std::string username = getCurrentUser(request);
        
        std::string transactionIdStr = parseJsonParam(request.body, "transactionId");
        std::string content = parseJsonParam(request.body, "content");
        
        if (transactionIdStr.empty() || content.empty()) {
            response.statusCode = 400;
            response.body = createJsonResponse("", false, "参数不完整");
            return response;
        }
        
        int transactionId = std::stoi(transactionIdStr);
        Transaction* transaction = dataManager->getTransactionById(transactionId);
        
        if (!transaction) {
            response.statusCode = 404;
            response.body = createJsonResponse("", false, "交易不存在");
            return response;
        }
        
        // 验证用户是否是交易的买家或卖家
        if (transaction->getBuyerUsername() != username && 
            transaction->getSellerUsername() != username) {
            response.statusCode = 403;
            response.body = createJsonResponse("", false, "无权向此交易发送消息");
            return response;
        }
        
        // 确定接收者
        std::string receiverUsername;
        if (transaction->getBuyerUsername() == username) {
            receiverUsername = transaction->getSellerUsername();
        } else {
            receiverUsername = transaction->getBuyerUsername();
        }
        
        int messageId = dataManager->addMessage(transactionId, username, receiverUsername, content);
        
        if (messageId > 0) {
            Message* message = dataManager->getMessageById(messageId);
            response.statusCode = 200;
            response.body = createJsonResponse(messageToJson(*message), true, "发送消息成功");
        } else {
            response.statusCode = 500;
            response.body = createJsonResponse("", false, "发送消息失败");
        }
        
        return response;
    }
    
    // GET /api/message/unread/count - 获取未读消息数量
    if (request.method == "GET" && path == "/unread/count") {
        if (!validateUserSession(request)) {
            response.statusCode = 401;
            response.body = createJsonResponse("", false, "未登录");
            return response;
        }
        
        std::string username = getCurrentUser(request);
        int unreadCount = dataManager->getUnreadMessageCount(username);
        
        response.statusCode = 200;
        response.body = createJsonResponse(std::to_string(unreadCount), true, "获取未读消息数量成功");
        return response;
    }
    
    response.statusCode = 404;
    response.body = createJsonResponse("", false, "API接口不存在");
    return response;
}