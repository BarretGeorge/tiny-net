#!/bin/bash

echo "=== Tiny-Net Ping 快速验证工具 ==="
echo ""

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# 项目根目录
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
# build 目录
BUILD_DIR="$PROJECT_ROOT/build"
# ping 程序路径
PING_PROG="$BUILD_DIR/ping"
# test_dynamic_ip 程序路径
TEST_PROG="$PROJECT_ROOT/test_dynamic_ip"

# 检查是否有 root 权限
if [ "$EUID" -ne 0 ]; then
    echo "❌ 需要 root 权限"
    echo "请使用: sudo $SCRIPT_DIR/quick_verify.sh"
    echo "或: cd $SCRIPT_DIR && sudo ./quick_verify.sh"
    exit 1
fi

# 检查 ping 程序是否存在
if [ ! -f "$PING_PROG" ]; then
    echo "⚠️  ping 程序不存在，开始编译..."
    mkdir -p "$BUILD_DIR"
    cd "$PROJECT_ROOT"
    cmake -B build -S . > /dev/null 2>&1
    cmake --build build --target ping -j4 > /dev/null 2>&1
    if [ ! -f "$PING_PROG" ]; then
        echo "❌ 编译失败"
        exit 1
    fi
    echo "✓ 编译成功"
fi

echo "📊 当前网络配置："
echo ""
if [ -f "$TEST_PROG" ]; then
    "$TEST_PROG" 2>/dev/null || echo "  (无法获取网络配置信息)"
else
    echo "  (test_dynamic_ip 程序不存在，跳过网络配置显示)"
fi
echo ""

# 选择测试目标
echo "请选择测试目标："
echo "  1) 本地回环 (127.0.0.1)"
echo "  2) 网关 (自动检测)"
echo "  3) DNS服务器 (8.8.8.8)"
echo "  4) 自定义 IP"
echo ""
read -p "选择 [1-4]: " choice

case $choice in
    1)
        TARGET="127.0.0.1"
        ;;
    2)
        TARGET=$(ip route | grep default | awk '{print $3}' | head -1)
        if [ -z "$TARGET" ]; then
            echo "❌ 无法检测到网关"
            exit 1
        fi
        ;;
    3)
        TARGET="8.8.8.8"
        ;;
    4)
        read -p "输入目标 IP: " TARGET
        ;;
    *)
        echo "❌ 无效选择"
        exit 1
        ;;
esac

echo ""
echo "🎯 目标: $TARGET"
echo ""
echo "开始验证，将同时显示："
echo "  - Ping 程序输出"
echo "  - tcpdump 抓包信息"
echo ""
echo "按 Ctrl+C 停止"
echo ""
sleep 2

# 获取第一个非回环接口
INTERFACE=$(ip -o link show | awk -F': ' '{print $2}' | grep -v lo | head -1)

# 在后台启动 tcpdump
echo "=== tcpdump 输出 ===" > /tmp/tcpdump.log
tcpdump -i any -n icmp and host $TARGET 2>/dev/null >> /tmp/tcpdump.log &
TCPDUMP_PID=$!

# 等待 tcpdump 启动
sleep 1

# 运行 ping
echo "=== Ping 输出 ==="
echo ""
timeout 10 "$PING_PROG" $TARGET 2>&1 || true

# 停止 tcpdump
kill $TCPDUMP_PID 2>/dev/null
wait $TCPDUMP_PID 2>/dev/null

echo ""
echo "=== 抓包分析 ==="
echo ""
if [ -f /tmp/tcpdump.log ]; then
    PACKET_COUNT=$(grep -c "ICMP" /tmp/tcpdump.log)
    REQUEST_COUNT=$(grep -c "echo request" /tmp/tcpdump.log)
    REPLY_COUNT=$(grep -c "echo reply" /tmp/tcpdump.log)

    echo "📦 抓取到的数据包："
    echo "   总 ICMP 包: $PACKET_COUNT"
    echo "   Echo Request: $REQUEST_COUNT"
    echo "   Echo Reply: $REPLY_COUNT"
    echo ""

    if [ $REQUEST_COUNT -gt 0 ]; then
        echo "✓ 检测到发送的 ICMP Echo Request"
    else
        echo "❌ 未检测到发送的 ICMP Echo Request"
    fi

    if [ $REPLY_COUNT -gt 0 ]; then
        echo "✓ 检测到接收的 ICMP Echo Reply"
    else
        echo "⚠️  未检测到接收的 ICMP Echo Reply (可能目标不可达)"
    fi

    echo ""
    echo "详细抓包信息："
    cat /tmp/tcpdump.log | head -20

    rm /tmp/tcpdump.log
fi

echo ""
echo "=== 验证完成 ==="
