// 页面加载完成后执行
window.addEventListener('DOMContentLoaded', function() {
    console.log('WebServer页面加载完成');
    
    // 初始化测试导航功能
    initTestNavigation();
    
    // 初始化表单测试
    initFormTest();
    
    // 初始化API测试工具
    initApiTest();
    
    // 初始化性能测试
    initPerformanceTest();
    
    // 添加一些页面交互效果
    initPageEffects();
});

// 测试导航功能
function initTestNavigation() {
    const navBtns = document.querySelectorAll('.nav-btn');
    const testSections = document.querySelectorAll('.test-section');
    
    navBtns.forEach(btn => {
        btn.addEventListener('click', function() {
            // 移除所有活动状态
            navBtns.forEach(b => b.classList.remove('active'));
            testSections.forEach(section => section.classList.add('hidden'));
            
            // 设置当前按钮为活动状态
            this.classList.add('active');
            
            // 显示对应的测试区块
            const target = this.getAttribute('data-target');
            const targetSection = document.getElementById(target);
            if (targetSection) {
                targetSection.classList.remove('hidden');
            }
        });
    });
}

// 表单测试功能
function initFormTest() {
    const form = document.getElementById('submit-form');
    const resultBox = document.getElementById('form-result');
    
    if (form && resultBox) {
        form.addEventListener('submit', function(e) {
            e.preventDefault();
            
            // 简单的表单验证
            const name = document.getElementById('name').value;
            const email = document.getElementById('email').value;
            
            if (!name || !email) {
                showResult(resultBox, '请填写所有必填字段', 'error');
                return;
            }
            
            // 验证邮箱格式
            const emailRegex = /^[a-zA-Z0-9._-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,6}$/;
            if (!emailRegex.test(email)) {
                showResult(resultBox, '请输入有效的邮箱地址', 'error');
                return;
            }
            
            // 创建FormData对象
            const formData = new FormData(this);
            
            // 显示加载状态
            showResult(resultBox, '正在提交表单...', 'loading');
            
            // 发送表单数据
            fetch(this.action, {
                method: this.method,
                body: formData
            })
            .then(response => {
                if (!response.ok) {
                    throw new Error(`HTTP错误! 状态码: ${response.status}`);
                }
                return response.text();
            })
            .then(data => {
                showResult(resultBox, `表单提交成功!\n服务器响应: ${data}`, 'success');
                // 重置表单
                form.reset();
            })
            .catch(error => {
                showResult(resultBox, `提交失败: ${error.message}`, 'error');
            });
        });
    }
}

// API测试工具功能
function initApiTest() {
    const sendBtn = document.getElementById('api-send-btn');
    const methodSelect = document.getElementById('api-method');
    const endpointInput = document.getElementById('api-endpoint');
    const headersText = document.getElementById('api-headers-text');
    const bodyText = document.getElementById('api-body-text');
    const responseStatus = document.getElementById('response-status');
    const responseBody = document.getElementById('response-body');
    
    if (sendBtn && methodSelect && endpointInput && headersText && bodyText && responseStatus && responseBody) {
        sendBtn.addEventListener('click', function() {
            const method = methodSelect.value;
            const endpoint = endpointInput.value;
            let headers = {};
            let body = null;
            
            // 解析请求头
            try {
                headers = JSON.parse(headersText.value);
            } catch (error) {
                showApiResponse(responseStatus, responseBody, '错误', `请求头解析失败: ${error.message}`);
                return;
            }
            
            // 解析请求体
            if (bodyText.value.trim() !== '') {
                try {
                    body = JSON.parse(bodyText.value);
                } catch (error) {
                    showApiResponse(responseStatus, responseBody, '错误', `请求体解析失败: ${error.message}`);
                    return;
                }
            }
            
            // 显示加载状态
            showApiResponse(responseStatus, responseBody, '加载中...', '正在发送请求...');
            
            // 发送API请求
            fetch(endpoint, {
                method: method,
                headers: headers,
                body: body ? JSON.stringify(body) : null
            })
            .then(response => {
                const status = response.status;
                
                // 尝试解析JSON响应
                return response.text().then(text => {
                    try {
                        const data = JSON.parse(text);
                        return { status, data: JSON.stringify(data, null, 2) };
                    } catch (e) {
                        // 如果不是JSON，返回原始文本
                        return { status, data: text };
                    }
                });
            })
            .then(result => {
                showApiResponse(responseStatus, responseBody, result.status, result.data);
            })
            .catch(error => {
                showApiResponse(responseStatus, responseBody, '错误', `请求失败: ${error.message}`);
            });
        });
    }
}

