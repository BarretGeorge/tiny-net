#!/usr/bin/env python3
"""UDP Client - 发送UDP数据并接收回复"""

import socket
import argparse


def main():
    parser = argparse.ArgumentParser(description="UDP Client")
    parser.add_argument("-s", "--server", default="192.168.100.120", help="服务器地址 (默认: 192.168.100.120)")
    parser.add_argument("-p", "--port", type=int, default=9999, help="服务器端口 (默认: 9999)")
    args = parser.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(3)
    print(f"UDP client -> {args.server}:{args.port}")
    print("输入消息发送到服务器 (输入 'exit' 退出):\n")

    try:
        while True:
            msg = input("> ")
            if msg.strip().lower() == "exit":
                break
            sock.sendto(msg.encode("utf-8"), (args.server, args.port))
            print(f"  sent {len(msg)} bytes")
            try:
                data, addr = sock.recvfrom(4096)
                text = data.decode("utf-8", errors="replace").rstrip("\x00")
                print(f"  recv from {addr[0]}:{addr[1]}: {text}")
            except socket.timeout:
                print("  (no reply, timeout)")
    except (KeyboardInterrupt, EOFError):
        print("\nclient stopped.")
    finally:
        sock.close()


if __name__ == "__main__":
    main()
