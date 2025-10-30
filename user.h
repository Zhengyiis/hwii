#ifndef USER_H
#define USER_H

#include <string>
#include <vector>
#include <ctime>

// 用户类
class User {
private:
    std::string username;        // 用户名
    std::string password;        // 密码
    std::string email;          // 邮箱
    std::string phone;          // 电话
    std::string studentId;      // 学号
    int creditScore;            // 信誉评分
    time_t registerTime;        // 注册时间
    
public:
    // 构造函数
    User();
    User(const std::string& username, const std::string& password, 
         const std::string& email, const std::string& phone, 
         const std::string& studentId);
    
    // Getter和Setter方法
    std::string getUsername() const;
    void setUsername(const std::string& username);
    
    std::string getPassword() const;
    void setPassword(const std::string& password);
    
    std::string getEmail() const;
    void setEmail(const std::string& email);
    
    std::string getPhone() const;
    void setPhone(const std::string& phone);
    
    std::string getStudentId() const;
    void setStudentId(const std::string& studentId);
    
    int getCreditScore() const;
    void setCreditScore(int score);
    
    time_t getRegisterTime() const;
    
    // 更新信誉评分
    void updateCreditScore(int change);
    
    // 验证密码
    bool verifyPassword(const std::string& password) const;
    
    // 显示用户信息
    void displayInfo() const;
    
    // 序列化和反序列化
    std::string serialize() const;
    static User deserialize(const std::string& data);
};

#endif

