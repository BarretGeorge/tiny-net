#!/usr/bin/env python3
"""UDP Server - 接收并回显UDP数据"""

import socket
import argparse


def main():
    parser = argparse.ArgumentParser(description="UDP Server")
    parser.add_argument("-p", "--port", type=int, default=9999, help="监听端口 (默认: 9999)")
    parser.add_argument("-b", "--bind", default="0.0.0.0", help="绑定地址 (默认: 0.0.0.0)")
    args = parser.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((args.bind, args.port))
    print(f"UDP server listening on {args.bind}:{args.port} ...")

    try:
        while True:
            data, addr = sock.recvfrom(4096)
            text = data.decode("utf-8", errors="replace").rstrip("\x00")
            print(f"[{addr[0]}:{addr[1]}] ({len(data)} bytes): {text}")
            # echo back
            sock.sendto(data, addr)
    except KeyboardInterrupt:
        print("\nserver stopped.")
    finally:
        sock.close()


if __name__ == "__main__":
    main()
