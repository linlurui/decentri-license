#!/bin/bash
# DecentriLicense Validation Wizard 启动脚本
# 自动从 build_config.sh 读取配置

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 读取构建配置（由一键打包程序更新）
if [ -f "${SCRIPT_DIR}/../build_config.sh" ]; then
    source "${SCRIPT_DIR}/../build_config.sh"
else
    # 默认配置
    export DYLD_LIBRARY_PATH="${SCRIPT_DIR}/../../../dl-core/build:${DYLD_LIBRARY_PATH}"
fi

# 运行验证向导
if [ -f "${SCRIPT_DIR}/validation_wizard" ]; then
    # 如果有编译好的可执行文件，直接运行
    "${SCRIPT_DIR}/validation_wizard" "$@"
else
    # 否则使用 go run
    cd "${SCRIPT_DIR}"
    go run validation_wizard.go "$@"
fi
