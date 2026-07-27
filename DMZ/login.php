<?php
session_start();
if (isset($_SESSION['username'])) {
    header("Location: chat.php");
    exit;
}

if ($_SERVER['REQUEST_METHOD'] == 'POST') {
    require 'db.php';
    $username = $_POST['username'] ?? '';
    $password = hash('sha256', $_POST['password'] ?? '');
    $stmt = $pdo->prepare("SELECT * FROM users WHERE username = ? AND password = ?");
    $stmt->execute([$username, $password]);
    if ($stmt->rowCount() > 0) {
	    $_SESSION['username'] = $username;
	    $token = bin2hex(random_bytes(32));
        $_SESSION['session_token'] = $token;
        $update = $pdo->prepare("UPDATE users SET session_token = ? WHERE username = ?");
        $update->execute([$token, $username]);
        header("Location: chat.php");
        exit;
    } else {
        $error = "用户名或密码错误";
    }
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
</body>
</html>
<p>还没有账户？<a href="register.php">立即注册</a></p>
