// WebFileServer 测试JavaScript代码

document.addEventListener('DOMContentLoaded', function() {
    const testBtn = document.getElementById('testBtn');
    const apiResult = document.getElementById('apiResult');
    
    testBtn.addEventListener('click', function() {
        // 显示加载中状态
        apiResult.textContent = '正在请求API...';
        
        // 测试基本API请求
        fetch('/api/test')
            .then(response => {
                if (!response.ok) {
                    throw new Error('API请求失败: ' + response.status);
                }
                return response.json();
            })
            .then(data => {
                // 格式化JSON输出
                apiResult.textContent = JSON.stringify(data, null, 2);
            })
            .catch(error => {
                apiResult.textContent = '请求错误: ' + error.message;
            });
    });
    
    // 自动加载并显示JSON数据文件
    fetch('data/test.json')
        .then(response => {
            if (!response.ok) {
                console.log('无法加载test.json文件');
                return;
            }
            return response.json();
        })
        .then(data => {
            console.log('成功加载test.json:', data);
        })
        .catch(error => {
            console.error('加载test.json错误:', error);
        });
});