// 性能测试功能
function initPerformanceTest() {
    const startTestBtn = document.getElementById('start-performance-test');
    const testUrlInput = document.getElementById('test-url');
    const requestCountInput = document.getElementById('request-count');
    const concurrencyInput = document.getElementById('concurrency');
    const performanceResults = document.getElementById('performance-results');
    
    if (startTestBtn && testUrlInput && requestCountInput && concurrencyInput && performanceResults) {
        startTestBtn.addEventListener('click', function() {
            const url = testUrlInput.value;
            const requestCount = parseInt(requestCountInput.value);
            const concurrency = parseInt(concurrencyInput.value);
            
            if (!url) {
                alert('请输入测试URL');
                return;
            }
            
            if (isNaN(requestCount) || requestCount <= 0) {
                alert('请输入有效的请求次数');
                return;
            }
            
            if (isNaN(concurrency) || concurrency <= 0) {
                alert('请输入有效的并发数');
                return;
            }
            
            // 禁用按钮防止重复点击
            startTestBtn.disabled = true;
            startTestBtn.textContent = '测试进行中...';
            
            // 显示结果区域
            performanceResults.classList.remove('hidden');
            
            // 执行性能测试
            runPerformanceTest(url, requestCount, concurrency)
            .then(results => {
                // 更新结果显示
                document.getElementById('total-requests').textContent = results.totalRequests;
                document.getElementById('successful-requests').textContent = results.successfulRequests;
                document.getElementById('failed-requests').textContent = results.failedRequests;
                document.getElementById('avg-response-time').textContent = results.avgResponseTime.toFixed(2);
                document.getElementById('min-response-time').textContent = results.minResponseTime;
                document.getElementById('max-response-time').textContent = results.maxResponseTime;
                document.getElementById('throughput').textContent = results.throughput.toFixed(2);
                
                // 创建响应时间图表
                createResponseTimeChart(results.responseTimes);
            })
            .finally(() => {
                // 恢复按钮状态
                startTestBtn.disabled = false;
                startTestBtn.textContent = '开始测试';
            });
        });
    }
}

// 运行性能测试
async function runPerformanceTest(url, requestCount, concurrency) {
    const results = {
        totalRequests: requestCount,
        successfulRequests: 0,
        failedRequests: 0,
        avgResponseTime: 0,
        minResponseTime: Infinity,
        maxResponseTime: 0,
        throughput: 0,
        responseTimes: []
    };
    
    const startTime = Date.now();
    
    // 创建请求队列
    const requestQueue = Array.from({ length: requestCount }, (_, i) => i);
    
    // 使用并发控制执行请求
    const activeRequests = new Set();
    
    while (requestQueue.length > 0 || activeRequests.size > 0) {
        // 启动新的请求，直到达到并发限制或队列为空
        while (activeRequests.size < concurrency && requestQueue.length > 0) {
            const requestId = requestQueue.shift();
            const requestPromise = fetchWithTiming(url, requestId);
            
            activeRequests.add(requestPromise);
            
            requestPromise
            .then(({ success, time }) => {
                activeRequests.delete(requestPromise);
                
                if (success) {
                    results.successfulRequests++;
                    results.responseTimes.push(time);
                    
                    results.minResponseTime = Math.min(results.minResponseTime, time);
                    results.maxResponseTime = Math.max(results.maxResponseTime, time);
                } else {
                    results.failedRequests++;
                }
            })
            .catch(() => {
                activeRequests.delete(requestPromise);
                results.failedRequests++;
            });
        }
        
        if (activeRequests.size > 0) {
            // 等待任一请求完成
            await Promise.race(Array.from(activeRequests));
        }
    }
    
    const endTime = Date.now();
    const totalTime = endTime - startTime;
    
    // 计算平均响应时间和吞吐量
    if (results.successfulRequests > 0) {
        results.avgResponseTime = results.responseTimes.reduce((sum, time) => sum + time, 0) / results.successfulRequests;
    }
    
    results.throughput = (results.totalRequests / (totalTime / 1000));
    
    return results;
}

// 带计时的fetch请求
async function fetchWithTiming(url, requestId) {
    const startTime = performance.now();
    
    try {
        const response = await fetch(url, {
            method: 'GET',
            cache: 'no-cache'
        });
        
        const endTime = performance.now();
        const time = Math.round(endTime - startTime);
        
        return { success: response.ok, time };
    } catch (error) {
        const endTime = performance.now();
        const time = Math.round(endTime - startTime);
        
        return { success: false, time };
    }
}

