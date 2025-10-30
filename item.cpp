#include "item.h"
#include <iostream>
#include <sstream>
#include <ctime>

// 默认构造函数
Item::Item() : id(0), price(0.0), category(ItemCategory::OTHER), 
               status(ItemStatus::AVAILABLE), publishTime(time(nullptr)) {}

// 带参数构造函数
Item::Item(const std::string& title, const std::string& description, 
           double price, ItemCategory category, const std::string& ownerUsername,
           const std::string& condition)
    : id(0), title(title), description(description), price(price), 
      category(category), status(ItemStatus::AVAILABLE), 
      ownerUsername(ownerUsername), publishTime(time(nullptr)), 
      condition(condition) {}

// MySQL专用构造函数（包含所有字段）
Item::Item(int id, const std::string& title, const std::string& description, 
           double price, ItemCategory category, ItemStatus status,
           const std::string& ownerUsername, const std::string& imagePath,
           time_t publishTime, const std::string& condition)
    : id(id), title(title), description(description), price(price),
      category(category), status(status), ownerUsername(ownerUsername),
      imagePath(imagePath), publishTime(publishTime), condition(condition) {}

// Getter和Setter方法
int Item::getId() const {
    return id;
}

void Item::setId(int id) {
    this->id = id;
}

std::string Item::getTitle() const {
    return title;
}

void Item::setTitle(const std::string& title) {
    this->title = title;
}

std::string Item::getDescription() const {
    return description;
}

void Item::setDescription(const std::string& description) {
    this->description = description;
}

double Item::getPrice() const {
    return price;
}

void Item::setPrice(double price) {
    this->price = price;
}

ItemCategory Item::getCategory() const {
    return category;
}

void Item::setCategory(ItemCategory category) {
    this->category = category;
}

ItemStatus Item::getStatus() const {
    return status;
}

void Item::setStatus(ItemStatus status) {
    this->status = status;
}

std::string Item::getOwnerUsername() const {
    return ownerUsername;
}

void Item::setOwnerUsername(const std::string& ownerUsername) {
    this->ownerUsername = ownerUsername;
}

std::string Item::getImagePath() const {
    return imagePath;
}

void Item::setImagePath(const std::string& imagePath) {
    this->imagePath = imagePath;
}

time_t Item::getPublishTime() const {
    return publishTime;
}

std::string Item::getCondition() const {
    return condition;
}

void Item::setCondition(const std::string& condition) {
    this->condition = condition;
}

// 获取类别名称
std::string Item::getCategoryName() const {
    switch (category) {
        case ItemCategory::BOOKS: return "书籍";
        case ItemCategory::ELECTRONICS: return "电子产品";
        case ItemCategory::CLOTHING: return "服装";
        case ItemCategory::SPORTS: return "运动器材";
        case ItemCategory::DAILY_GOODS: return "日用品";
        case ItemCategory::OTHER: return "其他";
        default: return "未知";
    }
}

// 获取状态名称
std::string Item::getStatusName() const {
    switch (status) {
        case ItemStatus::AVAILABLE: return "可交易";
        case ItemStatus::RESERVED: return "已预订";
        case ItemStatus::SOLD: return "已售出";
        case ItemStatus::CANCELLED: return "已取消";
        default: return "未知";
    }
}

// 显示物品信息
void Item::displayInfo() const {
    std::cout << "物品ID: " << id << std::endl;
    std::cout << "标题: " << title << std::endl;
    std::cout << "描述: " << description << std::endl;
    std::cout << "价格: ¥" << price << std::endl;
    std::cout << "类别: " << getCategoryName() << std::endl;
    std::cout << "状态: " << getStatusName() << std::endl;
    std::cout << "发布者: " << ownerUsername << std::endl;
    std::cout << "使用情况: " << condition << std::endl;
    
    if (!imagePath.empty()) {
        std::cout << "图片: " << imagePath << std::endl;
    }
    
    char timeStr[100];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&publishTime));
    std::cout << "发布时间: " << timeStr << std::endl;
    std::cout << "----------------------------------------" << std::endl;
}

// 序列化
std::string Item::serialize() const {
    std::ostringstream oss;
    oss << id << "|" << title << "|" << description << "|" << price << "|"
        << static_cast<int>(category) << "|" << static_cast<int>(status) << "|"
        << ownerUsername << "|" << imagePath << "|" << publishTime << "|" << condition;
    
    return oss.str();
}

// 反序列化
Item Item::deserialize(const std::string& data) {
    std::istringstream iss(data);
    std::string token;
    std::vector<std::string> tokens;
    
    while (std::getline(iss, token, '|')) {
        tokens.push_back(token);
    }
    
    if (tokens.size() >= 10) {
        Item item;
        item.id = std::stoi(tokens[0]);
        item.title = tokens[1];
        item.description = tokens[2];
        item.price = std::stod(tokens[3]);
        item.category = static_cast<ItemCategory>(std::stoi(tokens[4]));
        item.status = static_cast<ItemStatus>(std::stoi(tokens[5]));
        item.ownerUsername = tokens[6];
        item.imagePath = tokens[7];
        item.publishTime = std::stoll(tokens[8]);
        item.condition = tokens[9];
        return item;
    }
    
    return Item();
}
