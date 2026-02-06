#!/usr/bin/env python3
"""TCP Server - 接收TCP连接并回显数据"""

import socket
import argparse
import threading


def handle_client(conn, addr):
    print(f"[+] connected: {addr[0]}:{addr[1]}")
    try:
        while True:
            data = conn.recv(4096)
            if not data:
                break
            text = data.decode("utf-8", errors="replace").rstrip("\x00")
            print(f"[{addr[0]}:{addr[1]}] ({len(data)} bytes): {text}")
            conn.sendall(data)
    except (ConnectionResetError, BrokenPipeError):
        pass
    finally:
        print(f"[-] disconnected: {addr[0]}:{addr[1]}")
        conn.close()


def main():
    parser = argparse.ArgumentParser(description="TCP Server")
    parser.add_argument("-p", "--port", type=int, default=9999, help="监听端口 (默认: 9999)")
    parser.add_argument("-b", "--bind", default="0.0.0.0", help="绑定地址 (默认: 0.0.0.0)")
    args = parser.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((args.bind, args.port))
    sock.listen(5)
    print(f"TCP server listening on {args.bind}:{args.port} ...")

    try:
        while True:
            conn, addr = sock.accept()
            t = threading.Thread(target=handle_client, args=(conn, addr), daemon=True)
            t.start()
    except KeyboardInterrupt:
        print("\nserver stopped.")
    finally:
        sock.close()


if __name__ == "__main__":
    main()
