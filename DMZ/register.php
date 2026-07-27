<?php
session_start();
if (isset($_SESSION['username'])) {
    header("Location: chat.php");
    exit;
}

$error = '';

if ($_SERVER['REQUEST_METHOD'] == 'POST') {
    require 'db.php';
    $username = trim($_POST['username'] ?? '');
    $password = $_POST['password'] ?? '';
    $confirm = $_POST['confirm'] ?? '';

    // 简单验证
    if (empty($username) || empty($password) || empty($confirm)) {
        $error = "所有字段都必须填写。";
    } elseif ($password !== $confirm) {
        $error = "两次输入的密码不一致。";
    } elseif (strlen($password) < 6) {
        $error = "密码长度至少为6位。";
    } else {
        // 检查用户名是否已存在
        $stmt = $pdo->prepare("SELECT id FROM users WHERE username = ?");
        $stmt->execute([$username]);
        if ($stmt->fetch()) {
            $error = "用户名已被注册。";
        } else {
            // 哈希密码（使用 password_hash 推荐）
            $hashed = password_hash($password, PASSWORD_DEFAULT);
            $stmt = $pdo->prepare("INSERT INTO users (username, password) VALUES (?, ?)");
            if ($stmt->execute([$username, $hashed])) {
                // 自动登录
                $_SESSION['username'] = $username;
                header("Location: chat.php");
                exit;
            } else {
                $error = "注册失败，请稍后重试。";
            }
        }
    }
}
?>
<!DOCTYPE html>
<html>
<head><meta charset="UTF-8"><title>注册</title></head>
<body>
    <h2>注册新账户</h2>
    <?php if ($error): ?><p style="color:red"><?php echo htmlspecialchars($error); ?></p><?php endif; ?>
    <form method="POST">
        <input type="text" name="username" placeholder="用户名" required><br>
        <input type="password" name="password" placeholder="密码" required><br>
        <input type="password" name="confirm" placeholder="确认密码" required><br>
        <input type="submit" value="注册">
    </form>
    <p>已有账户？<a href="login.php">登录</a></p>
</body>
</html>
