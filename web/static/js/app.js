// 全局变量
let currentUser = null;
let currentItems = [];
let currentTransactions = [];
let currentFavorites = [];

// API基础URL
const API_BASE = 'http://localhost:8080/api';

// 页面加载完成后初始化
document.addEventListener('DOMContentLoaded', function() {
    loadSystemStats();
    loadItems();
    checkAuthStatus();
});

// 显示消息提示
function showMessage(message, type = 'success') {
    const toast = document.getElementById('messageToast');
    const messageText = document.getElementById('messageText');
    
    messageText.textContent = message;
    toast.className = `toast ${type}`;
    toast.classList.add('show');
    
    setTimeout(() => {
        toast.classList.remove('show');
    }, 3000);
}

// 加载我的物品
async function loadMyItems() {
    console.log('loadMyItems - 当前用户:', currentUser);
    
    if (!currentUser) {
        console.log('用户未登录，无法获取我的物品');
        showMessage('请先登录', 'warning');
        return;
    }
    
    console.log('正在获取我的物品，用户名:', currentUser.username);
    console.log('请求URL:', `${API_BASE}/item/owner/${currentUser.username}`);
    
    try {
        const response = await fetch(`${API_BASE}/item/owner/${currentUser.username}`, {
            headers: {
                'Authorization': currentUser.username
            }
        });
        
        console.log('响应状态:', response.status);
        console.log('响应头:', response.headers);
        
        const result = await response.json();
        
        if (result.success) {
            const items = result.data;
            displayMyItems(items);
            updateMyItemsStats(items);
        } else {
            showMessage(`获取我的物品失败: ${result.message}`, 'error');
        }
    } catch (error) {
        console.error('获取我的物品错误:', error);
        showMessage('获取我的物品失败，请检查网络连接', 'error');
    }
}

// 辅助函数
function getCategoryName(category) {
    const categories = {
        1: '电子产品',
        2: '服装',
        3: '图书',
        4: '日用品',
        5: '运动器材'
    };
    return categories[category] || '其他';
}

function getStatusClass(status) {
    const statusClasses = {
        0: 'available',
        1: 'reserved', 
        2: 'sold'
    };
    return statusClasses[status] || 'unknown';
}

function getStatusText(status) {
    const statusTexts = {
        0: '可交易',
        1: '已预订',
        2: '已售出'
    };
    return statusTexts[status] || '未知';
}

// 显示我的物品
function displayMyItems(items) {
    const grid = document.getElementById('myItemsGrid');
    if (!grid) return;
    
    if (items.length === 0) {
        grid.innerHTML = '<div class="empty-state"><i class="fas fa-box-open"></i><p>您还没有发布任何物品</p></div>';
        return;
    }
    
    grid.innerHTML = items.map(item => `
        <div class="item-card">
            <div class="item-image">
                <img src="${item.imagePath || 'web/static/images/default-item.jpg'}" alt="${item.title}">
            </div>
            <div class="item-content">
                <h3>${item.title}</h3>
                <p class="item-description">${item.description}</p>
                <div class="item-meta">
                    <span class="item-price">¥${item.price}</span>
                    <span class="item-category">${getCategoryName(item.category)}</span>
                </div>
                <div class="item-status status-${getStatusClass(item.status)}">
                    ${getStatusText(item.status)}
                </div>
                <div class="item-management">
                    ${item.status === 2 ? '' : `
                        <button class="btn btn-edit" onclick="editItem(${item.id})">
                            <i class="fas fa-edit"></i> 修改
                        </button>
                        <button class="btn btn-delete" onclick="deleteItem(${item.id})">
                            <i class="fas fa-trash"></i> 删除
                        </button>
                    `}
                </div>
            </div>
        </div>
    `).join('');
}

// 更新我的物品统计
function updateMyItemsStats(items) {
    const total = items.length;
    const available = items.filter(item => item.status === 0).length;
    const reserved = items.filter(item => item.status === 1).length;
    const sold = items.filter(item => item.status === 2).length;
    
    document.getElementById('totalItemsCount').textContent = total;
    document.getElementById('availableItemsCount').textContent = available;
    document.getElementById('reservedItemsCount').textContent = reserved;
    document.getElementById('soldItemsCount').textContent = sold;
}

// 编辑物品
function editItem(itemId) {
    // 找到要编辑的物品
    const item = currentItems.find(i => i.id === itemId);
    if (!item) {
        showMessage('物品不存在', 'error');
        return;
    }
    
    // 填充编辑表单
    document.getElementById('editItemTitle').value = item.title;
    document.getElementById('editItemDescription').value = item.description;
    document.getElementById('editItemPrice').value = item.price;
    document.getElementById('editItemCategory').value = item.category;
    document.getElementById('editItemCondition').value = item.condition;
    // 清空文件输入，保存原图片路径到hidden字段
    document.getElementById('editItemImage').value = '';
    document.getElementById('editItemImagePath').value = item.imagePath || '';
    
    // 存储当前编辑的物品ID
    document.getElementById('editItemForm').dataset.itemId = itemId;
    
    // 显示编辑模态框
    showModal('editItemModal');
}

