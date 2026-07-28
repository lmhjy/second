<?php
require_once 'check_auth.php';
session_start();
if (!isset($_SESSION['username'])) exit;

require 'db.php';

$stmt = $pdo->query("SELECT username, message, created_at FROM messages ORDER BY created_at ASC");
while ($row = $stmt->fetch()) {
    echo '<div class="msg">';
    echo '<span class="user">' . htmlspecialchars($row['username']) . ':</span>';
    echo htmlspecialchars($row['message']);
    echo '<span class="time">(' . $row['created_at'] . ')</span>';
    echo '</div>';
}
?>
