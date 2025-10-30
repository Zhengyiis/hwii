#include "transaction.h"
#include <iostream>
#include <sstream>
#include <ctime>

// Message类实现
Message::Message() : id(0), transactionId(0), sendTime(0), isRead(false) {}

Message::Message(int id, int transactionId, const std::string& senderUsername,
                 const std::string& receiverUsername, const std::string& content,
                 int sendTime, bool isRead)
    : id(id), transactionId(transactionId), senderUsername(senderUsername), 
      receiverUsername(receiverUsername), content(content),
      sendTime(sendTime > 0 ? sendTime : static_cast<int>(std::time(nullptr))),
      isRead(isRead) {}

int Message::getId() const {
    return id;
}

void Message::setId(int id) {
    this->id = id;
}

int Message::getTransactionId() const {
    return transactionId;
}

void Message::setTransactionId(int transactionId) {
    this->transactionId = transactionId;
}

std::string Message::getSenderUsername() const {
    return senderUsername;
}

void Message::setSenderUsername(const std::string& senderUsername) {
    this->senderUsername = senderUsername;
}

std::string Message::getReceiverUsername() const {
    return receiverUsername;
}

void Message::setReceiverUsername(const std::string& receiverUsername) {
    this->receiverUsername = receiverUsername;
}

std::string Message::getContent() const {
    return content;
}

void Message::setContent(const std::string& content) {
    this->content = content;
}

int Message::getSendTime() const {
    return sendTime;
}

bool Message::getIsRead() const {
    return isRead;
}

void Message::setIsRead(bool read) {
    this->isRead = read;
}

void Message::displayMessage() const {
    time_t t = static_cast<time_t>(sendTime);
    char timeStr[100];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&t));
    
    std::cout << "[" << timeStr << "] " << senderUsername << " -> " 
              << receiverUsername << ": " << content << std::endl;
}

std::string Message::toJson() const {
    std::ostringstream json;
    json << "{"
         << "\"id\":" << id << ","
         << "\"transactionId\":" << transactionId << ","
         << "\"senderUsername\":\"" << senderUsername << "\","
         << "\"receiverUsername\":\"" << receiverUsername << "\","
         << "\"content\":\"" << content << "\","
         << "\"sendTime\":" << sendTime << ","
         << "\"isRead\":" << (isRead ? "true" : "false")
         << "}";
    return json.str();
}

std::string Message::serialize() const {
    std::ostringstream oss;
    oss << id << "|"
        << transactionId << "|"
        << senderUsername << "|"
        << receiverUsername << "|"
        << content << "|"
        << sendTime << "|"
        << (isRead ? "1" : "0");
    return oss.str();
}

Message Message::deserialize(const std::string& data) {
    std::istringstream iss(data);
    std::string token;
    std::vector<std::string> tokens;
    
    while (std::getline(iss, token, '|')) {
        tokens.push_back(token);
    }
    
    if (tokens.size() >= 7) {
        return Message(
            std::stoi(tokens[0]),      // id
            std::stoi(tokens[1]),      // transactionId
            tokens[2],                  // senderUsername
            tokens[3],                  // receiverUsername
            tokens[4],                  // content
            std::stoi(tokens[5]),      // sendTime
            tokens[6] == "1"           // isRead
        );
    }
    
    return Message();
}

// Transaction类实现
Transaction::Transaction() : id(0), itemId(0), price(0.0), 
                           status(TransactionStatus::PENDING), 
                           createTime(time(nullptr)), completeTime(0) {}

Transaction::Transaction(int itemId, const std::string& sellerUsername, 
                       const std::string& buyerUsername, double price)
    : id(0), itemId(itemId), sellerUsername(sellerUsername), 
      buyerUsername(buyerUsername), price(price), 
      status(TransactionStatus::PENDING), createTime(time(nullptr)), completeTime(0) {}

// MySQL专用构造函数（包含所有字段）
Transaction::Transaction(int id, int itemId, const std::string& buyerUsername,
                        const std::string& sellerUsername, double price,
                        TransactionStatus status, time_t createTime, time_t completeTime)
    : id(id), itemId(itemId), buyerUsername(buyerUsername),
      sellerUsername(sellerUsername), price(price), status(status),
      createTime(createTime), completeTime(completeTime) {}