// 更新物品
async function updateItem(event) {
    event.preventDefault();
    
    if (!currentUser) {
        showMessage('请先登录', 'warning');
        return;
    }
    
    const itemId = document.getElementById('editItemForm').dataset.itemId;
    const title = document.getElementById('editItemTitle').value;
    const description = document.getElementById('editItemDescription').value;
    const price = document.getElementById('editItemPrice').value;
    const category = document.getElementById('editItemCategory').value;
    const condition = document.getElementById('editItemCondition').value;
    const imageFile = document.getElementById('editItemImage').files[0];
    const oldImagePath = document.getElementById('editItemImagePath').value;
    
    try {
        // 处理图片文件
        let imagePath = oldImagePath; // 默认保留原图片
        if (imageFile) {
            // 检查文件大小（限制为5MB）
            const maxSize = 5 * 1024 * 1024; // 5MB
            if (imageFile.size > maxSize) {
                showMessage('图片文件过大，请选择小于5MB的图片', 'error');
                return;
            }
            
            // 如果选择了新图片，转换为Base64
            const reader = new FileReader();
            imagePath = await new Promise((resolve, reject) => {
                reader.onload = (e) => resolve(e.target.result);
                reader.onerror = reject;
                reader.readAsDataURL(imageFile);
            });
        }
        
        const response = await fetch(`${API_BASE}/item/${itemId}`, {
            method: 'PUT',
            headers: {
                'Content-Type': 'application/json',
                'Authorization': currentUser.username
            },
            body: JSON.stringify({
                title,
                description,
                price: parseFloat(price),
                category: parseInt(category),
                condition,
                imagePath
            })
        });
        
        const result = await response.json();
        
        if (result.success) {
            showMessage('物品信息更新成功', 'success');
            closeModal('editItemModal');
            loadMyItems(); // 重新加载我的物品
        } else {
            showMessage(result.message || '更新失败', 'error');
        }
    } catch (error) {
        showMessage('更新失败，请检查网络连接', 'error');
    }
}

// 删除物品
async function deleteItem(itemId) {
    if (!confirm('确定要删除这个物品吗？删除后无法恢复。')) {
        return;
    }
    
    if (!currentUser) {
        showMessage('请先登录', 'warning');
        return;
    }
    
    try {
        const response = await fetch(`${API_BASE}/item/${itemId}`, {
            method: 'DELETE',
            headers: {
                'Authorization': currentUser.username
            }
        });
        
        const result = await response.json();
        
        if (result.success) {
            showMessage('物品删除成功', 'success');
            loadMyItems(); // 重新加载我的物品
        } else {
            showMessage(result.message || '删除失败', 'error');
        }
    } catch (error) {
        showMessage('删除失败，请检查网络连接', 'error');
    }
}

// 显示/隐藏模态框
function showModal(modalId) {
    document.getElementById(modalId).style.display = 'block';
}

function closeModal(modalId) {
    document.getElementById(modalId).style.display = 'none';
}

// 切换导航菜单
function toggleNav() {
    const navMenu = document.getElementById('navMenu');
    navMenu.classList.toggle('active');
}

// 显示页面部分
function showSection(sectionId) {
    // 隐藏所有部分
    document.querySelectorAll('.section').forEach(section => {
        section.classList.remove('active');
    });
    
    // 显示选中的部分
    document.getElementById(sectionId).classList.add('active');
    
    // 关闭移动端菜单
    document.getElementById('navMenu').classList.remove('active');
    
    // 根据部分加载相应数据
    switch(sectionId) {
        case 'items':
            loadItems();
            break;
        case 'myitems':
            loadMyItems();
            break;
        case 'transactions':
            loadTransactions();
            break;
        case 'favorites':
            loadFavorites();
            break;
        case 'profile':
            loadProfile();
            break;
    }
}

// 显示登录模态框
function showLoginModal() {
    showModal('loginModal');
}

// 显示注册模态框
function showRegisterModal() {
    showModal('registerModal');
}

// 用户登录
async function login(event) {
    event.preventDefault();
    
    const username = document.getElementById('loginUsername').value;
    const password = document.getElementById('loginPassword').value;
    
    try {
        const response = await fetch(`${API_BASE}/user/login`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({ username, password })
        });
        
        const result = await response.json();
        
        if (result.success) {
            currentUser = result.data;
            localStorage.setItem('currentUser', JSON.stringify(currentUser));
            updateAuthUI();
            closeModal('loginModal');
            showMessage('登录成功！', 'success');
            document.getElementById('loginForm').reset();
        } else {
            showMessage(result.message, 'error');
        }
    } catch (error) {
        showMessage('登录失败，请检查网络连接', 'error');
    }
}

