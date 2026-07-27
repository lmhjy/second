#!/bin/bash
# ============================================================
# 修正版企业防火墙脚本
# 功能：SNAT/DNAT + 防攻击 + 限速 + 日志
# 适用：Ubuntu Server 22.04+，三网卡（WAN/LAN/DMZ）
# ============================================================
sysctl -w net.ipv4.ip_forward=1
# ----- 网络接口和网段定义 -----
WAN_IF="ens33"
LAN_IF="ens38"          # 若没有LAN口，可忽略，但保留变量
DMZ_IF="ens37"
LAN_NET="192.168.1.0/24"
DMZ_NET="10.0.0.0/24"
DMZ_SERVER="10.0.0.10"
WAN_IP="192.168.124.82"    # 根据实际修改

# ----- 清空所有规则 -----
iptables -F
iptables -X
iptables -t nat -F
iptables -t mangle -F

# ----- 默认策略 -----
iptables -P INPUT DROP
iptables -P FORWARD DROP
iptables -P OUTPUT ACCEPT

# ----- 创建自定义日志链（必须先创建！）-----
iptables -N LOG_DROP
iptables -A LOG_DROP -m limit --limit 5/min -j LOG --log-prefix "FW-DROP: " --log-level 4
iptables -A LOG_DROP -j DROP

# ============================================================
# 通用放行（本地 + 已建立连接）
# ============================================================
iptables -A INPUT -i lo -j ACCEPT
iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
iptables -A FORWARD -m state --state ESTABLISHED,RELATED -j ACCEPT

# 允许 ICMP（ping）
iptables -A INPUT -p icmp --icmp-type echo-request -j ACCEPT

# ============================================================
# 1. 防火墙自身管理（SSH 只允许内网和DMZ网段）
# ============================================================
# 记录并放行内网/DMZ的SSH新连接（用于后续防暴力破解统计）
iptables -A INPUT -p tcp --dport 22 -s $LAN_NET -m state --state NEW -m recent --set --name SSH -j ACCEPT
iptables -A INPUT -p tcp --dport 22 -s $DMZ_NET -m state --state NEW -m recent --set --name SSH -j ACCEPT
# 如果之前已经存在该IP的SSH连接记录，且60秒内超过4次，则丢弃
iptables -A INPUT -p tcp --dport 22 -s $LAN_NET -m state --state NEW -m recent --update --seconds 60 --hitcount 4 --name SSH -j DROP
iptables -A INPUT -p tcp --dport 22 -s $DMZ_NET -m state --state NEW -m recent --update --seconds 60 --hitcount 4 --name SSH -j DROP

# ============================================================
# 2. 防攻击（限速）
# ============================================================
# 2.1 防 SYN Flood（限制每秒 SYN 包数量）
iptables -A INPUT -p tcp --syn -m limit --limit 10/s --limit-burst 20 -j ACCEPT
iptables -A INPUT -p tcp --syn -j DROP

# 2.2 防端口扫描（先放行限速内的新连接，再丢弃超限的）
iptables -A INPUT -p tcp -m state --state NEW -m limit --limit 30/min --limit-burst 50 -j ACCEPT
iptables -A INPUT -p tcp -m state --state NEW -j DROP

# ============================================================
# 3. SNAT：内网和 DMZ 访问外网
# ============================================================
iptables -t nat -A POSTROUTING -s $LAN_NET -o $WAN_IF -j MASQUERADE
iptables -t nat -A POSTROUTING -s $DMZ_NET -o $WAN_IF -j MASQUERADE

# ============================================================
# 4. DNAT：对外发布 DMZ 服务
# ============================================================
iptables -t nat -A PREROUTING -i $WAN_IF -p tcp --dport 80 -j DNAT --to-destination $DMZ_SERVER:80
iptables -t nat -A PREROUTING -i $WAN_IF -p tcp --dport 443 -j DNAT --to-destination $DMZ_SERVER:443
iptables -t nat -A PREROUTING -i $WAN_IF -p tcp --dport 21 -j DNAT --to-destination $DMZ_SERVER:21
iptables -t nat -A PREROUTING -i $WAN_IF -p tcp --dport 22 -j DNAT --to-destination $DMZ_SERVER:22

# ============================================================
# 5. FORWARD 规则（精确转发，拒绝一切未明确允许的流量）
# ============================================================
# 5.1 内网和 DMZ 主动访问外网（出站）——允许所有协议
iptables -A FORWARD -s $LAN_NET -o $WAN_IF -j ACCEPT
iptables -A FORWARD -s $DMZ_NET -o $WAN_IF -j ACCEPT

# 5.2 外网访问 DMZ 服务（入站）——只放行特定端口
iptables -A FORWARD -i $WAN_IF -o $DMZ_IF -p tcp --dport 80 -j ACCEPT
iptables -A FORWARD -i $WAN_IF -o $DMZ_IF -p tcp --dport 443 -j ACCEPT
iptables -A FORWARD -i $WAN_IF -o $DMZ_IF -p tcp --dport 21 -j ACCEPT
iptables -A FORWARD -i $WAN_IF -o $DMZ_IF -p tcp --dport 22 -j ACCEPT

# 5.3 内网访问 DMZ（管理）
iptables -A FORWARD -s $LAN_NET -d $DMZ_NET -j ACCEPT

# 5.4 未匹配的转发流量丢弃并记录日志
iptables -A FORWARD -j LOG_DROP

# ============================================================
# 6. 未匹配的入站流量丢弃并记录日志
# ============================================================
iptables -A INPUT -j LOG_DROP

# ----- 显示规则（便于检查）-----
echo "=== 修正版防火墙规则已加载 ==="
iptables -L -n -v --line-numbers

# ----- 持久化保存提示 -----
read -p "是否保存规则使其永久生效？(y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    if command -v netfilter-persistent >/dev/null 2>&1; then
        sudo netfilter-persistent save
        echo "规则已保存（netfilter-persistent）"
    else
        sudo apt install -y iptables-persistent
        sudo netfilter-persistent save
    fi
else
    echo "规则未保存，重启后将丢失。"
fi