int Transaction::getId() const {
    return id;
}

void Transaction::setId(int id) {
    this->id = id;
}

int Transaction::getItemId() const {
    return itemId;
}

void Transaction::setItemId(int itemId) {
    this->itemId = itemId;
}

std::string Transaction::getSellerUsername() const {
    return sellerUsername;
}

void Transaction::setSellerUsername(const std::string& sellerUsername) {
    this->sellerUsername = sellerUsername;
}

std::string Transaction::getBuyerUsername() const {
    return buyerUsername;
}

void Transaction::setBuyerUsername(const std::string& buyerUsername) {
    this->buyerUsername = buyerUsername;
}

double Transaction::getPrice() const {
    return price;
}

void Transaction::setPrice(double price) {
    this->price = price;
}

TransactionStatus Transaction::getStatus() const {
    return status;
}

void Transaction::setStatus(TransactionStatus status) {
    this->status = status;
}

time_t Transaction::getCreateTime() const {
    return createTime;
}

time_t Transaction::getCompleteTime() const {
    return completeTime;
}

void Transaction::setCompleteTime(time_t completeTime) {
    this->completeTime = completeTime;
}

std::string Transaction::getStatusName() const {
    switch (status) {
        case TransactionStatus::PENDING: return "待确认";
        case TransactionStatus::CONFIRMED: return "已确认";
        case TransactionStatus::COMPLETED: return "已完成";
        case TransactionStatus::CANCELLED: return "已取消";
        default: return "未知";
    }
}

void Transaction::addMessage(const Message& message) {
    messages.push_back(message);
}

std::vector<Message> Transaction::getMessages() const {
    return messages;
}

void Transaction::displayInfo() const {
    std::cout << "交易ID: " << id << std::endl;
    std::cout << "物品ID: " << itemId << std::endl;
    std::cout << "卖家: " << sellerUsername << std::endl;
    std::cout << "买家: " << buyerUsername << std::endl;
    std::cout << "价格: ¥" << price << std::endl;
    std::cout << "状态: " << getStatusName() << std::endl;
    
    char timeStr[100];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&createTime));
    std::cout << "创建时间: " << timeStr << std::endl;
    
    if (completeTime > 0) {
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&completeTime));
        std::cout << "完成时间: " << timeStr << std::endl;
    }
    
    std::cout << "消息数量: " << messages.size() << std::endl;
    std::cout << "----------------------------------------" << std::endl;
}

void Transaction::displayMessages() const {
    std::cout << "交易消息记录:" << std::endl;
    for (const auto& message : messages) {
        message.displayMessage();
    }
}

std::string Transaction::serialize() const {
    std::ostringstream oss;
    oss << id << "|" << itemId << "|" << sellerUsername << "|" << buyerUsername << "|"
        << price << "|" << static_cast<int>(status) << "|" << createTime << "|" << completeTime;
    
    // 序列化消息
    oss << "|" << messages.size();
    for (const auto& message : messages) {
        oss << "|" << message.serialize();
    }
    
    return oss.str();
}

Transaction Transaction::deserialize(const std::string& data) {
    std::istringstream iss(data);
    std::string token;
    std::vector<std::string> tokens;
    
    while (std::getline(iss, token, '|')) {
        tokens.push_back(token);
    }
    
    if (tokens.size() >= 9) {
        Transaction transaction;
        transaction.id = std::stoi(tokens[0]);
        transaction.itemId = std::stoi(tokens[1]);
        transaction.sellerUsername = tokens[2];
        transaction.buyerUsername = tokens[3];
        transaction.price = std::stod(tokens[4]);
        transaction.status = static_cast<TransactionStatus>(std::stoi(tokens[5]));
        transaction.createTime = std::stoll(tokens[6]);
        transaction.completeTime = std::stoll(tokens[7]);
        
        // 反序列化消息
        int messageCount = std::stoi(tokens[8]);
        for (int i = 0; i < messageCount; i++) {
            if (9 + i < static_cast<int>(tokens.size())) {
                Message message = Message::deserialize(tokens[9 + i]);
                transaction.messages.push_back(message);
            }
        }
        
        return transaction;
    }
    
    return Transaction();
}