// 用户注册
async function register(event) {
    event.preventDefault();
    
    const username = document.getElementById('registerUsername').value;
    const password = document.getElementById('registerPassword').value;
    const email = document.getElementById('registerEmail').value;
    const phone = document.getElementById('registerPhone').value;
    const studentId = document.getElementById('registerStudentId').value;
    
    try {
        const response = await fetch(`${API_BASE}/user/register`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({ username, password, email, phone, studentId })
        });
        
        const result = await response.json();
        
        if (result.success) {
            closeModal('registerModal');
            showMessage('注册成功！请登录', 'success');
            document.getElementById('registerForm').reset();
        } else {
            showMessage(result.message, 'error');
        }
    } catch (error) {
        showMessage('注册失败，请检查网络连接', 'error');
    }
}

// 用户退出
function logout() {
    currentUser = null;
    localStorage.removeItem('currentUser');
    updateAuthUI();
    showMessage('已退出登录', 'success');
    showSection('home');
}

// 更新认证UI
function updateAuthUI() {
    const navAuth = document.getElementById('navAuth');
    const navUser = document.getElementById('navUser');
    const userInfo = document.getElementById('userInfo');
    
    if (currentUser) {
        navAuth.style.display = 'none';
        navUser.style.display = 'flex';
        userInfo.textContent = `${currentUser.username} (信誉: ${currentUser.creditScore})`;
    } else {
        navAuth.style.display = 'flex';
        navUser.style.display = 'none';
    }
}

// 检查认证状态
async function checkAuthStatus() {
    // 检查本地存储的登录状态
    const savedUser = localStorage.getItem('currentUser');
    if (savedUser) {
        try {
            currentUser = JSON.parse(savedUser);
            updateAuthUI();
        } catch (error) {
            console.error('解析用户数据失败:', error);
            localStorage.removeItem('currentUser');
        }
    }
}

// 加载系统统计
async function loadSystemStats() {
    try {
        // 获取用户数量
        const userResponse = await fetch(`${API_BASE}/user/count`);
        const userResult = await userResponse.json();
        if (userResult.success) {
            document.getElementById('userCount').textContent = userResult.data;
        }
        
        // 获取物品数量
        const itemResponse = await fetch(`${API_BASE}/item/count`);
        const itemResult = await itemResponse.json();
        if (itemResult.success) {
            document.getElementById('itemCount').textContent = itemResult.data;
        }
        
        // 获取交易数量
        const transactionResponse = await fetch(`${API_BASE}/transaction/count`);
        console.log('交易计数响应状态:', transactionResponse.status);
        const transactionResult = await transactionResponse.json();
        console.log('交易计数结果:', transactionResult);
        if (transactionResult.success) {
            document.getElementById('transactionCount').textContent = transactionResult.data;
        } else {
            console.error('交易计数失败:', transactionResult.message);
        }
    } catch (error) {
        console.error('加载统计数据失败:', error);
        // 如果API调用失败，显示默认值
        document.getElementById('userCount').textContent = '0';
        document.getElementById('itemCount').textContent = '0';
        document.getElementById('transactionCount').textContent = '0';
    }
}

// 加载物品列表
async function loadItems() {
    console.log('loadItems函数被调用');
    try {
        console.log('正在请求API:', `${API_BASE}/item`);
        const response = await fetch(`${API_BASE}/item`);
        console.log('响应状态:', response.status);
        
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }
        
        const result = await response.json();
        console.log('响应结果:', result);
        
        if (result.success) {
            currentItems = result.data;
            console.log('成功获取物品数量:', currentItems.length);
            displayItems(currentItems);
        } else {
            console.error('API返回失败:', result.message);
            showMessage('加载物品列表失败', 'error');
        }
    } catch (error) {
        console.error('加载物品列表错误:', error);
        console.error('错误详情:', error.message);
        showMessage('加载物品列表失败，请检查网络连接', 'error');
    }
}

