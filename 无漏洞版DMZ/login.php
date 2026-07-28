<?php
session_start();
if (isset($_SESSION['username'])) {
    header("Location: chat.php");
    exit;
}

if ($_SERVER['REQUEST_METHOD'] == 'POST') {
    require 'db.php';
    $username = $_POST['username'] ?? '';
    $password = $_POST['password'] ?? '';

    // 先根据用户名查询
    $stmt = $pdo->prepare("SELECT password FROM users WHERE username = ?");
    $stmt->execute([$username]);
    $row = $stmt->fetch();

    if ($row) {
        // 使用 bcrypt 验证（新用户）
        if (password_verify($password, $row['password'])) {
            $_SESSION['username'] = $username;
            $token = bin2hex(random_bytes(32));
            $_SESSION['session_token'] = $token;
            $update = $pdo->prepare("UPDATE users SET session_token = ? WHERE username = ?");
            $update->execute([$token, $username]);
            header("Location: chat.php");
            exit;
        }
        // 兼容老用户 admin (SHA-256)
        elseif (hash('sha256', $password) == $row['password']) {
            // 自动升级为 bcrypt
            $newHash = password_hash($password, PASSWORD_DEFAULT);
            $update = $pdo->prepare("UPDATE users SET password = ? WHERE username = ?");
            $update->execute([$newHash, $username]);
            $_SESSION['username'] = $username;
            $token = bin2hex(random_bytes(32));
            $_SESSION['session_token'] = $token;
            $update2 = $pdo->prepare("UPDATE users SET session_token = ? WHERE username = ?");
            $update2->execute([$token, $username]);
            header("Location: chat.php");
            exit;
        }
    }
    $error = "用户名或密码错误";
}
?>
<!DOCTYPE html>
<html>
<head><meta charset="UTF-8"><title>登录</title></head>
<body>
    <h2>登录</h2>
    <?php if (isset($error)) echo "<p style='color:red'>$error</p>"; ?>
    <form method="POST">
        <input type="text" name="username" placeholder="用户名" required><br>
        <input type="password" name="password" placeholder="密码" required><br>
        <input type="submit" value="登录">
    </form>
    <p>还没有账户？<a href="register.php">立即注册</a></p>
</body>
</html>
