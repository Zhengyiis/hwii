/**
 * 校园绿色物品共享与循环交易系统 - Web服务器
 * 
 * 功能：提供基于HTTP的RESTful API服务
 * - 用户注册/登录
 * - 物品发布/浏览/搜索
 * - 交易管理
 * - 留言功能
 * - 收藏功能
 * 
 * 技术栈：C++17, MySQL 8.0, Winsock2/Socket
 */

#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <fstream>
#include <vector>
#include "mysql_manager.h"
#include "web_manager.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

/**
 * SimpleWebServer - 轻量级HTTP服务器
 * 负责监听端口、接收客户端请求并分发处理
 */
class SimpleWebServer {
private:
    int port;
    int serverSocket;
    MySQLManager dataManager;
    WebManager webManager;
    
public:
    /**
     * 构造函数 - 初始化服务器
     * @param port 监听端口号
     * 
     * 注意：修改MySQL连接参数（用户名、密码）请编辑本行
     */
    SimpleWebServer(int port) : port(port), dataManager("localhost", "root", "root", "campus_trading"), webManager(&dataManager) {
        // 初始化MySQL连接
        if (!dataManager.initialize()) {
            std::cerr << "MySQL数据库连接失败！请确保MySQL服务已启动并且数据库已创建。" << std::endl;
            std::cerr << "运行命令: mysql -u root -p < database_schema.sql" << std::endl;
            exit(1);
        }
        std::cout << "MySQL数据库连接成功！" << std::endl;
#ifdef _WIN32
        // 初始化Winsock
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup失败" << std::endl;
            exit(1);
        }
#endif
        
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            std::cerr << "创建socket失败" << std::endl;
            exit(1);
        }
        
        // 设置socket选项
        int opt = 1;
        setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
        
        // 绑定地址
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);
        
        if (bind(serverSocket, (struct sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "绑定端口失败" << std::endl;
            exit(1);
        }
        
        // 监听连接
        if (listen(serverSocket, 10) < 0) {
            std::cerr << "监听失败" << std::endl;
            exit(1);
        }
        
        std::cout << "Web服务器启动成功，监听端口: " << port << std::endl;
    }
    
    ~SimpleWebServer() {
#ifdef _WIN32
        closesocket(serverSocket);
        WSACleanup();
#else
        close(serverSocket);
#endif
    }
    
    /**
     * 启动服务器主循环
     * 持续监听并接受客户端连接，每个连接在独立线程中处理
     */
    void start() {
        std::cout << "服务器正在运行，按 Ctrl+C 停止..." << std::endl;
        
        while (true) {
            struct sockaddr_in clientAddress;
            socklen_t clientAddressLength = sizeof(clientAddress);
            
            int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
            if (clientSocket < 0) {
                std::cerr << "接受连接失败" << std::endl;
                continue;
            }
            
            // 处理客户端请求（单线程）
            handleClient(clientSocket);
        }
    }
    
private:
    /**
     * 处理客户端HTTP请求
     * @param clientSocket 客户端套接字
     * 
     * 处理流程：
     * 1. 接收并解析HTTP请求（支持大文件上传）
     * 2. 路由到对应的处理函数
     * 3. 发送HTTP响应
     * 4. 关闭连接
     */
    void handleClient(int clientSocket) {
        const int INITIAL_BUFFER_SIZE = 65536; // 64KB初始缓冲区
        const int MAX_BUFFER_SIZE = 10 * 1024 * 1024; // 10MB最大缓冲区
        std::string request;
        char buffer[INITIAL_BUFFER_SIZE];
        
        // 首先读取请求头
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytesRead <= 0) {
#ifdef _WIN32
            closesocket(clientSocket);
#else
            close(clientSocket);
#endif
            return;
        }
        
        buffer[bytesRead] = '\0';
        request = std::string(buffer, bytesRead);
        
        // 检查是否有更多数据需要读取
        size_t headerEnd = request.find("\r\n\r\n");
        
        if (headerEnd != std::string::npos) {
            size_t contentLengthPos = request.find("Content-Length:");
            if (contentLengthPos != std::string::npos) {
                try {
                    size_t lineEnd = request.find("\r\n", contentLengthPos);
                    std::string lengthStr = request.substr(contentLengthPos + 15, lineEnd - contentLengthPos - 15);
                    // 去除空格
                    lengthStr.erase(0, lengthStr.find_first_not_of(" \t"));
                    lengthStr.erase(lengthStr.find_last_not_of(" \t\r\n") + 1);
                    
                    int contentLength = std::stoi(lengthStr);
                    int bodyStart = headerEnd + 4;
                    int currentBodyLength = bytesRead - bodyStart;
                    
                    // 如果还需要读取更多数据
                    if (currentBodyLength < contentLength && contentLength < MAX_BUFFER_SIZE) {
                        int remainingBytes = contentLength - currentBodyLength;
                        
                        std::vector<char> additionalBuffer(remainingBytes + 1);
                        int additionalBytesRead = 0;
                        
                        while (additionalBytesRead < remainingBytes) {
                            int n = recv(clientSocket, additionalBuffer.data() + additionalBytesRead, 
                                        remainingBytes - additionalBytesRead, 0);
                            if (n <= 0) break;
                            additionalBytesRead += n;
                        }
                        
                        if (additionalBytesRead > 0) {
                            request.append(additionalBuffer.data(), additionalBytesRead);
                        }
                    }
                } catch (const std::exception& e) {
                    // 解析Content-Length失败，使用已读取的数据
                }
            }
        }
        
        // 解析HTTP请求
        HttpRequest httpRequest = parseHttpRequest(request);
        
        // 处理请求
        HttpResponse response = webManager.handleRequest(httpRequest);
        
        // 发送响应
        sendHttpResponse(clientSocket, response);
        
#ifdef _WIN32
        closesocket(clientSocket);
#else
        close(clientSocket);
#endif
    }
    
    HttpRequest parseHttpRequest(const std::string& request) {
        HttpRequest httpRequest;
        std::istringstream requestStream(request);
        std::string line;
        
        // 解析请求行
        if (std::getline(requestStream, line)) {
            std::istringstream lineStream(line);
            lineStream >> httpRequest.method >> httpRequest.path;
        }
        
        // 解析请求头
        while (std::getline(requestStream, line) && line != "\r") {
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string key = line.substr(0, colonPos);
                std::string value = line.substr(colonPos + 1);
                // 去除前后空格
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                httpRequest.headers[key] = value;
            }
        }
        
        // 解析请求体
        std::string body;
        while (std::getline(requestStream, line)) {
            body += line + "\n";
        }
        httpRequest.body = body;
        
        // 解析URL参数
        size_t queryPos = httpRequest.path.find('?');
        if (queryPos != std::string::npos) {
            std::string query = httpRequest.path.substr(queryPos + 1);
            httpRequest.path = httpRequest.path.substr(0, queryPos);
            httpRequest.params = webManager.parseQueryParams(query);
        }
        
        return httpRequest;
    }
    
    void sendHttpResponse(int clientSocket, const HttpResponse& response) {
        std::string httpResponse = "HTTP/1.1 " + std::to_string(response.statusCode) + " OK\r\n";
        httpResponse += "Content-Type: " + response.contentType + "\r\n";
        httpResponse += "Content-Length: " + std::to_string(response.body.length()) + "\r\n";
        
        // 添加CORS头
        httpResponse += "Access-Control-Allow-Origin: *\r\n";
        httpResponse += "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n";
        httpResponse += "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
        
        // 添加其他响应头
        for (const auto& header : response.headers) {
            httpResponse += header.first + ": " + header.second + "\r\n";
        }
        
        httpResponse += "\r\n";
        httpResponse += response.body;
        
        send(clientSocket, httpResponse.c_str(), httpResponse.length(), 0);
    }
};

int main() {
    // 设置控制台编码为UTF-8，支持中文显示
    #ifdef _WIN32
        system("chcp 65001 > nul");
    #endif
    
    std::cout << "=========================================" << std::endl;
    std::cout << "    校园绿色物品共享与循环交易系统" << std::endl;
    std::cout << "            Web版本服务器" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    int port = 8080;
    std::cout << "正在启动Web服务器..." << std::endl;
    
    try {
        SimpleWebServer server(port);
        std::cout << "服务器启动成功！" << std::endl;
        std::cout << "请在浏览器中访问: http://localhost:" << port << std::endl;
        std::cout << "按 Ctrl+C 停止服务器" << std::endl;
        
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "服务器启动失败: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