// 显示物品列表
function displayItems(items) {
    const itemsGrid = document.getElementById('itemsGrid');
    
    if (items.length === 0) {
        itemsGrid.innerHTML = '<p style="text-align: center; color: #6c757d; grid-column: 1/-1;">暂无物品</p>';
        return;
    }
    
    itemsGrid.innerHTML = items.map(item => `
        <div class="item-card">
            <div class="item-image">
                ${item.imagePath ? `<img src="${item.imagePath}" alt="${item.title}">` : '<i class="fas fa-image"></i>'}
            </div>
            <div class="item-content">
                <h3 class="item-title">${item.title}</h3>
                <p class="item-description">${item.description}</p>
                <div class="item-owner">
                    <i class="fas fa-user"></i>
                    <span class="owner-name">${item.ownerUsername || '未知用户'}</span>
                </div>
                <div class="item-meta">
                    <span class="item-price">¥${item.price}</span>
                    <span class="item-status status-${getStatusClass(item.status)} ${item.status === 2 ? 'status-sold-highlight' : item.status === 0 ? 'status-available-highlight' : ''}">${getStatusText(item.status)}</span>
                </div>
                <div class="item-actions">
                    <button class="btn btn-primary" onclick="showItemDetail(${item.id})">查看详情</button>
                    ${currentUser && item.ownerUsername !== currentUser.username ? `
                        <button class="btn btn-success" onclick="addToFavorites(${item.id})">收藏</button>
                    ` : ''}
                </div>
            </div>
        </div>
    `).join('');
}

// 搜索物品
async function searchItems() {
    const keyword = document.getElementById('searchInput').value.trim();
    
    if (!keyword) {
        loadItems();
        return;
    }
    
    try {
        const response = await fetch(`${API_BASE}/item/search?keyword=${encodeURIComponent(keyword)}`);
        const result = await response.json();
        
        if (result.success) {
            currentItems = result.data;
            displayItems(currentItems);
        } else {
            showMessage('搜索失败', 'error');
        }
    } catch (error) {
        showMessage('搜索失败，请检查网络连接', 'error');
    }
}

// 筛选物品
async function filterItems() {
    const categoryFilter = document.getElementById('categoryFilter').value;
    const statusFilter = document.getElementById('statusFilter').value;
    const minPriceFilter = document.getElementById('minPriceFilter').value;
    const maxPriceFilter = document.getElementById('maxPriceFilter').value;
    const timeFilter = document.getElementById('timeFilter').value;
    
    // 构建查询参数
    const params = new URLSearchParams();
    
    if (categoryFilter) {
        params.append('category', categoryFilter);
    }
    
    if (statusFilter) {
        params.append('status', statusFilter);
    }
    
    if (minPriceFilter) {
        params.append('minPrice', minPriceFilter);
    }
    
    if (maxPriceFilter) {
        params.append('maxPrice', maxPriceFilter);
    }
    
    // 处理时间筛选
    if (timeFilter) {
        const now = new Date();
        let startTime, endTime;
        
        switch (timeFilter) {
            case 'today':
                startTime = new Date(now.getFullYear(), now.getMonth(), now.getDate());
                endTime = new Date(now.getFullYear(), now.getMonth(), now.getDate() + 1);
                break;
            case 'week':
                startTime = new Date(now.getTime() - 7 * 24 * 60 * 60 * 1000);
                endTime = now;
                break;
            case 'month':
                startTime = new Date(now.getTime() - 30 * 24 * 60 * 60 * 1000);
                endTime = now;
                break;
            case '3months':
                startTime = new Date(now.getTime() - 90 * 24 * 60 * 60 * 1000);
                endTime = now;
                break;
        }
        
        if (startTime && endTime) {
            params.append('startTime', Math.floor(startTime.getTime() / 1000));
            params.append('endTime', Math.floor(endTime.getTime() / 1000));
        }
    }
    
    try {
        const queryString = params.toString();
        const url = queryString ? `${API_BASE}/item?${queryString}` : `${API_BASE}/item`;
        
        const response = await fetch(url);
        const result = await response.json();
        
        if (result.success) {
            currentItems = result.data;
            displayItems(currentItems);
        } else {
            showMessage('筛选失败', 'error');
        }
    } catch (error) {
        showMessage('筛选失败，请检查网络连接', 'error');
    }
}

// 清除所有筛选
function clearFilters() {
    document.getElementById('categoryFilter').value = '';
    document.getElementById('statusFilter').value = '';
    document.getElementById('minPriceFilter').value = '';
    document.getElementById('maxPriceFilter').value = '';
    document.getElementById('timeFilter').value = '';
    document.getElementById('searchInput').value = '';
    
    // 重新加载所有物品
    loadItems();
}

// 判断两个日期是否是同一天
function isSameDay(date1, date2) {
    return date1.getFullYear() === date2.getFullYear() &&
           date1.getMonth() === date2.getMonth() &&
           date1.getDate() === date2.getDate();
}

