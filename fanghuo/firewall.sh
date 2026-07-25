#!/bin/bash
# ============================================================
# 精简版企业防火墙脚本
# 功能：SNAT/DNAT + 防攻击 + 限速 + 日志
# 适用：Ubuntu Server 22.04+，三网卡（WAN/LAN/DMZ）
# ============================================================

# ----- 网络接口和网段定义 -----
WAN_IF="ens33"
LAN_IF="ens34"
DMZ_IF="ens35"
LAN_NET="192.168.1.0/24"
DMZ_NET="10.0.0.0/24"
DMZ_SERVER="10.0.0.10"
WAN_IP="192.168.124.94"

# ----- 清空所有规则 -----
iptables -F
iptables -X
iptables -t nat -F
iptables -t mangle -F

# ----- 默认策略 -----
iptables -P INPUT DROP
iptables -P FORWARD DROP
iptables -P OUTPUT ACCEPT

# ----- 通用放行（本地 + 已建立连接）-----
iptables -A INPUT -i lo -j ACCEPT
iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
iptables -A FORWARD -m state --state ESTABLISHED,RELATED -j ACCEPT

# ----- 1. 防火墙管理（SSH）-----
iptables -A INPUT -p tcp --dport 22 -s $LAN_NET -j ACCEPT
iptables -A INPUT -p tcp --dport 22 -s $DMZ_NET -j ACCEPT

# ----- 2. 防攻击（限速）-----
# SYN Flood
iptables -A INPUT -p tcp --syn -m limit --limit 10/s --limit-burst 20 -j ACCEPT
iptables -A INPUT -p tcp --syn -j DROP

# 端口扫描
iptables -A INPUT -p tcp -m state --state NEW -m limit --limit 30/min --limit-burst 50 -j ACCEPT
iptables -A INPUT -p tcp -m state --state NEW -j DROP

# SSH 暴力破解防护
iptables -A INPUT -p tcp --dport 22 -m state --state NEW -m recent --set --name SSH
iptables -A INPUT -p tcp --dport 22 -m state --state NEW -m recent --update --seconds 60 --hitcount 4 --name SSH -j DROP

# ----- 3. SNAT（内网/DMZ 访问外网）-----
iptables -t nat -A POSTROUTING -s $LAN_NET -o $WAN_IF -j MASQUERADE
iptables -t nat -A POSTROUTING -s $DMZ_NET -o $WAN_IF -j MASQUERADE

# ----- 4. DNAT（外网访问 DMZ 服务）-----
iptables -t nat -A PREROUTING -i $WAN_IF -p tcp --dport 80 -j DNAT --to-destination $DMZ_SERVER:80
iptables -t nat -A PREROUTING -i $WAN_IF -p tcp --dport 443 -j DNAT --to-destination $DMZ_SERVER:443
iptables -t nat -A PREROUTING -i $WAN_IF -p tcp --dport 21 -j DNAT --to-destination $DMZ_SERVER:21
iptables -t nat -A PREROUTING -i $WAN_IF -p tcp --dport 22 -j DNAT --to-destination $DMZ_SERVER:22

# ----- 5. FORWARD 规则（精确转发）-----
# 内网/DMZ 出站
iptables -A FORWARD -s $LAN_NET -o $WAN_IF -j ACCEPT
iptables -A FORWARD -s $DMZ_NET -o $WAN_IF -j ACCEPT

# 外网访问 DMZ 服务（入站）
iptables -A FORWARD -i $WAN_IF -o $DMZ_IF -p tcp --dport 80 -j ACCEPT
iptables -A FORWARD -i $WAN_IF -o $DMZ_IF -p tcp --dport 443 -j ACCEPT
iptables -A FORWARD -i $WAN_IF -o $DMZ_IF -p tcp --dport 21 -j ACCEPT
iptables -A FORWARD -i $WAN_IF -o $DMZ_IF -p tcp --dport 22 -j ACCEPT

# 内网访问 DMZ（管理）
iptables -A FORWARD -s $LAN_NET -d $DMZ_NET -j ACCEPT




# ----- 6. 日志链（采样）-----
iptables -N LOG_DROP
iptables -A LOG_DROP -m limit --limit 5/min -j LOG --log-prefix "FW-DROP: " --log-level 4
iptables -A LOG_DROP -j DROP

# 未匹配的入站流量丢弃 + 日志（放最后）
iptables -A INPUT -j LOG_DROP
# 未匹配的转发流量丢弃 + 日志
iptables -A FORWARD -j LOG_DROP

# ----- 显示规则 -----
echo "=== 精简版防火墙规则已加载 ==="
iptables -L -n -v --line-numbers

# ----- 持久化保存提示 -----
read -p "是否保存规则使其永久生效？(y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    if command -v netfilter-persistent >/dev/null 2>&1; then
        sudo netfilter-persistent save
        echo "规则已保存"
    else
        sudo apt install -y iptables-persistent
        sudo netfilter-persistent save
    fi
else
    echo "规则未保存，重启后将丢失。"
fi
