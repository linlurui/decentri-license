#!/bin/bash

echo "=========================================="
echo "DecentriLicense C++ SDK 验证向导启动器"
echo "=========================================="
echo

# 检查CMake
if ! command -v cmake &> /dev/null
then
    echo "错误: 未找到CMake，请先安装CMake"
    exit 1
fi

# 检查编译器
if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null
then
    echo "错误: 未找到C++编译器，请先安装GCC或Clang"
    exit 1
fi

# 创建构建目录
echo "创建构建目录..."
mkdir -p build
cd build

# 配置项目
echo "配置项目..."
cmake ..

# 编译项目
echo "编译验证向导..."
make

# 检查编译是否成功
if [ ! -f "bin/validation_wizard" ]; then
    echo "编译失败，请检查错误信息"
    exit 1
fi

echo
echo "正在启动 DecentriLicense C++ SDK 验证向导..."
echo "请在程序中选择相应选项进行操作:"
echo "  1. 激活令牌 - 使用本地令牌文件激活许可证"
echo "  2. 校验令牌 - 验证令牌文件的有效性"
echo "  3. 记账信息 - 查看和管理许可证使用记录"
echo "  4. 信任链验证 - 验证令牌的信任链完整性"
echo "  5. 综合验证 - 执行完整的许可证验证流程"
echo "  6. 退出 - 退出程序"
echo

# 运行验证向导
./bin/validation_wizard