// 显示物品详情
async function showItemDetail(itemId) {
    try {
        const response = await fetch(`${API_BASE}/item/${itemId}`);
        const result = await response.json();
        
        if (result.success) {
            const item = result.data;
            const modalTitle = document.getElementById('itemModalTitle');
            const modalContent = document.getElementById('itemModalContent');
            
            modalTitle.textContent = item.title;
            modalContent.innerHTML = `
                <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 2rem;">
                    <div>
                        <div class="item-image" style="height: 300px;">
                            ${item.imagePath ? `<img src="${item.imagePath}" alt="${item.title}">` : '<i class="fas fa-image"></i>'}
                        </div>
                    </div>
                    <div>
                        <h3>${item.title}</h3>
                        <p><strong>价格:</strong> ¥${item.price}</p>
                        <p><strong>类别:</strong> ${item.categoryName}</p>
                        <p><strong>状态:</strong> ${item.statusName}</p>
                        <p><strong>发布者:</strong> ${item.ownerUsername}</p>
                        <p><strong>使用情况:</strong> ${item.condition}</p>
                        <p><strong>描述:</strong></p>
                        <p>${item.description}</p>
                        <div style="margin-top: 1rem;">
                            ${currentUser && item.ownerUsername !== currentUser.username ? `
                                <button class="btn btn-primary" onclick="createTransaction(${item.id})">创建交易</button>
                            ` : ''}
                            <button class="btn btn-secondary" onclick="closeModal('itemModal')">关闭</button>
                        </div>
                    </div>
                </div>
            `;
            
            showModal('itemModal');
        } else {
            showMessage('获取物品详情失败', 'error');
        }
    } catch (error) {
        showMessage('获取物品详情失败，请检查网络连接', 'error');
    }
}

// 发布物品
async function publishItem(event) {
    event.preventDefault();
    
    console.log('发布物品 - 当前用户:', currentUser);
    
    if (!currentUser) {
        console.log('用户未登录，无法发布物品');
        showMessage('请先登录', 'warning');
        return;
    }
    
    const title = document.getElementById('itemTitle').value;
    const description = document.getElementById('itemDescription').value;
    const price = document.getElementById('itemPrice').value;
    const category = document.getElementById('itemCategory').value;
    const condition = document.getElementById('itemCondition').value;
    const imageFile = document.getElementById('itemImage').files[0];
    
    try {
        // 处理图片文件
        let imagePath = '';
        if (imageFile) {
            // 检查文件大小（限制为500KB，暂时用较小的限制）
            const maxSize = 500 * 1024; // 500KB
            if (imageFile.size > maxSize) {
                showMessage('图片文件过大，请选择小于500KB的图片（建议压缩后上传）', 'error');
                return;
            }
            
            console.log('准备上传图片，大小:', imageFile.size, '字节');
            
            // 将图片转换为Base64
            const reader = new FileReader();
            imagePath = await new Promise((resolve, reject) => {
                reader.onload = (e) => {
                    console.log('图片Base64长度:', e.target.result.length);
                    resolve(e.target.result);
                };
                reader.onerror = reject;
                reader.readAsDataURL(imageFile);
            });
        }
        
        const requestData = {
            title,
            description,
            price: parseFloat(price),
            category: parseInt(category),
            condition,
            imagePath
        };
        
        const jsonString = JSON.stringify(requestData);
        console.log('请求数据大小:', jsonString.length, '字节');
        
        const response = await fetch(`${API_BASE}/item`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'Authorization': currentUser.username
            },
            body: jsonString
        });
        
        const result = await response.json();
        
        if (result.success) {
            showMessage('物品发布成功！', 'success');
            document.getElementById('publishForm').reset();
            loadItems();
        } else {
            showMessage(result.message, 'error');
        }
    } catch (error) {
        showMessage('发布物品失败，请检查网络连接', 'error');
    }
}

// 创建交易
async function createTransaction(itemId) {
    if (!currentUser) {
        showMessage('请先登录', 'warning');
        return;
    }
    
    try {
        const response = await fetch(`${API_BASE}/transaction`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'Authorization': currentUser.username
            },
            body: JSON.stringify({ itemId })
        });
        
        const result = await response.json();
        
        if (result.success) {
            showMessage('交易创建成功！', 'success');
            closeModal('itemModal');
            loadTransactions();
        } else {
            showMessage(result.message, 'error');
        }
    } catch (error) {
        showMessage('创建交易失败，请检查网络连接', 'error');
    }
}

// 添加到收藏
async function addToFavorites(itemId) {
    if (!currentUser) {
        showMessage('请先登录', 'warning');
        return;
    }
    
    try {
        const response = await fetch(`${API_BASE}/favorite`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'Authorization': currentUser.username
            },
            body: JSON.stringify({ itemId })
        });
        
        const result = await response.json();
        
        if (result.success) {
            showMessage('已添加到收藏！', 'success');
        } else {
            showMessage(result.message, 'error');
        }
    } catch (error) {
        showMessage('添加收藏失败，请检查网络连接', 'error');
    }
}

