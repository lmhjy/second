#!/usr/bin/env python3
import socket
import sys
import time

def scan_ports(target_ip, ports):
    print(f"[*] 开始扫描目标主机: {target_ip}")
    print("[*] 正在探测端口状态...")
    
    for port in ports:
        # 创建一个 socket 对象
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        # 设置超时时间为 1 秒，防止卡死
        s.settimeout(1)
        
        # 尝试连接目标端口
        result = s.connect_ex((target_ip, port))
        
        if result == 0:
            print(f"[+] 发现开放端口: {port}")
        
        s.close()

if __name__ == "__main__":
    # DMZ 服务器的 IP 地址
    target = "192.168.124.82"
    # 我们重点关注 Web(80)、FTP(21)、SSH(22) 和可能漏网的内网穿透端口
    common_ports = [21, 22, 80, 443, 8080, 3306]
    
    start_time = time.time()
    scan_ports(target, common_ports)
    end_time = time.time()
    
    print(f"[*] 扫描完成，耗时: {round(end_time - start_time, 2)} 秒")
