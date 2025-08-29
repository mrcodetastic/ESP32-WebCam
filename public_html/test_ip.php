<?php
// Test script to verify IP address saving functionality
// Usage: Place this file in the same directory and access via browser

include 'config.php';

echo "<h2>ESP32-CAM IP Address Test</h2>";

// Function to get real client IP (same as in upload.php)
function getRealClientIP() {
    $ip_headers = [
        'HTTP_CF_CONNECTING_IP',
        'HTTP_CLIENT_IP',
        'HTTP_X_FORWARDED_FOR',
        'HTTP_X_FORWARDED',
        'HTTP_X_CLUSTER_CLIENT_IP',
        'HTTP_FORWARDED_FOR',
        'HTTP_FORWARDED',
        'REMOTE_ADDR'
    ];
    
    foreach ($ip_headers as $header) {
        if (!empty($_SERVER[$header])) {
            $ip = $_SERVER[$header];
            
            if (strpos($ip, ',') !== false) {
                $ip_list = explode(',', $ip);
                $ip = trim($ip_list[0]);
            }
            
            if (filter_var(trim($ip), FILTER_VALIDATE_IP, FILTER_FLAG_NO_PRIV_RANGE | FILTER_FLAG_NO_RES_RANGE)) {
                return trim($ip);
            } elseif (filter_var(trim($ip), FILTER_VALIDATE_IP)) {
                $fallback_ip = trim($ip);
            }
        }
    }
    
    return isset($fallback_ip) ? $fallback_ip : 'unknown';
}

// Current detected IP
$current_ip = getRealClientIP();
echo "<p><strong>Currently detected IP:</strong> {$current_ip}</p>";

// Show what's in the IP file
echo "<h3>Current stored IP address:</h3>";
if (file_exists($stream_remote_ip_file)) {
    $stored_ip = file_get_contents($stream_remote_ip_file);
    echo "<p><strong>Stored IP:</strong> {$stored_ip}</p>";
    echo "<p><strong>File location:</strong> {$stream_remote_ip_file}</p>";
} else {
    echo "<p><em>IP file does not exist yet: {$stream_remote_ip_file}</em></p>";
}

// Test writing current IP
if (isset($_GET['update'])) {
    if (file_put_contents($stream_remote_ip_file, $current_ip) !== false) {
        echo "<p style='color: green;'>✅ Successfully updated IP file with: {$current_ip}</p>";
    } else {
        echo "<p style='color: red;'>❌ Failed to write to IP file</p>";
    }
}

// Show all IP-related headers
echo "<h3>All IP-related headers:</h3>";
echo "<table border='1' cellpadding='5'>";
echo "<tr><th>Header</th><th>Value</th></tr>";

$headers_to_check = [
    'REMOTE_ADDR',
    'HTTP_CLIENT_IP',
    'HTTP_X_FORWARDED_FOR',
    'HTTP_X_FORWARDED',
    'HTTP_X_CLUSTER_CLIENT_IP',
    'HTTP_FORWARDED_FOR',
    'HTTP_FORWARDED',
    'HTTP_CF_CONNECTING_IP'
];

foreach ($headers_to_check as $header) {
    $value = $_SERVER[$header] ?? '<em>not set</em>';
    echo "<tr><td>{$header}</td><td>{$value}</td></tr>";
}
echo "</table>";

echo "<p><a href='?update=1'>Click here to update IP file with current IP</a></p>";
echo "<p><a href='index.php'>Back to Gallery</a></p>";
?>

<style>
body { font-family: Arial, sans-serif; margin: 20px; }
table { border-collapse: collapse; margin: 10px 0; }
th { background: #f0f0f0; }
</style>