// 加载交易列表
async function loadTransactions() {
    console.log('loadTransactions - 当前用户:', currentUser);
    
    if (!currentUser) {
        console.log('用户未登录，无法获取交易列表');
        showMessage('请先登录', 'warning');
        return;
    }
    
    console.log('正在获取交易列表，用户名:', currentUser.username);
    console.log('请求URL:', `${API_BASE}/transaction`);
    
    try {
        const response = await fetch(`${API_BASE}/transaction`, {
            headers: {
                'Authorization': currentUser.username
            }
        });
        
        console.log('响应状态:', response.status);
        console.log('响应OK:', response.ok);
        
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }
        
        const result = await response.json();
        
        if (result.success) {
            currentTransactions = result.data;
            displayTransactions(currentTransactions);
        } else {
            showMessage('加载交易列表失败', 'error');
        }
    } catch (error) {
        showMessage('加载交易列表失败，请检查网络连接', 'error');
    }
}

// 显示交易列表
function displayTransactions(transactions) {
    const transactionsList = document.getElementById('transactionsList');
    
    if (transactions.length === 0) {
        transactionsList.innerHTML = '<p style="text-align: center; color: #6c757d;">暂无交易记录</p>';
        return;
    }
    
    transactionsList.innerHTML = transactions.map(transaction => `
        <div class="transaction-card">
            <div class="transaction-header">
                <span class="transaction-id">交易 #${transaction.id}</span>
                <span class="transaction-status status-${transaction.statusName.toLowerCase().replace('已', '').replace('待', 'pending')}">${transaction.statusName}</span>
            </div>
            <div class="transaction-details">
                <div class="transaction-detail">
                    <label>物品ID</label>
                    <span>${transaction.itemId}</span>
                </div>
                <div class="transaction-detail">
                    <label>卖家</label>
                    <span>${transaction.sellerUsername}</span>
                </div>
                <div class="transaction-detail">
                    <label>买家</label>
                    <span>${transaction.buyerUsername}</span>
                </div>
                <div class="transaction-detail">
                    <label>价格</label>
                    <span>¥${transaction.price}</span>
                </div>
            </div>
            <div class="transaction-actions">
                <button class="btn btn-primary" onclick="showTransactionDetail(${transaction.id})">查看详情</button>
                <button class="btn btn-info" onclick="showMessageModal(${transaction.id})">
                    <i class="fas fa-comments"></i> 留言
                </button>
                ${transaction.status === 0 ? `
                    <button class="btn btn-success" onclick="updateTransactionStatus(${transaction.id}, 1)">确认交易</button>
                ` : ''}
                ${transaction.status === 1 ? `
                    <button class="btn btn-warning" onclick="updateTransactionStatus(${transaction.id}, 2)">完成交易</button>
                ` : ''}
                ${transaction.status < 2 ? `
                    <button class="btn btn-danger" onclick="updateTransactionStatus(${transaction.id}, 3)">取消交易</button>
                ` : ''}
            </div>
        </div>
    `).join('');
}

// 显示交易详情
async function showTransactionDetail(transactionId) {
    // 简化处理，直接显示交易信息
    const transaction = currentTransactions.find(t => t.id === transactionId);
    if (transaction) {
        const modalTitle = document.getElementById('transactionModalTitle');
        const modalContent = document.getElementById('transactionModalContent');
        
        modalTitle.textContent = `交易详情 #${transaction.id}`;
        modalContent.innerHTML = `
            <div style="display: grid; gap: 1rem;">
                <div><strong>交易ID:</strong> ${transaction.id}</div>
                <div><strong>物品ID:</strong> ${transaction.itemId}</div>
                <div><strong>卖家:</strong> ${transaction.sellerUsername}</div>
                <div><strong>买家:</strong> ${transaction.buyerUsername}</div>
                <div><strong>价格:</strong> ¥${transaction.price}</div>
                <div><strong>状态:</strong> ${transaction.statusName}</div>
                <div><strong>创建时间:</strong> ${new Date(transaction.createTime * 1000).toLocaleString()}</div>
                ${transaction.completeTime > 0 ? `
                    <div><strong>完成时间:</strong> ${new Date(transaction.completeTime * 1000).toLocaleString()}</div>
                ` : ''}
            </div>
            <div style="margin-top: 2rem;">
                <button class="btn btn-secondary" onclick="closeModal('transactionModal')">关闭</button>
            </div>
        `;
        
        showModal('transactionModal');
    }
}

// 更新交易状态
async function updateTransactionStatus(transactionId, status) {
    try {
        const response = await fetch(`${API_BASE}/transaction/${transactionId}`, {
            method: 'PUT',
            headers: {
                'Content-Type': 'application/json',
                'Authorization': currentUser.username
            },
            body: JSON.stringify({ status })
        });
        
        const result = await response.json();
        
        if (result.success) {
            showMessage('交易状态更新成功！', 'success');
            loadTransactions();
        } else {
            showMessage(result.message, 'error');
        }
    } catch (error) {
        showMessage('更新交易状态失败，请检查网络连接', 'error');
    }
}

