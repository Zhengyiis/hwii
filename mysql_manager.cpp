#include "mysql_manager.h"
#include <iostream>
#include <sstream>
#include <ctime>

MySQLManager::MySQLManager(const std::string& host, const std::string& user, 
                           const std::string& password, const std::string& database, int port)
    : host(host), user(user), password(password), database(database), port(port), conn(nullptr), currentUser("") {
}

MySQLManager::~MySQLManager() {
    disconnect();
}

bool MySQLManager::connect() {
    conn = mysql_init(nullptr);
    if (!conn) {
        std::cerr << "MySQL初始化失败" << std::endl;
        return false;
    }
    
    // 设置字符集
    mysql_options(conn, MYSQL_SET_CHARSET_NAME, "utf8mb4");
    
    if (!mysql_real_connect(conn, host.c_str(), user.c_str(), password.c_str(), 
                           database.c_str(), port, nullptr, 0)) {
        std::cerr << "MySQL连接失败: " << mysql_error(conn) << std::endl;
        return false;
    }
    
    return true;
}

void MySQLManager::disconnect() {
    if (conn) {
        mysql_close(conn);
        conn = nullptr;
    }
}

std::string MySQLManager::escapeString(const std::string& str) {
    if (!conn) return str;
    
    char* escaped = new char[str.length() * 2 + 1];
    mysql_real_escape_string(conn, escaped, str.c_str(), str.length());
    std::string result(escaped);
    delete[] escaped;
    return result;
}

bool MySQLManager::initialize() {
    return connect();
}

// ==================== 用户相关操作 ====================

bool MySQLManager::addUser(const User& user) {
    if (!conn) return false;
    
    std::ostringstream query;
    query << "INSERT INTO users (username, password, email, phone, student_id, credit_score) VALUES ('"
          << escapeString(user.getUsername()) << "', '"
          << escapeString(user.getPassword()) << "', '"
          << escapeString(user.getEmail()) << "', '"
          << escapeString(user.getPhone()) << "', '"
          << escapeString(user.getStudentId()) << "', "
          << user.getCreditScore() << ")";
    
    if (mysql_query(conn, query.str().c_str())) {
        std::cerr << "添加用户失败: " << mysql_error(conn) << std::endl;
        return false;
    }
    
    return true;
}

