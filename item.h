#ifndef ITEM_H
#define ITEM_H

#include <string>
#include <vector>
#include <ctime>

// 物品状态枚举
enum class ItemStatus {
    AVAILABLE,      // 可交易
    RESERVED,       // 已预订
    SOLD,          // 已售出
    CANCELLED      // 已取消
};

// 物品类别枚举
enum class ItemCategory {
    BOOKS,          // 书籍
    ELECTRONICS,    // 电子产品
    CLOTHING,       // 服装
    SPORTS,         // 运动器材
    DAILY_GOODS,    // 日用品
    OTHER          // 其他
};

// 物品类
class Item {
private:
    int id;                     // 物品ID
    std::string title;          // 物品标题
    std::string description;    // 物品描述
    double price;              // 价格
    ItemCategory category;      // 物品类别
    ItemStatus status;          // 物品状态
    std::string ownerUsername;  // 发布者用户名
    std::string imagePath;      // 图片路径
    time_t publishTime;         // 发布时间
    std::string condition;      // 使用情况（如：全新、九成新等）
    
public:
    // 构造函数
    Item();
    Item(const std::string& title, const std::string& description, 
         double price, ItemCategory category, const std::string& ownerUsername,
         const std::string& condition);
    // MySQL专用构造函数（包含所有字段）
    Item(int id, const std::string& title, const std::string& description, 
         double price, ItemCategory category, ItemStatus status,
         const std::string& ownerUsername, const std::string& imagePath,
         time_t publishTime, const std::string& condition);
    
    // Getter和Setter方法
    int getId() const;
    void setId(int id);
    
    std::string getTitle() const;
    void setTitle(const std::string& title);
    
    std::string getDescription() const;
    void setDescription(const std::string& description);
    
    double getPrice() const;
    void setPrice(double price);
    
    ItemCategory getCategory() const;
    void setCategory(ItemCategory category);
    
    ItemStatus getStatus() const;
    void setStatus(ItemStatus status);
    
    std::string getOwnerUsername() const;
    void setOwnerUsername(const std::string& ownerUsername);
    
    std::string getImagePath() const;
    void setImagePath(const std::string& imagePath);
    
    time_t getPublishTime() const;
    
    std::string getCondition() const;
    void setCondition(const std::string& condition);
    
    // 获取类别名称
    std::string getCategoryName() const;
    
    // 获取状态名称
    std::string getStatusName() const;
    
    // 显示物品信息
    void displayInfo() const;
    
    // 序列化和反序列化
    std::string serialize() const;
    static Item deserialize(const std::string& data);
};

#endif

