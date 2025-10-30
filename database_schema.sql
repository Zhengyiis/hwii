-- 校园绿色物品共享与循环交易系统数据库设计
-- 创建数据库
CREATE DATABASE IF NOT EXISTS campus_trading CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE campus_trading;

-- 用户表
CREATE TABLE IF NOT EXISTS users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(50) NOT NULL UNIQUE,
    password VARCHAR(255) NOT NULL,
    email VARCHAR(100) NOT NULL,
    phone VARCHAR(20) NOT NULL,
    student_id VARCHAR(20) NOT NULL,
    credit_score INT DEFAULT 100,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_username (username),
    INDEX idx_student_id (student_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 物品表
CREATE TABLE IF NOT EXISTS items (
    id INT AUTO_INCREMENT PRIMARY KEY,
    title VARCHAR(200) NOT NULL,
    description TEXT,
    price DECIMAL(10, 2) NOT NULL,
    category INT NOT NULL COMMENT '0:图书,1:电子产品,2:生活用品,3:运动器材,4:其他',
    status INT DEFAULT 0 COMMENT '0:可交易,1:已预订,2:已售出',
    owner_username VARCHAR(50) NOT NULL,
    image_path TEXT COMMENT 'Base64编码的图片或文件路径',
    publish_time INT NOT NULL,
    item_condition VARCHAR(50) NOT NULL COMMENT '使用情况',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_owner (owner_username),
    INDEX idx_status (status),
    INDEX idx_category (category),
    FOREIGN KEY (owner_username) REFERENCES users(username) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 交易表
CREATE TABLE IF NOT EXISTS transactions (
    id INT AUTO_INCREMENT PRIMARY KEY,
    item_id INT NOT NULL,
    buyer_username VARCHAR(50) NOT NULL,
    seller_username VARCHAR(50) NOT NULL,
    price DECIMAL(10, 2) NOT NULL,
    status INT DEFAULT 0 COMMENT '0:待确认,1:已确认,2:已完成,3:已取消',
    create_time INT NOT NULL,
    complete_time INT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_buyer (buyer_username),
    INDEX idx_seller (seller_username),
    INDEX idx_item (item_id),
    INDEX idx_status (status),
    FOREIGN KEY (item_id) REFERENCES items(id) ON DELETE CASCADE,
    FOREIGN KEY (buyer_username) REFERENCES users(username) ON DELETE CASCADE,
    FOREIGN KEY (seller_username) REFERENCES users(username) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 收藏表
CREATE TABLE IF NOT EXISTS favorites (
    id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(50) NOT NULL,
    item_id INT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY unique_favorite (username, item_id),
    INDEX idx_username (username),
    INDEX idx_item (item_id),
    FOREIGN KEY (username) REFERENCES users(username) ON DELETE CASCADE,
    FOREIGN KEY (item_id) REFERENCES items(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 消息表（买卖双方留言）
CREATE TABLE IF NOT EXISTS messages (
    id INT AUTO_INCREMENT PRIMARY KEY,
    transaction_id INT NOT NULL,
    sender_username VARCHAR(50) NOT NULL,
    receiver_username VARCHAR(50) NOT NULL,
    content TEXT NOT NULL,
    send_time INT NOT NULL,
    is_read TINYINT(1) DEFAULT 0 COMMENT '0:未读,1:已读',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_transaction (transaction_id),
    INDEX idx_sender (sender_username),
    INDEX idx_receiver (receiver_username),
    INDEX idx_is_read (is_read),
    FOREIGN KEY (transaction_id) REFERENCES transactions(id) ON DELETE CASCADE,
    FOREIGN KEY (sender_username) REFERENCES users(username) ON DELETE CASCADE,
    FOREIGN KEY (receiver_username) REFERENCES users(username) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
