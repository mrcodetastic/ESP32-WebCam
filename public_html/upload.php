<?php
// Improved ESP32-CAM Photo Upload Handler
// Based on Rui Santos example: https://RandomNerdTutorials.com/esp32-cam-post-image-photo-server/

// Security headers
header('X-Content-Type-Options: nosniff');
header('X-Frame-Options: DENY');
header('X-XSS-Protection: 1; mode=block');

// Configuration
$target_dir = "uploads/";
$max_file_size = 2 * 1024 * 1024; // 2MB limit
$allowed_types = ['jpg', 'jpeg', 'png', 'gif'];
$max_files_to_keep = 50; // Maximum number of files to keep

// Create uploads directory if it doesn't exist
if (!is_dir($target_dir)) {
    if (!mkdir($target_dir, 0755, true)) {
        http_response_code(500);
        die(json_encode(['error' => 'Failed to create upload directory']));
    }
}

// Function to clean up old files
function cleanupOldFiles($dir, $max_files) {
    $files = glob($dir . "*.{jpg,jpeg,png,gif}", GLOB_BRACE);
    if (count($files) > $max_files) {
        // Sort by modification time, oldest first
        usort($files, function($a, $b) {
            return filemtime($a) - filemtime($b);
        });
        
        // Delete oldest files
        $files_to_delete = count($files) - $max_files;
        for ($i = 0; $i < $files_to_delete; $i++) {
            if (file_exists($files[$i])) {
                unlink($files[$i]);
            }
        }
    }
}

// Function to validate image
function validateImage($file) {
    // Check if it's a real image
    $check = getimagesize($file['tmp_name']);
    if ($check === false) {
        return ['valid' => false, 'error' => 'File is not a valid image'];
    }
    
    // Additional security: check for embedded PHP code
    $file_content = file_get_contents($file['tmp_name'], false, null, 0, 1024);
    if (strpos($file_content, '<?php') !== false || strpos($file_content, '<?=') !== false) {
        return ['valid' => false, 'error' => 'File contains suspicious content'];
    }
    
    return ['valid' => true, 'mime' => $check['mime']];
}

// Function to generate secure filename
function generateSecureFilename($original_name) {
    $timestamp = date('Y.m.d_H_i_s');
    $extension = strtolower(pathinfo($original_name, PATHINFO_EXTENSION));
    
    // Generate random component for uniqueness
    $random = bin2hex(random_bytes(4));
    
    return $timestamp . '_' . $random . '.' . $extension;
}

// Main upload logic
$response = ['success' => false, 'message' => ''];

try {
    // Check if file was uploaded
    if (!isset($_FILES["imageFile"]) || $_FILES["imageFile"]["error"] !== UPLOAD_ERR_OK) {
        throw new Exception('No file uploaded or upload error occurred');
    }
    
    $uploaded_file = $_FILES["imageFile"];
    
    // Validate file size
    if ($uploaded_file["size"] > $max_file_size) {
        throw new Exception('File size exceeds limit of ' . ($max_file_size / 1024 / 1024) . 'MB');
    }
    
    // Get file extension
    $file_extension = strtolower(pathinfo($uploaded_file["name"], PATHINFO_EXTENSION));
    
    // Validate file type
    if (!in_array($file_extension, $allowed_types)) {
        throw new Exception('Only ' . implode(', ', array_map('strtoupper', $allowed_types)) . ' files are allowed');
    }
    
    // Validate image integrity
    $validation = validateImage($uploaded_file);
    if (!$validation['valid']) {
        throw new Exception($validation['error']);
    }
    
    // Generate secure filename
    $target_filename = generateSecureFilename($uploaded_file["name"]);
    $target_file = $target_dir . $target_filename;
    
    // Check if file already exists (very unlikely with timestamp + random)
    if (file_exists($target_file)) {
        throw new Exception('File already exists (please try again)');
    }
    
    // Move uploaded file
    if (!move_uploaded_file($uploaded_file["tmp_name"], $target_file)) {
        throw new Exception('Failed to save uploaded file');
    }
    
    // Set proper file permissions
    chmod($target_file, 0644);
    
    // Store the remote IP address
    $remote_ip = $_SERVER['REMOTE_ADDR'] ?? 'unknown';
    
    // Validate IP address
    if (filter_var($remote_ip, FILTER_VALIDATE_IP)) {
        file_put_contents('remote_addr.txt', $remote_ip);
    }
    
    // Clean up old files
    cleanupOldFiles($target_dir, $max_files_to_keep);
    
    // Success response
    $response['success'] = true;
    $response['message'] = 'File uploaded successfully';
    $response['filename'] = $target_filename;
    $response['mime_type'] = $validation['mime'];
    $response['size'] = $uploaded_file["size"];
    
    // Log successful upload
    error_log("ESP32-CAM: Successfully uploaded {$target_filename} from {$remote_ip}");
    
} catch (Exception $e) {
    $response['message'] = $e->getMessage();
    error_log("ESP32-CAM Upload Error: " . $e->getMessage());
    http_response_code(400);
}

// Return JSON response for API clients, plain text for simple clients
$user_agent = $_SERVER['HTTP_USER_AGENT'] ?? '';
if (strpos($user_agent, 'ESP32') !== false || isset($_GET['format']) && $_GET['format'] === 'json') {
    header('Content-Type: application/json');
    echo json_encode($response);
} else {
    // Plain text response for browser compatibility
    echo $response['message'];
}
?>