// 加载收藏列表
async function loadFavorites() {
    if (!currentUser) {
        showMessage('请先登录', 'warning');
        return;
    }
    
    try {
        const response = await fetch(`${API_BASE}/favorite`, {
            headers: {
                'Authorization': currentUser.username
            }
        });
        
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }
        
        const result = await response.json();
        
        if (result.success) {
            currentFavorites = result.data;
            displayFavorites(currentFavorites);
        } else {
            showMessage('加载收藏列表失败', 'error');
        }
    } catch (error) {
        showMessage('加载收藏列表失败，请检查网络连接', 'error');
    }
}

// 显示收藏列表
function displayFavorites(favorites) {
    const favoritesGrid = document.getElementById('favoritesGrid');
    
    if (favorites.length === 0) {
        favoritesGrid.innerHTML = '<p style="text-align: center; color: #6c757d; grid-column: 1/-1;">暂无收藏物品</p>';
        return;
    }
    
    favoritesGrid.innerHTML = favorites.map(item => `
        <div class="item-card">
            <div class="item-image">
                ${item.imagePath ? `<img src="${item.imagePath}" alt="${item.title}">` : '<i class="fas fa-image"></i>'}
            </div>
            <div class="item-content">
                <h3 class="item-title">${item.title}</h3>
                <p class="item-description">${item.description}</p>
                <div class="item-meta">
                    <span class="item-price">¥${item.price}</span>
                    <span class="item-status status-${item.statusName.toLowerCase().replace('已', '').replace('可', 'available')}">${item.statusName}</span>
                </div>
                <div class="item-actions">
                    <button class="btn btn-primary" onclick="showItemDetail(${item.id})">查看详情</button>
                    <button class="btn btn-danger" onclick="removeFromFavorites(${item.id})">移除收藏</button>
                </div>
            </div>
        </div>
    `).join('');
}

// 从收藏中移除
async function removeFromFavorites(itemId) {
    try {
        const response = await fetch(`${API_BASE}/favorite/${itemId}`, {
            method: 'DELETE',
            headers: {
                'Authorization': currentUser.username
            }
        });
        
        const result = await response.json();
        
        if (result.success) {
            showMessage('已从收藏中移除！', 'success');
            loadFavorites();
        } else {
            showMessage(result.message, 'error');
        }
    } catch (error) {
        showMessage('移除收藏失败，请检查网络连接', 'error');
    }
}

// 加载个人资料
async function loadProfile() {
    if (!currentUser) {
        showMessage('请先登录', 'warning');
        return;
    }
    
    const profileInfo = document.getElementById('profileInfo');
    profileInfo.innerHTML = `
        <h3>个人信息</h3>
        <div style="display: grid; gap: 1rem; margin-top: 1rem;">
            <div><strong>用户名:</strong> ${currentUser.username}</div>
            <div><strong>邮箱:</strong> ${currentUser.email}</div>
            <div><strong>电话:</strong> ${currentUser.phone}</div>
            <div><strong>学号:</strong> ${currentUser.studentId}</div>
            <div><strong>信誉评分:</strong> ${currentUser.creditScore}</div>
        </div>
    `;
    
    // 加载交易数据以确保统计正确
    try {
        const transactionResponse = await fetch(`${API_BASE}/transaction`, {
            headers: {
                'Authorization': currentUser.username
            }
        });
        
        if (transactionResponse.ok) {
            const transactionResult = await transactionResponse.json();
            if (transactionResult.success) {
                currentTransactions = transactionResult.data;
            }
        }
    } catch (error) {
        console.error('加载交易数据失败:', error);
    }
    
    // 加载收藏数据以确保统计正确
    try {
        const favoriteResponse = await fetch(`${API_BASE}/favorite`, {
            headers: {
                'Authorization': currentUser.username
            }
        });
        
        if (favoriteResponse.ok) {
            const favoriteResult = await favoriteResponse.json();
            if (favoriteResult.success) {
                currentFavorites = favoriteResult.data;
            }
        }
    } catch (error) {
        console.error('加载收藏数据失败:', error);
    }
    
    // 更新统计信息
    document.getElementById('myItemCount').textContent = currentItems.filter(item => item.ownerUsername === currentUser.username).length;
    // 只统计已完成的交易（status === 2）
    document.getElementById('myTransactionCount').textContent = currentTransactions.filter(t => t.status === 2).length;
    document.getElementById('myFavoriteCount').textContent = currentFavorites.length;
    document.getElementById('myCreditScore').textContent = currentUser.creditScore;
}

// 点击模态框外部关闭
window.onclick = function(event) {
    const modals = document.querySelectorAll('.modal');
    modals.forEach(modal => {
        if (event.target === modal) {
            modal.style.display = 'none';
        }
    });
}

