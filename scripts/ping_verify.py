#!/usr/bin/env python3
"""Tiny-Net Ping 快速验证工具"""

import os
import sys
import subprocess
import time
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent
BUILD_DIR = PROJECT_ROOT / "build"
PING_PROG = BUILD_DIR / "ping"
TEST_PROG = PROJECT_ROOT / "test_dynamic_ip"
TCPDUMP_LOG = Path("/tmp/tcpdump.log")


def check_root():
    if os.geteuid() != 0:
        print("需要 root 权限")
        print(f"请使用: sudo {SCRIPT_DIR}/ping_verify.py")
        sys.exit(1)


def build_ping():
    if PING_PROG.is_file():
        return
    print("ping 程序不存在，开始编译...")
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    try:
        subprocess.run(
            ["cmake", "-B", "build", "-S", "."],
            cwd=PROJECT_ROOT, capture_output=True, check=True,
        )
        subprocess.run(
            ["cmake", "--build", "build", "--target", "ping", "-j4"],
            cwd=PROJECT_ROOT, capture_output=True, check=True,
        )
    except subprocess.CalledProcessError:
        pass
    if not PING_PROG.is_file():
        print("编译失败")
        sys.exit(1)
    print("编译成功")


def show_network_config():
    print("当前网络配置：")
    print()
    if TEST_PROG.is_file():
        try:
            subprocess.run([str(TEST_PROG)], stderr=subprocess.DEVNULL)
        except Exception:
            print("  (无法获取网络配置信息)")
    else:
        print("  (test_dynamic_ip 程序不存在，跳过网络配置显示)")
    print()


def detect_gateway():
    try:
        result = subprocess.run(
            ["ip", "route"], capture_output=True, text=True, check=True,
        )
        for line in result.stdout.splitlines():
            if line.startswith("default"):
                parts = line.split()
                idx = parts.index("via") if "via" in parts else -1
                if idx >= 0 and idx + 1 < len(parts):
                    return parts[idx + 1]
    except Exception:
        pass
    return None


def choose_target():
    print("请选择测试目标：")
    print("  1) 本地回环 (127.0.0.1)")
    print("  2) 网关 (自动检测)")
    print("  3) DNS服务器 (8.8.8.8)")
    print("  4) 自定义 IP")
    print()

    choice = input("选择 [1-4]: ").strip()

    if choice == "1":
        return "127.0.0.1"
    elif choice == "2":
        gw = detect_gateway()
        if not gw:
            print("无法检测到网关")
            sys.exit(1)
        return gw
    elif choice == "3":
        return "8.8.8.8"
    elif choice == "4":
        return input("输入目标 IP: ").strip()
    else:
        print("无效选择")
        sys.exit(1)


def run_test(target):
    print(f"\n目标: {target}\n")
    print("开始验证，将同时显示：")
    print("  - Ping 程序输出")
    print("  - tcpdump 抓包信息")
    print("\n按 Ctrl+C 停止\n")
    time.sleep(2)

    # 启动 tcpdump
    with open(TCPDUMP_LOG, "w") as f:
        f.write("=== tcpdump 输出 ===\n")

    tcpdump_proc = subprocess.Popen(
        ["tcpdump", "-i", "any", "-n", "icmp", "and", "host", target],
        stdout=open(TCPDUMP_LOG, "a"),
        stderr=subprocess.DEVNULL,
    )
    time.sleep(1)

    # 运行 ping
    print("=== Ping 输出 ===\n")
    try:
        subprocess.run(
            ["timeout", "10", str(PING_PROG), target],
            timeout=15,
        )
    except (subprocess.TimeoutExpired, Exception):
        pass

    # 停止 tcpdump
    tcpdump_proc.terminate()
    try:
        tcpdump_proc.wait(timeout=3)
    except subprocess.TimeoutExpired:
        tcpdump_proc.kill()


def analyze_results():
    print("\n=== 抓包分析 ===\n")
    if not TCPDUMP_LOG.is_file():
        return

    content = TCPDUMP_LOG.read_text()
    lines = content.splitlines()

    packet_count = sum(1 for l in lines if "ICMP" in l)
    request_count = sum(1 for l in lines if "echo request" in l)
    reply_count = sum(1 for l in lines if "echo reply" in l)

    print("抓取到的数据包：")
    print(f"   总 ICMP 包: {packet_count}")
    print(f"   Echo Request: {request_count}")
    print(f"   Echo Reply: {reply_count}")
    print()

    if request_count > 0:
        print("检测到发送的 ICMP Echo Request")
    else:
        print("未检测到发送的 ICMP Echo Request")

    if reply_count > 0:
        print("检测到接收的 ICMP Echo Reply")
    else:
        print("未检测到接收的 ICMP Echo Reply (可能目标不可达)")

    print("\n详细抓包信息：")
    for line in lines[:20]:
        print(line)

    TCPDUMP_LOG.unlink(missing_ok=True)


def main():
    print("=== Tiny-Net Ping 快速验证工具 ===\n")

    check_root()
    build_ping()
    show_network_config()

    target = choose_target()
    run_test(target)
    analyze_results()

    print("\n=== 验证完成 ===")


if __name__ == "__main__":
    main()
