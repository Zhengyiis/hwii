#include "user.h"
#include <iostream>
#include <sstream>
#include <ctime>

// 默认构造函数
User::User() : creditScore(100), registerTime(time(nullptr)) {}

// 带参数构造函数
User::User(const std::string& username, const std::string& password, 
           const std::string& email, const std::string& phone, 
           const std::string& studentId) 
    : username(username), password(password), email(email), 
      phone(phone), studentId(studentId), creditScore(100), 
      registerTime(time(nullptr)) {}

// Getter方法
std::string User::getUsername() const {
    return username;
}

void User::setUsername(const std::string& username) {
    this->username = username;
}

std::string User::getPassword() const {
    return password;
}

void User::setPassword(const std::string& password) {
    this->password = password;
}

std::string User::getEmail() const {
    return email;
}

void User::setEmail(const std::string& email) {
    this->email = email;
}

std::string User::getPhone() const {
    return phone;
}

void User::setPhone(const std::string& phone) {
    this->phone = phone;
}

std::string User::getStudentId() const {
    return studentId;
}

void User::setStudentId(const std::string& studentId) {
    this->studentId = studentId;
}

int User::getCreditScore() const {
    return creditScore;
}

void User::setCreditScore(int score) {
    this->creditScore = score;
}

time_t User::getRegisterTime() const {
    return registerTime;
}

// 更新信誉评分
void User::updateCreditScore(int change) {
    creditScore += change;
    if (creditScore < 0) creditScore = 0;
    if (creditScore > 100) creditScore = 100;
}

// 验证密码
bool User::verifyPassword(const std::string& password) const {
    return this->password == password;
}

// 显示用户信息
void User::displayInfo() const {
    std::cout << "用户名: " << username << std::endl;
    std::cout << "邮箱: " << email << std::endl;
    std::cout << "电话: " << phone << std::endl;
    std::cout << "学号: " << studentId << std::endl;
    std::cout << "信誉评分: " << creditScore << std::endl;
    
    char timeStr[100];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&registerTime));
    std::cout << "注册时间: " << timeStr << std::endl;
}

// 序列化
std::string User::serialize() const {
    std::ostringstream oss;
    oss << username << "|" << password << "|" << email << "|" 
        << phone << "|" << studentId << "|" << creditScore << "|" << registerTime;
    return oss.str();
}

// 反序列化
User User::deserialize(const std::string& data) {
    std::istringstream iss(data);
    std::string token;
    std::vector<std::string> tokens;
    
    while (std::getline(iss, token, '|')) {
        tokens.push_back(token);
    }
    
    if (tokens.size() >= 7) {
        User user;
        user.username = tokens[0];
        user.password = tokens[1];
        user.email = tokens[2];
        user.phone = tokens[3];
        user.studentId = tokens[4];
        user.creditScore = std::stoi(tokens[5]);
        user.registerTime = std::stoll(tokens[6]);
        return user;
    }
    
    return User();
}

