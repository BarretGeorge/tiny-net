#!/usr/bin/env python3

# 获取本机内网IP地址
import socket


def get_local_ip():
    """获取本机局域网IP"""
    try:
        # 创建一个UDP socket
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        # 连接一个外部地址（不需要真实可达）
        s.connect(("8.8.8.8", 80))
        local_ip = s.getsockname()[0]
        s.close()
        return local_ip
    except Exception:
        return "127.0.0.1"


local_ip = get_local_ip()

# 将最后一个字节设置为95
ip_parts = local_ip.split('.')
ip_parts[-1] = '95'

target_ip = '.'.join(ip_parts)
target_port = 9999
print(f"待发送地址: {target_ip}:{target_port}")

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# 获取键盘输入的消息
while True:
    message = input("请输入要发送的UDP消息（输入 'exit' 退出）：")
    if message.lower() == 'exit':
        print("退出程序。")
        break
    sock.sendto(message.encode(), (target_ip, target_port))
    print(f"发送消息完毕")
