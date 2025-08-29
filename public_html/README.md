# ESP32 WebCam Gallery - File Structure

## Web Files

### Main Files
- `index.php` - Main gallery page with PHP backend logic
- `upload.php` - Handles photo uploads from ESP32-CAM
- `config.php` - Configuration settings including timezone
- `demo.html` - Feature demonstration page

### Assets
- `styles.css` - All CSS styling for the gallery interface
- `scripts.js` - JavaScript functionality for interactive features
- `stream_error.jpg` - Fallback image for live stream errors

### Directories
- `uploads/` - Directory where uploaded photos are stored
- `images/` - Static images for documentation

## Configuration

### config.php Settings
- `$image_dir` - Directory for uploaded images (default: './uploads')
- `$stream_remote_port` - Port for ESP32 live stream (default: '38343')
- `$stream_remote_ip_file` - File storing the remote IP address
- `$timezone` - Timezone for timestamp display (default: 'Europe/London')

### Timezone Options
You can change the timezone in `config.php` to any valid PHP timezone:
- `Europe/London` (UK)
- `America/New_York` (US Eastern)
- `America/Los_Angeles` (US Pacific)
- `Europe/Paris` (Central Europe)
- `Asia/Tokyo` (Japan)
- `Australia/Sydney` (Australia)

Full list: https://www.php.net/manual/en/timezones.php

## Features

### Gallery Interface
- YouTube-style thumbnail timeline
- Click thumbnails to view full images
- Responsive design for mobile/desktop
- Live stream integration
- Automatic image cleanup (keeps latest 16 images)

### Security Improvements
- File validation and size limits
- Malicious content detection
- Secure filename generation
- Automatic old file cleanup
- Enhanced error handling

## File Dependencies
- `index.php` requires: `config.php`, `styles.css`, `scripts.js`
- `upload.php` requires: `config.php` (for timezone if needed)
- All files work independently but are designed to work together

## Browser Compatibility
- Modern browsers with CSS Grid and Flexbox support
- JavaScript ES6+ features
- Responsive design for mobile devices