User* MySQLManager::getUserByUsername(const std::string& username) {
    if (!conn) return nullptr;
    
    std::ostringstream query;
    query << "SELECT username, password, email, phone, student_id, credit_score FROM users WHERE username = '"
          << escapeString(username) << "'";
    
    if (mysql_query(conn, query.str().c_str())) {
        std::cerr << "查询用户失败: " << mysql_error(conn) << std::endl;
        return nullptr;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return nullptr;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    User* user = nullptr;
    
    if (row) {
        user = new User(row[0], row[1], row[2], row[3], row[4]);
        user->setCreditScore(std::stoi(row[5]));
    }
    
    mysql_free_result(result);
    return user;
}

int MySQLManager::getUserCount() {
    if (!conn) return 0;
    
    if (mysql_query(conn, "SELECT COUNT(*) FROM users")) {
        return 0;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return 0;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    int count = row ? std::stoi(row[0]) : 0;
    
    mysql_free_result(result);
    return count;
}

// ==================== 物品相关操作 ====================

int MySQLManager::addItem(const std::string& title, const std::string& description, 
                          double price, ItemCategory category, const std::string& condition, 
                          const std::string& imagePath, const std::string& ownerUsername) {
    if (!conn) return -1;
    
    int publishTime = static_cast<int>(std::time(nullptr));
    
    std::ostringstream query;
    query << "INSERT INTO items (title, description, price, category, status, owner_username, "
          << "image_path, publish_time, item_condition) VALUES ('"
          << escapeString(title) << "', '"
          << escapeString(description) << "', "
          << price << ", "
          << static_cast<int>(category) << ", 0, '"
          << escapeString(ownerUsername) << "', '"
          << escapeString(imagePath) << "', "
          << publishTime << ", '"
          << escapeString(condition) << "')";
    
    if (mysql_query(conn, query.str().c_str())) {
        std::cerr << "添加物品失败: " << mysql_error(conn) << std::endl;
        return -1;
    }
    
    return static_cast<int>(mysql_insert_id(conn));
}

Item* MySQLManager::getItemById(int id) {
    if (!conn) return nullptr;
    
    std::ostringstream query;
    query << "SELECT id, title, description, price, category, status, owner_username, "
          << "image_path, publish_time, item_condition FROM items WHERE id = " << id;
    
    if (mysql_query(conn, query.str().c_str())) {
        return nullptr;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return nullptr;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    Item* item = nullptr;
    
    if (row) {
        item = new Item(
            std::stoi(row[0]),           // id
            row[1],                       // title
            row[2],                       // description
            std::stod(row[3]),           // price
            static_cast<ItemCategory>(std::stoi(row[4])), // category
            static_cast<ItemStatus>(std::stoi(row[5])),   // status
            row[6],                       // ownerUsername
            row[7] ? row[7] : "",        // imagePath
            std::stoi(row[8]),           // publishTime
            row[9]                        // condition
        );
    }
    
    mysql_free_result(result);
    return item;
}

std::vector<Item> MySQLManager::getAllItems() {
    std::vector<Item> items;
    if (!conn) return items;
    
    const char* query = "SELECT id, title, description, price, category, status, owner_username, "
                       "image_path, publish_time, item_condition FROM items ORDER BY id DESC";
    
    if (mysql_query(conn, query)) {
        return items;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return items;
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        Item item(
            std::stoi(row[0]),
            row[1],
            row[2],
            std::stod(row[3]),
            static_cast<ItemCategory>(std::stoi(row[4])),
            static_cast<ItemStatus>(std::stoi(row[5])),
            row[6],
            row[7] ? row[7] : "",
            std::stoi(row[8]),
            row[9]
        );
        items.push_back(item);
    }
    
    mysql_free_result(result);
    return items;
}

std::vector<Item> MySQLManager::getItemsByOwner(const std::string& username) {
    std::vector<Item> items;
    if (!conn) return items;
    
    std::ostringstream query;
    query << "SELECT id, title, description, price, category, status, owner_username, "
          << "image_path, publish_time, item_condition FROM items WHERE owner_username = '"
          << escapeString(username) << "' ORDER BY id DESC";
    
    if (mysql_query(conn, query.str().c_str())) {
        return items;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return items;
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        Item item(
            std::stoi(row[0]),
            row[1],
            row[2],
            std::stod(row[3]),
            static_cast<ItemCategory>(std::stoi(row[4])),
            static_cast<ItemStatus>(std::stoi(row[5])),
            row[6],
            row[7] ? row[7] : "",
            std::stoi(row[8]),
            row[9]
        );
        items.push_back(item);
    }
    
    mysql_free_result(result);
    return items;
}

bool MySQLManager::updateItem(const Item& item) {
    if (!conn) return false;
    
    std::ostringstream query;
    query << "UPDATE items SET "
          << "title = '" << escapeString(item.getTitle()) << "', "
          << "description = '" << escapeString(item.getDescription()) << "', "
          << "price = " << item.getPrice() << ", "
          << "category = " << static_cast<int>(item.getCategory()) << ", "
          << "status = " << static_cast<int>(item.getStatus()) << ", "
          << "image_path = '" << escapeString(item.getImagePath()) << "', "
          << "item_condition = '" << escapeString(item.getCondition()) << "' "
          << "WHERE id = " << item.getId();
    
    if (mysql_query(conn, query.str().c_str())) {
        std::cerr << "更新物品失败: " << mysql_error(conn) << std::endl;
        return false;
    }
    
    return true;
}

bool MySQLManager::deleteItem(int id) {
    if (!conn) return false;
    
    std::ostringstream query;
    query << "DELETE FROM items WHERE id = " << id;
    
    if (mysql_query(conn, query.str().c_str())) {
        return false;
    }
    
    return true;
}

int MySQLManager::getItemCount() {
    if (!conn) return 0;
    
    if (mysql_query(conn, "SELECT COUNT(*) FROM items")) {
        return 0;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return 0;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    int count = row ? std::stoi(row[0]) : 0;
    
    mysql_free_result(result);
    return count;
}

// ==================== 交易相关操作 ====================

int MySQLManager::createTransaction(int itemId, const std::string& buyerUsername, 
                                    const std::string& sellerUsername, double price) {
    if (!conn) return -1;
    
    int createTime = static_cast<int>(std::time(nullptr));
    
    std::ostringstream query;
    query << "INSERT INTO transactions (item_id, buyer_username, seller_username, price, "
          << "status, create_time, complete_time) VALUES ("
          << itemId << ", '"
          << escapeString(buyerUsername) << "', '"
          << escapeString(sellerUsername) << "', "
          << price << ", 0, "
          << createTime << ", 0)";
    
    if (mysql_query(conn, query.str().c_str())) {
        std::cerr << "创建交易失败: " << mysql_error(conn) << std::endl;
        return -1;
    }
    
    return static_cast<int>(mysql_insert_id(conn));
}

std::vector<Transaction> MySQLManager::getTransactionsByUser(const std::string& username) {
    std::vector<Transaction> transactions;
    if (!conn) return transactions;
    
    std::ostringstream query;
    query << "SELECT id, item_id, buyer_username, seller_username, price, status, "
          << "create_time, complete_time FROM transactions WHERE buyer_username = '"
          << escapeString(username) << "' OR seller_username = '"
          << escapeString(username) << "' ORDER BY id DESC";
    
    if (mysql_query(conn, query.str().c_str())) {
        return transactions;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return transactions;
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        Transaction trans(
            std::stoi(row[0]),  // id
            std::stoi(row[1]),  // itemId
            row[2],             // buyerUsername
            row[3],             // sellerUsername
            std::stod(row[4]),  // price
            static_cast<TransactionStatus>(std::stoi(row[5])), // status
            std::stoi(row[6]),  // createTime
            std::stoi(row[7])   // completeTime
        );
        transactions.push_back(trans);
    }
    
    mysql_free_result(result);
    return transactions;
}

bool MySQLManager::updateTransactionStatus(int id, TransactionStatus status) {
    if (!conn) return false;
    
    std::ostringstream query;
    query << "UPDATE transactions SET status = " << static_cast<int>(status);
    
    if (status == TransactionStatus::COMPLETED) {
        int completeTime = static_cast<int>(std::time(nullptr));
        query << ", complete_time = " << completeTime;
    }
    
    query << " WHERE id = " << id;
    
    if (mysql_query(conn, query.str().c_str())) {
        return false;
    }
    
    return true;
}

int MySQLManager::getTransactionCount() {
    if (!conn) return 0;
    
    if (mysql_query(conn, "SELECT COUNT(*) FROM transactions")) {
        return 0;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return 0;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    int count = row ? std::stoi(row[0]) : 0;
    
    mysql_free_result(result);
    return count;
}

// ==================== 收藏相关操作 ====================

bool MySQLManager::addFavorite(const std::string& username, int itemId) {
    if (!conn) return false;
    
    std::ostringstream query;
    query << "INSERT IGNORE INTO favorites (username, item_id) VALUES ('"
          << escapeString(username) << "', " << itemId << ")";
    
    if (mysql_query(conn, query.str().c_str())) {
        return false;
    }
    
    return true;
}

bool MySQLManager::removeFavorite(const std::string& username, int itemId) {
    if (!conn) return false;
    
    std::ostringstream query;
    query << "DELETE FROM favorites WHERE username = '"
          << escapeString(username) << "' AND item_id = " << itemId;
    
    if (mysql_query(conn, query.str().c_str())) {
        return false;
    }
    
    return true;
}

std::vector<int> MySQLManager::getFavoriteItemIds(const std::string& username) {
    std::vector<int> itemIds;
    if (!conn) return itemIds;
    
    std::ostringstream query;
    query << "SELECT item_id FROM favorites WHERE username = '"
          << escapeString(username) << "' ORDER BY created_at DESC";
    
    if (mysql_query(conn, query.str().c_str())) {
        return itemIds;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return itemIds;
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        itemIds.push_back(std::stoi(row[0]));
    }
    
    mysql_free_result(result);
    return itemIds;
}

bool MySQLManager::isFavorite(const std::string& username, int itemId) {
    if (!conn) return false;
    
    std::ostringstream query;
    query << "SELECT COUNT(*) FROM favorites WHERE username = '"
          << escapeString(username) << "' AND item_id = " << itemId;
    
    if (mysql_query(conn, query.str().c_str())) {
        return false;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return false;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    bool exists = row && std::stoi(row[0]) > 0;
    
    mysql_free_result(result);
    return exists;
}

// ==================== 其他方法的完整实现 ====================

bool MySQLManager::updateUser(const User& user) {
    if (!conn) return false;
    
    std::ostringstream query;
    query << "UPDATE users SET "
          << "password = '" << escapeString(user.getPassword()) << "', "
          << "email = '" << escapeString(user.getEmail()) << "', "
          << "phone = '" << escapeString(user.getPhone()) << "', "
          << "student_id = '" << escapeString(user.getStudentId()) << "', "
          << "credit_score = " << user.getCreditScore() << " "
          << "WHERE username = '" << escapeString(user.getUsername()) << "'";
    
    if (mysql_query(conn, query.str().c_str())) {
        std::cerr << "更新用户失败: " << mysql_error(conn) << std::endl;
        return false;
    }
    
    return true;
}

bool MySQLManager::deleteUser(const std::string& username) {
    if (!conn) return false;
    
    std::ostringstream query;
    query << "DELETE FROM users WHERE username = '" << escapeString(username) << "'";
    
    if (mysql_query(conn, query.str().c_str())) {
        std::cerr << "删除用户失败: " << mysql_error(conn) << std::endl;
        return false;
    }
    
    return true;
}

std::vector<User> MySQLManager::getAllUsers() {
    std::vector<User> users;
    if (!conn) return users;
    
    const char* query = "SELECT username, password, email, phone, student_id, credit_score FROM users ORDER BY id DESC";
    
    if (mysql_query(conn, query)) {
        return users;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return users;
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        User user(row[0], row[1], row[2], row[3], row[4]);
        user.setCreditScore(std::stoi(row[5]));
        users.push_back(user);
    }
    
    mysql_free_result(result);
    return users;
}

std::vector<Item> MySQLManager::getItemsByCategory(ItemCategory category) {
    std::vector<Item> items;
    if (!conn) return items;
    
    std::ostringstream query;
    query << "SELECT id, title, description, price, category, status, owner_username, "
          << "image_path, publish_time, item_condition FROM items WHERE category = "
          << static_cast<int>(category) << " ORDER BY id DESC";
    
    if (mysql_query(conn, query.str().c_str())) {
        return items;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return items;
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        Item item(
            std::stoi(row[0]),
            row[1],
            row[2],
            std::stod(row[3]),
            static_cast<ItemCategory>(std::stoi(row[4])),
            static_cast<ItemStatus>(std::stoi(row[5])),
            row[6],
            row[7] ? row[7] : "",
            std::stoi(row[8]),
            row[9]
        );
        items.push_back(item);
    }
    
    mysql_free_result(result);
    return items;
}

Transaction* MySQLManager::getTransactionById(int id) {
    if (!conn) return nullptr;
    
    std::ostringstream query;
    query << "SELECT id, item_id, buyer_username, seller_username, price, status, "
          << "create_time, complete_time FROM transactions WHERE id = " << id;
    
    if (mysql_query(conn, query.str().c_str())) {
        return nullptr;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return nullptr;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    Transaction* trans = nullptr;
    
    if (row) {
        trans = new Transaction(
            std::stoi(row[0]),  // id
            std::stoi(row[1]),  // itemId
            row[2],             // buyerUsername
            row[3],             // sellerUsername
            std::stod(row[4]),  // price
            static_cast<TransactionStatus>(std::stoi(row[5])), // status
            std::stoi(row[6]),  // createTime
            std::stoi(row[7])   // completeTime
        );
    }
    
    mysql_free_result(result);
    return trans;
}

std::vector<Transaction> MySQLManager::getAllTransactions() {
    std::vector<Transaction> transactions;
    if (!conn) return transactions;
    
    const char* query = "SELECT id, item_id, buyer_username, seller_username, price, status, "
                       "create_time, complete_time FROM transactions ORDER BY id DESC";
    
    if (mysql_query(conn, query)) {
        return transactions;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return transactions;
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        Transaction trans(
            std::stoi(row[0]),  // id
            std::stoi(row[1]),  // itemId
            row[2],             // buyerUsername
            row[3],             // sellerUsername
            std::stod(row[4]),  // price
            static_cast<TransactionStatus>(std::stoi(row[5])), // status
            std::stoi(row[6]),  // createTime
            std::stoi(row[7])   // completeTime
        );
        transactions.push_back(trans);
    }
    
    mysql_free_result(result);
    return transactions;
}

bool MySQLManager::deleteTransaction(int id) {
    if (!conn) return false;
    
    std::ostringstream query;
    query << "DELETE FROM transactions WHERE id = " << id;
    
    if (mysql_query(conn, query.str().c_str())) {
        std::cerr << "删除交易失败: " << mysql_error(conn) << std::endl;
        return false;
    }
    
    return true;
}

// ==================== 消息相关操作 ====================

int MySQLManager::addMessage(int transactionId, const std::string& senderUsername, 
                              const std::string& receiverUsername, const std::string& content) {
    if (!conn) return -1;
    
    int sendTime = static_cast<int>(std::time(nullptr));
    
    std::ostringstream query;
    query << "INSERT INTO messages (transaction_id, sender_username, receiver_username, "
          << "content, send_time, is_read) VALUES ("
          << transactionId << ", '"
          << escapeString(senderUsername) << "', '"
          << escapeString(receiverUsername) << "', '"
          << escapeString(content) << "', "
          << sendTime << ", 0)";
    
    if (mysql_query(conn, query.str().c_str())) {
        std::cerr << "添加消息失败: " << mysql_error(conn) << std::endl;
        return -1;
    }
    
    return static_cast<int>(mysql_insert_id(conn));
}

Message* MySQLManager::getMessageById(int id) {
    if (!conn) return nullptr;
    
    std::ostringstream query;
    query << "SELECT id, transaction_id, sender_username, receiver_username, "
          << "content, send_time, is_read FROM messages WHERE id = " << id;
    
    if (mysql_query(conn, query.str().c_str())) {
        return nullptr;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return nullptr;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    Message* message = nullptr;
    
    if (row) {
        message = new Message(
            std::stoi(row[0]),  // id
            std::stoi(row[1]),  // transactionId
            row[2],             // senderUsername
            row[3],             // receiverUsername
            row[4],             // content
            std::stoi(row[5]),  // sendTime
            std::stoi(row[6]) != 0  // isRead
        );
    }
    
    mysql_free_result(result);
    return message;
}

std::vector<Message> MySQLManager::getMessagesByTransaction(int transactionId) {
    std::vector<Message> messages;
    if (!conn) return messages;
    
    std::ostringstream query;
    query << "SELECT id, transaction_id, sender_username, receiver_username, "
          << "content, send_time, is_read FROM messages WHERE transaction_id = "
          << transactionId << " ORDER BY send_time ASC";
    
    if (mysql_query(conn, query.str().c_str())) {
        return messages;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return messages;
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        Message message(
            std::stoi(row[0]),  // id
            std::stoi(row[1]),  // transactionId
            row[2],             // senderUsername
            row[3],             // receiverUsername
            row[4],             // content
            std::stoi(row[5]),  // sendTime
            std::stoi(row[6]) != 0  // isRead
        );
        messages.push_back(message);
    }
    
    mysql_free_result(result);
    return messages;
}

std::vector<Message> MySQLManager::getMessagesByUser(const std::string& username) {
    std::vector<Message> messages;
    if (!conn) return messages;
    
    std::ostringstream query;
    query << "SELECT id, transaction_id, sender_username, receiver_username, "
          << "content, send_time, is_read FROM messages WHERE sender_username = '"
          << escapeString(username) << "' OR receiver_username = '"
          << escapeString(username) << "' ORDER BY send_time DESC";
    
    if (mysql_query(conn, query.str().c_str())) {
        return messages;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return messages;
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        Message message(
            std::stoi(row[0]),  // id
            std::stoi(row[1]),  // transactionId
            row[2],             // senderUsername
            row[3],             // receiverUsername
            row[4],             // content
            std::stoi(row[5]),  // sendTime
            std::stoi(row[6]) != 0  // isRead
        );
        messages.push_back(message);
    }
    
    mysql_free_result(result);
    return messages;
}

std::vector<Message> MySQLManager::getUnreadMessages(const std::string& username) {
    std::vector<Message> messages;
    if (!conn) return messages;
    
    std::ostringstream query;
    query << "SELECT id, transaction_id, sender_username, receiver_username, "
          << "content, send_time, is_read FROM messages WHERE receiver_username = '"
          << escapeString(username) << "' AND is_read = 0 ORDER BY send_time DESC";
    
    if (mysql_query(conn, query.str().c_str())) {
        return messages;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return messages;
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        Message message(
            std::stoi(row[0]),  // id
            std::stoi(row[1]),  // transactionId
            row[2],             // senderUsername
            row[3],             // receiverUsername
            row[4],             // content
            std::stoi(row[5]),  // sendTime
            std::stoi(row[6]) != 0  // isRead
        );
        messages.push_back(message);
    }
    
    mysql_free_result(result);
    return messages;
}

bool MySQLManager::markMessageAsRead(int messageId) {
    if (!conn) return false;
    
    std::ostringstream query;
    query << "UPDATE messages SET is_read = 1 WHERE id = " << messageId;
    
    if (mysql_query(conn, query.str().c_str())) {
        std::cerr << "标记消息已读失败: " << mysql_error(conn) << std::endl;
        return false;
    }
    
    return true;
}

int MySQLManager::getUnreadMessageCount(const std::string& username) {
    if (!conn) return 0;
    
    std::ostringstream query;
    query << "SELECT COUNT(*) FROM messages WHERE receiver_username = '"
          << escapeString(username) << "' AND is_read = 0";
    
    if (mysql_query(conn, query.str().c_str())) {
        return 0;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return 0;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    int count = row ? std::stoi(row[0]) : 0;
    
    mysql_free_result(result);
    return count;
}

// ==================== 用户会话管理 ====================

bool MySQLManager::registerUser(const std::string& username, const std::string& password,
                                 const std::string& email, const std::string& phone,
                                 const std::string& studentId) {
    User user(username, password, email, phone, studentId);
    return addUser(user);
}

bool MySQLManager::loginUser(const std::string& username, const std::string& password) {
    User* user = getUserByUsername(username);
    if (user && user->verifyPassword(password)) {
        currentUser = username;
        delete user;
        return true;
    }
    if (user) delete user;
    return false;
}

User* MySQLManager::getCurrentUser() {
    if (currentUser.empty()) {
        return nullptr;
    }
    return getUserByUsername(currentUser);
}

bool MySQLManager::logoutUser() {
    if (currentUser.empty()) {
        return false;
    }
    currentUser.clear();
    return true;
}

// ==================== 物品搜索和筛选 ====================

std::vector<Item> MySQLManager::searchItems(const std::string& keyword) {
    std::vector<Item> items;
    if (!conn || keyword.empty()) return items;
    
    std::string escapedKeyword = escapeString(keyword);
    std::ostringstream query;
    query << "SELECT id, title, description, price, category, status, owner_username, "
          << "image_path, publish_time, item_condition FROM items WHERE "
          << "(title LIKE '%" << escapedKeyword << "%' OR "
          << "description LIKE '%" << escapedKeyword << "%') "
          << "AND status = 0 ORDER BY id DESC";
    
    if (mysql_query(conn, query.str().c_str())) {
        return items;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return items;
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        Item item(
            std::stoi(row[0]),
            row[1],
            row[2],
            std::stod(row[3]),
            static_cast<ItemCategory>(std::stoi(row[4])),
            static_cast<ItemStatus>(std::stoi(row[5])),
            row[6],
            row[7] ? row[7] : "",
            std::stoi(row[8]),
            row[9]
        );
        items.push_back(item);
    }
    
    mysql_free_result(result);
    return items;
}

std::vector<Item> MySQLManager::getItemsWithFilters(const std::string& category, 
                                                     double minPrice, double maxPrice,
                                                     time_t startTime, time_t endTime,
                                                     const std::string& status) {
    std::vector<Item> items;
    if (!conn) return items;
    
    std::ostringstream query;
    query << "SELECT id, title, description, price, category, status, owner_username, "
          << "image_path, publish_time, item_condition FROM items WHERE 1=1";
    
    if (!category.empty()) {
        query << " AND category = " << category;
    }
    if (minPrice >= 0) {
        query << " AND price >= " << minPrice;
    }
    if (maxPrice >= 0) {
        query << " AND price <= " << maxPrice;
    }
    if (startTime >= 0) {
        query << " AND publish_time >= " << startTime;
    }
    if (endTime >= 0) {
        query << " AND publish_time <= " << endTime;
    }
    if (!status.empty()) {
        query << " AND status = " << status;
    }
    
    query << " ORDER BY id DESC";
    
    if (mysql_query(conn, query.str().c_str())) {
        return items;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return items;
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        Item item(
            std::stoi(row[0]),
            row[1],
            row[2],
            std::stod(row[3]),
            static_cast<ItemCategory>(std::stoi(row[4])),
            static_cast<ItemStatus>(std::stoi(row[5])),
            row[6],
            row[7] ? row[7] : "",
            std::stoi(row[8]),
            row[9]
        );
        items.push_back(item);
    }
    
    mysql_free_result(result);
    return items;
}

bool MySQLManager::updateItemStatus(int itemId, ItemStatus status) {
    if (!conn) return false;
    
    std::ostringstream query;
    query << "UPDATE items SET status = " << static_cast<int>(status)
          << " WHERE id = " << itemId;
    
    if (mysql_query(conn, query.str().c_str())) {
        std::cerr << "更新物品状态失败: " << mysql_error(conn) << std::endl;
        return false;
    }
    
    return true;
}

// ==================== 交易创建重载 ====================

int MySQLManager::createTransaction(int itemId, const std::string& buyerUsername) {
    // 根据itemId查找卖家
    Item* item = getItemById(itemId);
    if (!item) {
        return -1;
    }
    
    std::string sellerUsername = item->getOwnerUsername();
    double price = item->getPrice();
    delete item;
    
    return createTransaction(itemId, buyerUsername, sellerUsername, price);
}

std::vector<Transaction> MySQLManager::getUserTransactions(const std::string& username) {
    return getTransactionsByUser(username);
}

// ==================== 收藏功能别名 ====================

bool MySQLManager::addToFavorites(const std::string& username, int itemId) {
    return addFavorite(username, itemId);
}

bool MySQLManager::removeFromFavorites(const std::string& username, int itemId) {
    return removeFavorite(username, itemId);
}

std::vector<Item> MySQLManager::getUserFavorites(const std::string& username) {
    std::vector<Item> items;
    std::vector<int> itemIds = getFavoriteItemIds(username);
    
    for (int itemId : itemIds) {
        Item* item = getItemById(itemId);
        if (item) {
            items.push_back(*item);
            delete item;
        }
    }
    
    return items;
}

