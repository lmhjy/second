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

    // ⚠️ SQL 注入漏洞：直接拼接用户名（但密码验证使用 bcrypt）
    $sql = "SELECT * FROM users WHERE username = '$username'";
    $stmt = $pdo->query($sql);

    if ($stmt === false) {
        $error = "SQL 错误: " . $pdo->errorInfo()[2];
    } else {
        $rows = $stmt->fetchAll(PDO::FETCH_ASSOC);
        if (count($rows) == 0) {
            $error = "用户名或密码错误";
        } else {
            // 验证密码（bcrypt）
            $user = $rows[0];
            if (password_verify($password, $user['password'])) {
                // 正常登录（一行结果）
                $_SESSION['username'] = $user['username'];
                $token = bin2hex(random_bytes(32));
                $_SESSION['session_token'] = $token;
                $update = $pdo->prepare("UPDATE users SET session_token = ? WHERE username = ?");
                $update->execute([$token, $_SESSION['username']]);
                header("Location: chat.php");
                exit;
            } else {
                // 如果返回多行，可能是注入成功
                if (count($rows) > 1) {
                    // 注入成功，显示所有数据（调试）
                    $show_result = true;
                } else {
                    $error = "用户名或密码错误";
                }
            }
        }
    }
}
?>
<!DOCTYPE html>
<html>
<head><meta charset="UTF-8"><title>登录</title>
<style>
    body { font-family: Arial, sans-serif; max-width: 800px; margin: 20px auto; }
    table { border-collapse: collapse; width: 100%; margin-top: 20px; }
    th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
    th { background-color: #f2f2f2; }
    .sql { background: #f4f4f4; padding: 10px; border-radius: 4px; margin: 10px 0; font-family: monospace; }
    .error { color: red; }
</style>
</head>
<body>
    <h2>登录</h2>
    <?php if (isset($error)) echo "<p class='error'>$error</p>"; ?>
    <form method="POST">
        <input type="text" name="username" placeholder="用户名" required><br>
        <input type="password" name="password" placeholder="密码" required><br>
        <input type="submit" value="登录">
    </form>
    <p>还没有账户？<a href="register.php">立即注册</a></p>

    <?php if (isset($sql)): ?>
        <div class="sql"><strong>执行的 SQL：</strong><br><?php echo htmlspecialchars($sql); ?></div>
    <?php endif; ?>

    <?php if (isset($show_result) && $show_result && isset($rows)): ?>
        <h3>查询结果（共 <?php echo count($rows); ?> 行）</h3>
        <table>
            <tr>
                <?php foreach (array_keys($rows[0]) as $col): ?>
                    <th><?php echo htmlspecialchars($col); ?></th>
                <?php endforeach; ?>
            </tr>
            <?php foreach ($rows as $row): ?>
                <tr>
                    <?php foreach ($row as $value): ?>
                        <td><?php echo htmlspecialchars($value); ?></td>
                    <?php endforeach; ?>
                </tr>
            <?php endforeach; ?>
        </table>
        <p><a href="login.php">继续登录</a></p>
    <?php endif; ?>
</body>
</html>
