#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import random
import string
import re

# ---------- 策略配置 ----------
MIN_LEN = 8
MAX_LEN = 16

WEAK_PASSWORDS = {
    'password', '123456', '12345678', 'qwerty', 'abc123',
    'admin', 'root', 'toor', 'letmein', 'welcome',
    'monkey', 'dragon', 'master', 'sunshine', 'princess'
}

KEYBOARD_SEQUENCES = [
    'qwertyuiop', 'asdfghjkl', 'zxcvbnm',
    'poiuytrewq', 'lkjhgfdsa', 'mnbvcxz',
    '1234567890', '0987654321'
]
MIN_SEQ_LEN = 4

# 随机测试密码数量
PASSWORD_COUNT = 10000

# ---------- 辅助函数 ----------
def has_keyboard_sequence(password):
    lower_pwd = password.lower()
    for seq in KEYBOARD_SEQUENCES:
        for i in range(len(seq) - MIN_SEQ_LEN + 1):
            if seq[i:i+MIN_SEQ_LEN] in lower_pwd:
                return True
    return False

def check_complexity(password):
    categories = 0
    if re.search(r'[a-z]', password):
        categories += 1
    if re.search(r'[A-Z]', password):
        categories += 1
    if re.search(r'[0-9]', password):
        categories += 1
    if re.search(r'[!@#$%^&*()_+\-=\[\]{};:\'",.<>?/\\|`~]', password):
        categories += 1
    return categories >= 3, categories

def generate_random_password():
    length = random.randint(MIN_LEN, MAX_LEN)
    chars = string.ascii_letters + string.digits + string.punctuation
    return ''.join(random.choice(chars) for _ in range(length))

def validate_password(password, history_set=None):
    """返回 (通过与否, 失败原因列表)"""
    if history_set is None:
        history_set = set()
    reasons = []

    # 1. 长度检查
    if not (MIN_LEN <= len(password) <= MAX_LEN):
        reasons.append(f"长度 {len(password)} 不在 {MIN_LEN}-{MAX_LEN} 之间")

    # 2. 复杂度检查（至少3类字符）
    ok, cat = check_complexity(password)
    if not ok:
        reasons.append(f"字符类型不足 ({cat} 类，需要至少3类)")

    # 3. 弱口令黑名单
    if password.lower() in WEAK_PASSWORDS:
        reasons.append("命中弱口令黑名单")

    # 4. 键盘连续序列
    if has_keyboard_sequence(password):
        reasons.append("包含键盘连续序列")

    # 5. 历史重复（仅在提供历史集合时检查）
    if history_set and password in history_set:
        reasons.append("与历史密码重复（模拟最近3次不同）")

    return len(reasons) == 0, reasons

# ---------- 手动验证模式 ----------
def manual_check():
    print("\n" + "="*50)
    print("【手动密码验证】")
    print(f"策略要求：长度 {MIN_LEN}-{MAX_LEN}，至少3类字符，不在弱口令列表，不含键盘连续序列")
    pwd = input("请输入要验证的密码: ").strip()
    if not pwd:
        print("密码不能为空")
        return
    ok, reasons = validate_password(pwd)
    if ok:
        print("✅ 密码符合所有策略要求！")
    else:
        print("❌ 密码不符合以下要求：")
        for r in reasons:
            print(f"  - {r}")

# ---------- 批量随机测试 ----------
def random_test():
    print("\n" + "="*50)
    print(f"【随机生成 {PASSWORD_COUNT} 个密码进行批量测试】")

    history = set()
    passed = 0
    failed_passwords = []
    fail_stats = {"长度": 0, "复杂度": 0, "弱口令": 0, "键盘序列": 0, "历史重复": 0}

    for _ in range(PASSWORD_COUNT):
        pwd = generate_random_password()
        ok, reasons = validate_password(pwd, history)
        history.add(pwd)   # 模拟历史记录
        if ok:
            passed += 1
        else:
            failed_passwords.append((pwd, reasons))
            for r in reasons:
                if "长度" in r:
                    fail_stats["长度"] += 1
                elif "字符类型" in r:
                    fail_stats["复杂度"] += 1
                elif "弱口令" in r:
                    fail_stats["弱口令"] += 1
                elif "键盘" in r:
                    fail_stats["键盘序列"] += 1
                elif "历史" in r:
                    fail_stats["历史重复"] += 1

    print(f"总密码数: {PASSWORD_COUNT}")
    print(f"通过数: {passed}")
    print(f"失败数: {PASSWORD_COUNT - passed}")
    print(f"通过率: {passed / PASSWORD_COUNT * 100:.2f}%")
    print("\n失败原因统计（可累计）:")
    for key, value in fail_stats.items():
        if value:
            print(f"  - {key}: {value} 次")
    if failed_passwords:
        print("\n失败密码样例（显示10个）:")
        for pwd, reasons in failed_passwords[:10]:
            print(f"  {pwd}  -> 原因: {', '.join(reasons)}")

# ---------- 主菜单 ----------
def main():
    while True:
        print("\n" + "="*50)
        print("       密码策略验证工具")
        print("="*50)
        print("1. 随机生成 10000 个密码进行批量测试")
        print("2. 手动输入密码进行验证")
        print("3. 退出程序")
        choice = input("请选择 (1/2/3): ").strip()
        if choice == '1':
            random_test()
        elif choice == '2':
            manual_check()
        elif choice == '3':
            print("退出程序。")
            break
        else:
            print("无效输入，请重新选择。")

if __name__ == "__main__":
    main()
