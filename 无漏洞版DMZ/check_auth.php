<?php
session_start();
if (!isset($_SESSION['username'])) {
    header("Location: login.php");
    exit;
}

require_once 'db.php';

$stmt = $pdo->prepare("SELECT session_token FROM users WHERE username = ?");
$stmt->execute([$_SESSION['username']]);
$row = $stmt->fetch();

if (!$row || $row['session_token'] !== $_SESSION['session_token']) {
    // 令牌无效，说明已被其他登录踢出
    session_destroy();
    header("Location: login.php?msg=duplicate");
    exit;
}
?>
