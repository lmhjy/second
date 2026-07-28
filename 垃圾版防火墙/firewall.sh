#!/bin/bash
# ============================================================
# 弱化版防火墙脚本（含多个安全漏洞）
# 用途：演示不安全配置，仅限实验环境
# ============================================================

# ----- 网络接口和网段定义 -----
WAN_IF="ens33"
LAN_IF="ens38"
DMZ_IF="ens37"
LAN_NET="192.168.1.0/24"
DMZ_NET="10.0.0.0/24"
DMZ_SERVER="10.0.0.10"
WAN_IP="192.168.124.56"
sysctl -w net.ipv4.ip_forward=1

# ----- 清空所有规则 -----
iptables -F
iptables -X
iptables -t nat -F
iptables -t mangle -F

# ----- 默认策略（极为宽松）-----
iptables -P INPUT ACCEPT      # 漏洞1：允许所有入站流量
iptables -P FORWARD ACCEPT    # 漏洞2：允许所有转发流量
iptables -P OUTPUT ACCEPT     # 漏洞3：允许所有出站流量

# ----- 未添加任何防攻击规则（漏洞4）-----
# 无 SYN Flood 防护、无端口扫描限制、无 SSH 暴力破解防护

# ----- SNAT（保留，使内网能上网）-----
iptables -t nat -A POSTROUTING -s $LAN_NET -o $WAN_IF -j MASQUERADE
iptables -t nat -A POSTROUTING -s $DMZ_NET -o $WAN_IF -j MASQUERADE

# ----- DNAT（保留，使外网能访问 DMZ）-----
iptables -t nat -A PREROUTING -i $WAN_IF -p tcp --dport 80 -j DNAT --to-destination $DMZ_SERVER:80
iptables -t nat -A PREROUTING -i $WAN_IF -p tcp --dport 443 -j DNAT --to-destination $DMZ_SERVER:443
iptables -t nat -A PREROUTING -i $WAN_IF -p tcp --dport 21 -j DNAT --to-destination $DMZ_SERVER:21
iptables -t nat -A PREROUTING -i $WAN_IF -p tcp --dport 22 -j DNAT --to-destination $DMZ_SERVER:22

# ----- 未添加任何日志记录（漏洞5）-----
# 没有 LOG_DROP 链，无法审计被拒绝的流量（实际上没有拒绝）

# ----- 显示规则 -----
echo "=== 弱化版防火墙规则已加载（含多个漏洞） ==="
iptables -L -n -v --line-numbers

# ----- 不保存规则（漏洞6：规则不持久化，重启后失效）-----
echo "规则未保存，重启后将丢失。"
