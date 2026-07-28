<?php
require_once 'check_auth.php';
session_start();
if (!isset($_SESSION['username'])) {
    header("Location: login.php");
    exit;
}
$username = $_SESSION['username'];

require 'db.php';

// 处理发送消息
if ($_SERVER['REQUEST_METHOD'] == 'POST' && isset($_POST['message'])) {
    $msg = trim($_POST['message']);
    if ($msg != '') {
        $stmt = $pdo->prepare("INSERT INTO messages (username, message) VALUES (?, ?)");
        $stmt->execute([$username, $msg]);
    }
    header("Location: chat.php");
    exit;
}

// 获取所有历史消息
$stmt = $pdo->query("SELECT username, message, created_at FROM messages ORDER BY created_at ASC");
$messages = $stmt->fetchAll(PDO::FETCH_ASSOC);
?>
<!DOCTYPE html>
<html>
<head><meta charset="UTF-8"><title>公共聊天室</title>
<style>
    #chat-box { border:1px solid #ccc; height:400px; overflow-y:scroll; padding:10px; margin-bottom:10px; }
    .msg { margin:5px 0; }
    .user { font-weight:bold; color:#0077cc; }
    .time { font-size:0.8em; color:#999; }
</style>
<script src="https://code.jquery.com/jquery-3.6.0.min.js"></script>
    <script>
        function loadMessages() {
            $('#chat-box').load('get_messages.php');
        }

        // 每 3 秒自动刷新一次消息
        setInterval(loadMessages, 3000);

        // 发送消息后等待 500ms 再刷新（确保消息已写入数据库）
        $(document).on('submit', 'form', function() {
            setTimeout(loadMessages, 500);
        });

        // 页面加载完成后立即加载消息
        $(document).ready(function() {
            loadMessages();
        });
    </script>
</head>
<body>
    <h2>公共聊天室 - 欢迎 <?php echo htmlspecialchars($username); ?></h2>
    <div id="chat-box">
        <?php foreach ($messages as $msg): ?>
            <div class="msg">
                <span class="user"><?php echo htmlspecialchars($msg['username']); ?>:</span>
                <?php echo htmlspecialchars($msg['message']); ?>
                <span class="time">(<?php echo $msg['created_at']; ?>)</span>
            </div>
        <?php endforeach; ?>
    </div>
    <form method="POST">
        <input type="text" name="message" placeholder="输入消息..." style="width:70%;" required>
        <input type="submit" value="发送">
    </form>
    <p><a href="logout.php">退出登录</a></p>
</body>
</html>
