#!/bin/bash

# 编译项目
echo "Building WebFileServer..."
make

# 检查编译是否成功
if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "You can run the server with: ./main [port] [root_path] [thread_count]"
    echo "Default: ./main 8888 ./ 4"
else
    echo "Build failed!"
    exit 1
fi