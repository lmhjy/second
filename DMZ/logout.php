<?php
session_start();
if (isset($_SESSION['username'])) {
    require_once 'db.php';
    // 清空数据库中的 session_token，使该用户的令牌失效
    $update = $pdo->prepare("UPDATE users SET session_token = NULL WHERE username = ?");
    $update->execute([$_SESSION['username']]);
}

// 销毁当前会话
session_destroy();

// 重定向
header("Location: login.php");
exit;
