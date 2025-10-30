/**
 * MySQL数据库管理器
 * 
 * 功能：封装所有MySQL数据库操作
 * - 用户管理（增删改查）
 * - 物品管理（发布、搜索、筛选）
 * - 交易管理（创建、更新状态）
 * - 消息管理（留言、未读）
 * - 收藏管理
 * 
 * 安全：使用参数化查询防止SQL注入
 */

#ifndef MYSQL_MANAGER_H
#define MYSQL_MANAGER_H

#include <string>
#include <vector>
#include <mysql.h>
#include "user.h"
#include "item.h"
#include "transaction.h"

// 前向声明
class Message;

/**
 * MySQLManager类
 * 数据持久层：所有数据库交互的唯一入口
 */
class MySQLManager {
private:
    MYSQL* conn;
    std::string host;
    std::string user;
    std::string password;
    std::string database;
    int port;
    std::string currentUser;  // 当前登录用户
    
    bool connect();
    void disconnect();
    std::string escapeString(const std::string& str);
    
public:
    MySQLManager(const std::string& host = "localhost", 
                 const std::string& user = "root", 
                 const std::string& password = "root", 
                 const std::string& database = "campus_trading",
                 int port = 3306);
    ~MySQLManager();
    
    // 初始化数据库连接
    bool initialize();
    
    // 用户相关操作
    bool addUser(const User& user);
    User* getUserByUsername(const std::string& username);
    bool updateUser(const User& user);
    bool deleteUser(const std::string& username);
    std::vector<User> getAllUsers();
    int getUserCount();
    bool registerUser(const std::string& username, const std::string& password,
                     const std::string& email, const std::string& phone,
                     const std::string& studentId);
    bool loginUser(const std::string& username, const std::string& password);
    User* getCurrentUser();
    bool logoutUser();
    
    // 物品相关操作
    int addItem(const std::string& title, const std::string& description, 
                double price, ItemCategory category, const std::string& condition, 
                const std::string& imagePath, const std::string& ownerUsername);
    Item* getItemById(int id);
    std::vector<Item> getAllItems();
    std::vector<Item> getItemsByOwner(const std::string& username);
    std::vector<Item> getItemsByCategory(ItemCategory category);
    bool updateItem(const Item& item);
    bool deleteItem(int id);
    int getItemCount();
    std::vector<Item> searchItems(const std::string& keyword);
    std::vector<Item> getItemsWithFilters(const std::string& category = "", 
                                         double minPrice = -1, double maxPrice = -1,
                                         time_t startTime = -1, time_t endTime = -1,
                                         const std::string& status = "");
    bool updateItemStatus(int itemId, ItemStatus status);
    
    // 交易相关操作
    int createTransaction(int itemId, const std::string& buyerUsername, 
                         const std::string& sellerUsername, double price);
    int createTransaction(int itemId, const std::string& buyerUsername);  // 重载版本
    Transaction* getTransactionById(int id);
    std::vector<Transaction> getTransactionsByUser(const std::string& username);
    std::vector<Transaction> getUserTransactions(const std::string& username);  // 别名
    std::vector<Transaction> getAllTransactions();
    bool updateTransactionStatus(int id, TransactionStatus status);
    bool deleteTransaction(int id);
    int getTransactionCount();
    
    // 收藏相关操作
    bool addFavorite(const std::string& username, int itemId);
    bool removeFavorite(const std::string& username, int itemId);
    std::vector<int> getFavoriteItemIds(const std::string& username);
    bool isFavorite(const std::string& username, int itemId);
    bool addToFavorites(const std::string& username, int itemId);  // 别名
    bool removeFromFavorites(const std::string& username, int itemId);  // 别名
    std::vector<Item> getUserFavorites(const std::string& username);
    
    // 消息相关操作
    int addMessage(int transactionId, const std::string& senderUsername, 
                   const std::string& receiverUsername, const std::string& content);
    Message* getMessageById(int id);
    std::vector<Message> getMessagesByTransaction(int transactionId);
    std::vector<Message> getMessagesByUser(const std::string& username);
    std::vector<Message> getUnreadMessages(const std::string& username);
    bool markMessageAsRead(int messageId);
    int getUnreadMessageCount(const std::string& username);
};

#endif // MYSQL_MANAGER_H

