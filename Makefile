# WebFileServer Makefile

# 编译器设置
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -g -O2
LDFLAGS = -lpthread

# 定义源文件和目标文件
SRCS = main.cpp http_connection.cpp utils.cpp event_loop.cpp channel.cpp epoller.cpp inet_address.cpp socket.cpp tcp_connection.cpp event_loop_thread_pool.cpp acceptor.cpp tcp_server.cpp timestamp.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = main

# 默认目标
all: $(TARGET)
	@echo "编译完成: $(TARGET)"

# 链接目标文件
$(TARGET): $(OBJS)
	@echo "链接目标文件..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# 编译源文件
%.o: %.cpp
	@echo "编译 $<..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# 清理生成的文件
clean:
	@echo "清理生成文件..."
	rm -f $(OBJS) $(TARGET)
	@echo "清理完成"

# 调试模式
debug:
	@echo "编译调试版本..."
	$(MAKE) CXXFLAGS="-std=c++11 -Wall -Wextra -O0 -g -DDEBUG" all

# 显示依赖关系
depend:
	@echo "生成依赖关系..."
	$(CXX) $(CXXFLAGS) -MM $(SRCS) > .depend

# 包含依赖关系
-include .depend

# 运行程序
run:
	./$(TARGET)

# 帮助信息
help:
	@echo "可用命令:"
	@echo "  make          - 编译项目"
	@echo "  make debug    - 编译调试版本"
	@echo "  make run      - 运行程序"
	@echo "  make clean    - 清理生成文件"
	@echo "  make depend   - 生成依赖关系"
	@echo "  make help     - 显示帮助信息"

# 伪目标
.PHONY: all clean debug depend run help