// 回车键搜索
document.getElementById('searchInput').addEventListener('keypress', function(event) {
    if (event.key === 'Enter') {
        searchItems();
    }
});

// ==================== 留言功能 ====================

let currentMessageTransactionId = null;
let messageRefreshInterval = null;

// 显示留言模态框
async function showMessageModal(transactionId) {
    if (!currentUser) {
        showMessage('请先登录', 'warning');
        return;
    }
    
    currentMessageTransactionId = transactionId;
    
    // 获取交易信息
    const transaction = currentTransactions.find(t => t.id === transactionId);
    if (!transaction) {
        showMessage('交易不存在', 'error');
        return;
    }
    
    // 获取物品信息
    const item = currentItems.find(i => i.id === transaction.itemId);
    const itemTitle = item ? item.title : '未知物品';
    
    // 确定对方用户
    const otherUser = transaction.buyerUsername === currentUser.username 
        ? transaction.sellerUsername 
        : transaction.buyerUsername;
    
    // 填充交易信息
    document.getElementById('messageTransactionId').textContent = transactionId;
    document.getElementById('messageItemTitle').textContent = itemTitle;
    document.getElementById('messageOtherUser').textContent = otherUser;
    
    // 清空输入框
    document.getElementById('messageInput').value = '';
    
    // 加载消息列表
    await loadMessages(transactionId);
    
    // 显示模态框
    document.getElementById('messageModal').style.display = 'block';
    
    // 启动自动刷新（每5秒刷新一次）
    if (messageRefreshInterval) {
        clearInterval(messageRefreshInterval);
    }
    messageRefreshInterval = setInterval(() => {
        loadMessages(transactionId);
    }, 5000);
}

// 加载消息列表
async function loadMessages(transactionId) {
    try {
        const response = await fetch(`${API_BASE}/message/transaction/${transactionId}`, {
            headers: {
                'Authorization': currentUser.username
            }
        });
        
        const result = await response.json();
        
        if (result.success) {
            displayMessages(result.data);
        } else {
            console.error('加载消息失败:', result.message);
        }
    } catch (error) {
        console.error('加载消息失败:', error);
    }
}

// 显示消息列表
function displayMessages(messages) {
    const messagesContainer = document.getElementById('messagesContainer');
    
    if (messages.length === 0) {
        messagesContainer.innerHTML = '<p style="text-align: center; color: #6c757d; padding: 20px;">暂无留言</p>';
        return;
    }
    
    messagesContainer.innerHTML = messages.map(msg => {
        const isSender = msg.senderUsername === currentUser.username;
        const messageClass = isSender ? 'message-item message-sent' : 'message-item message-received';
        const time = new Date(msg.sendTime * 1000).toLocaleString('zh-CN');
        
        return `
            <div class="${messageClass}">
                <div class="message-header">
                    <span class="message-user">${msg.senderUsername}</span>
                    <span class="message-time">${time}</span>
                </div>
                <div class="message-content">${escapeHtml(msg.content)}</div>
            </div>
        `;
    }).join('');
    
    // 滚动到底部
    messagesContainer.scrollTop = messagesContainer.scrollHeight;
}

// 发送消息
async function sendMessage() {
    const content = document.getElementById('messageInput').value.trim();
    
    if (!content) {
        showMessage('请输入留言内容', 'warning');
        return;
    }
    
    if (!currentMessageTransactionId) {
        showMessage('交易信息错误', 'error');
        return;
    }
    
    try {
        const response = await fetch(`${API_BASE}/message`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'Authorization': currentUser.username
            },
            body: JSON.stringify({
                transactionId: currentMessageTransactionId,
                content: content
            })
        });
        
        const result = await response.json();
        
        if (result.success) {
            // 清空输入框
            document.getElementById('messageInput').value = '';
            
            // 重新加载消息列表
            await loadMessages(currentMessageTransactionId);
            
            showMessage('发送成功', 'success');
        } else {
            showMessage(result.message || '发送失败', 'error');
        }
    } catch (error) {
        console.error('发送消息失败:', error);
        showMessage('发送失败，请检查网络连接', 'error');
    }
}

// 关闭留言模态框时清除刷新定时器
const originalCloseModal = closeModal;
closeModal = function(modalId) {
    if (modalId === 'messageModal' && messageRefreshInterval) {
        clearInterval(messageRefreshInterval);
        messageRefreshInterval = null;
        currentMessageTransactionId = null;
    }
    originalCloseModal(modalId);
};

// HTML转义函数
function escapeHtml(text) {
    const map = {
        '&': '&amp;',
        '<': '&lt;',
        '>': '&gt;',
        '"': '&quot;',
        "'": '&#039;'
    };
    return text.replace(/[&<>"']/g, m => map[m]);
}