// 创建响应时间图表
function createResponseTimeChart(responseTimes) {
    const ctx = document.getElementById('response-time-chart').getContext('2d');
    
    // 销毁可能存在的旧图表
    if (window.responseTimeChart) {
        window.responseTimeChart.destroy();
    }
    
    // 准备数据
    const labels = responseTimes.map((_, index) => index + 1);
    
    // 创建新图表
    window.responseTimeChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: labels,
            datasets: [{
                label: '响应时间(ms)',
                data: responseTimes,
                borderColor: '#4CAF50',
                backgroundColor: 'rgba(76, 175, 80, 0.1)',
                borderWidth: 2,
                pointRadius: 3,
                pointBackgroundColor: '#4CAF50',
                tension: 0.1,
                fill: true
            }]
        },
        options: {
            responsive: true,
            plugins: {
                legend: {
                    position: 'top',
                },
                tooltip: {
                    mode: 'index',
                    intersect: false,
                }
            },
            scales: {
                y: {
                    beginAtZero: true,
                    title: {
                        display: true,
                        text: '响应时间(毫秒)'
                    }
                },
                x: {
                    title: {
                        display: true,
                        text: '请求序号'
                    }
                }
            }
        }
    });
}

// 显示结果信息
function showResult(resultBox, message, type = 'info') {
    resultBox.classList.remove('hidden');
    resultBox.innerHTML = message.replace(/\n/g, '<br>');
    
    // 根据类型设置样式
    resultBox.className = 'result-box';
    
    switch (type) {
        case 'success':
            resultBox.classList.add('success');
            resultBox.style.borderLeftColor = '#4CAF50';
            break;
        case 'error':
            resultBox.classList.add('error');
            resultBox.style.borderLeftColor = '#f44336';
            break;
        case 'loading':
            resultBox.classList.add('loading');
            resultBox.style.borderLeftColor = '#2196F3';
            break;
        default:
            resultBox.style.borderLeftColor = '#4CAF50';
    }
}

// 显示API响应
function showApiResponse(statusElement, bodyElement, status, body) {
    statusElement.textContent = status;
    bodyElement.textContent = body;
    
    // 根据状态码设置样式
    if (typeof status === 'number') {
        if (status >= 200 && status < 300) {
            statusElement.style.color = '#4CAF50';
        } else if (status >= 400) {
            statusElement.style.color = '#f44336';
        } else {
            statusElement.style.color = '#FF9800';
        }
    } else {
        statusElement.style.color = status === '错误' ? '#f44336' : '#2196F3';
    }
}

// 页面效果初始化
function initPageEffects() {
    // 添加一些页面交互效果
    const sections = document.querySelectorAll('section');
    sections.forEach(section => {
        section.addEventListener('mouseenter', function() {
            this.style.transform = 'translateY(-5px)';
            this.style.boxShadow = '0 5px 15px rgba(0, 0, 0, 0.1)';
            this.style.transition = 'all 0.3s ease';
        });
        
        section.addEventListener('mouseleave', function() {
            this.style.transform = 'translateY(0)';
            this.style.boxShadow = '0 2px 10px rgba(0, 0, 0, 0.1)';
        });
    });
    
    // 添加页面滚动效果
    window.addEventListener('scroll', function() {
        const header = document.querySelector('header');
        if (window.scrollY > 100) {
            header.style.padding = '1rem 2rem';
            header.style.transition = 'padding 0.3s ease';
        } else {
            header.style.padding = '2rem';
        }
    });
    
    // 添加一个简单的动画效果
    function animateElements() {
        const elements = document.querySelectorAll('h2, h3, p, li');
        elements.forEach((element, index) => {
            setTimeout(() => {
                element.style.opacity = '0';
                element.style.transform = 'translateY(20px)';
                element.style.transition = 'opacity 0.5s ease, transform 0.5s ease';
                
                setTimeout(() => {
                    element.style.opacity = '1';
                    element.style.transform = 'translateY(0)';
                }, 100);
            }, index * 100);
        });
    }
    
    // 延迟执行动画，让页面加载更流畅
    setTimeout(animateElements, 500);
}

// API请求函数
function fetchAPI(endpoint, data = null, method = 'GET') {
    return fetch(endpoint, {
        method: method,
        headers: {
            'Content-Type': 'application/json'
        },
        body: data ? JSON.stringify(data) : null
    })
    .then(response => {
        if (!response.ok) {
            throw new Error(`HTTP错误! 状态码: ${response.status}`);
        }
        return response.json();
    })
    .catch(error => {
        console.error('API请求错误:', error);
        return { error: error.message };
    });
}