#!/usr/bin/env python3
"""检查本机 IP 地址，以及 tiny-net 虚拟 IP (.95) 是否可用"""

import socket
import subprocess
import sys


def get_local_ip():
    """获取本机在局域网中的 IP 地址"""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.connect(("8.8.8.8", 80))
            return s.getsockname()[0]
    except OSError:
        return None


def get_all_interfaces():
    """通过 ip addr 获取所有接口的 IP"""
    result = subprocess.run(
        ["ip", "-4", "-o", "addr", "show"],
        capture_output=True, text=True,
    )
    interfaces = []
    for line in result.stdout.splitlines():
        parts = line.split()
        # 格式: idx name inet ip/prefix ...
        name = parts[1]
        addr = parts[3].split("/")[0]
        interfaces.append((name, addr))
    return interfaces


def make_virtual_ip(ip):
    """根据 tiny-net 规则生成虚拟 IP：末位设为 95，若已为 95 则设为 96"""
    parts = ip.split(".")
    if len(parts) != 4:
        return None
    d = int(parts[3])
    virtual_d = 96 if d == 95 else 95
    return f"{parts[0]}.{parts[1]}.{parts[2]}.{virtual_d}"


def check_ip_available(ip, timeout=1):
    """通过 arping 检测 IP 是否已被占用（需要 root）"""
    try:
        result = subprocess.run(
            ["arping", "-c", "2", "-w", str(timeout), "-D", ip],
            capture_output=True, text=True, timeout=timeout + 3,
        )
        # arping -D: 返回 0 表示无冲突（IP 可用），返回 1 表示有冲突
        return result.returncode == 0
    except (FileNotFoundError, subprocess.TimeoutExpired):
        pass

    # fallback: 用 ping 探测
    try:
        result = subprocess.run(
            ["ping", "-c", "2", "-W", str(timeout), ip],
            capture_output=True, text=True, timeout=timeout + 3,
        )
        # ping 返回 0 表示有回复 → IP 已被占用
        return result.returncode != 0
    except subprocess.TimeoutExpired:
        return True  # 超时视为可用


def main():
    print("=== Tiny-Net IP 检查工具 ===\n")

    # 1. 显示所有接口
    print("[本机网络接口]")
    interfaces = get_all_interfaces()
    if interfaces:
        for name, addr in interfaces:
            print(f"  {name:16s} {addr}")
    else:
        print("  (未检测到网络接口)")
    print()

    # 2. 显示默认出口 IP
    local_ip = get_local_ip()
    if not local_ip:
        print("无法获取本机 IP 地址")
        sys.exit(1)
    print(f"[默认出口 IP]  {local_ip}\n")

    # 3. 计算虚拟 IP
    virtual_ip = make_virtual_ip(local_ip)
    if not virtual_ip:
        print("IP 地址格式异常")
        sys.exit(1)
    print(f"[虚拟 IP (tiny-net)]  {virtual_ip}")

    # 4. 检查虚拟 IP 是否可用
    print(f"\n正在检测 {virtual_ip} 是否被占用 ...")
    available = check_ip_available(virtual_ip)

    if available:
        print(f"\n{virtual_ip} 可用 — tiny-net 可以正常使用该地址")
    else:
        print(f"\n{virtual_ip} 已被占用！")
        print("tiny-net 启动后可能出现 IP 冲突，建议：")
        print(f"  1) 释放占用 {virtual_ip} 的设备")
        print(f"  2) 或修改 common.c 中的虚拟 IP 规则")

    print("\n=== 检查完成 ===")


if __name__ == "__main__":
    main()
