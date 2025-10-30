#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>
#include <vector>
#include <ctime>

// 交易状态枚举
enum class TransactionStatus {
    PENDING,        // 待确认
    CONFIRMED,      // 已确认
    COMPLETED,      // 已完成
    CANCELLED       // 已取消
};

// 消息类
class Message {
private:
    int id;                     // 消息ID
    int transactionId;          // 关联交易ID
    std::string senderUsername; // 发送者用户名
    std::string receiverUsername; // 接收者用户名
    std::string content;        // 消息内容
    int sendTime;              // 发送时间（使用int存储时间戳）
    bool isRead;               // 是否已读
    
public:
    Message();
    Message(int id, int transactionId, const std::string& senderUsername, 
            const std::string& receiverUsername, const std::string& content,
            int sendTime = 0, bool isRead = false);
    
    // Getter和Setter方法
    int getId() const;
    void setId(int id);
    
    int getTransactionId() const;
    void setTransactionId(int transactionId);
    
    std::string getSenderUsername() const;
    void setSenderUsername(const std::string& senderUsername);
    
    std::string getReceiverUsername() const;
    void setReceiverUsername(const std::string& receiverUsername);
    
    std::string getContent() const;
    void setContent(const std::string& content);
    
    int getSendTime() const;
    
    bool getIsRead() const;
    void setIsRead(bool read);
    
    // 显示消息
    void displayMessage() const;
    
    // 转换为JSON
    std::string toJson() const;
    
    // 序列化和反序列化
    std::string serialize() const;
    static Message deserialize(const std::string& data);
};

// 交易类
class Transaction {
private:
    int id;                     // 交易ID
    int itemId;                 // 物品ID
    std::string sellerUsername; // 卖家用户名
    std::string buyerUsername;  // 买家用户名
    double price;               // 交易价格
    TransactionStatus status;   // 交易状态
    time_t createTime;          // 创建时间
    time_t completeTime;        // 完成时间
    std::vector<Message> messages; // 交易消息
    
public:
    // 构造函数
    Transaction();
    Transaction(int itemId, const std::string& sellerUsername, 
                const std::string& buyerUsername, double price);
    // MySQL专用构造函数（包含所有字段）
    Transaction(int id, int itemId, const std::string& buyerUsername,
                const std::string& sellerUsername, double price,
                TransactionStatus status, time_t createTime, time_t completeTime);
    
    // Getter和Setter方法
    int getId() const;
    void setId(int id);
    
    int getItemId() const;
    void setItemId(int itemId);
    
    std::string getSellerUsername() const;
    void setSellerUsername(const std::string& sellerUsername);
    
    std::string getBuyerUsername() const;
    void setBuyerUsername(const std::string& buyerUsername);
    
    double getPrice() const;
    void setPrice(double price);
    
    TransactionStatus getStatus() const;
    void setStatus(TransactionStatus status);
    
    time_t getCreateTime() const;
    
    time_t getCompleteTime() const;
    void setCompleteTime(time_t completeTime);
    
    // 获取状态名称
    std::string getStatusName() const;
    
    // 添加消息
    void addMessage(const Message& message);
    
    // 获取消息列表
    std::vector<Message> getMessages() const;
    
    // 显示交易信息
    void displayInfo() const;
    
    // 显示交易消息
    void displayMessages() const;
    
    // 序列化和反序列化
    std::string serialize() const;
    static Transaction deserialize(const std::string& data);
};

#endif

