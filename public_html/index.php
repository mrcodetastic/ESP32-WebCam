<?php

/*
ini_set('display_errors', '1');
ini_set('display_startup_errors', '1');
error_reporting(E_ALL);
*/

include 'config.php';

if(!is_file($stream_remote_ip_file)){
    $contents = '';           // Some simple example content.
    file_put_contents($stream_remote_ip_file, $contents);     // Save our content to the file.
}

$stream_remote_ip = file_get_contents($stream_remote_ip_file);
$stream_remote_img_src = 'http://' . $stream_remote_ip . ':' . $stream_remote_port;

//https://www.php.net/manual/en/function.scandir.php
function myscandir($dir, $exp, $how='name', $desc=0)
{
    $r = array();
    $dh = @opendir($dir);
    if ($dh) {
        while (($fname = readdir($dh)) !== false) {
            if (preg_match($exp, $fname)) {
                $stat = stat("$dir/$fname");
                $r[$fname] = ($how == 'name')? $fname: $stat[$how];
            }
        }
        closedir($dh);
        if ($desc) {
            arsort($r);
        }
        else {
            asort($r);
        }
    }
    return(array_keys($r));
}

$r = myscandir($image_dir, '/.jpg/i', 'ctime', 1);

if ( count($r) == 0)
{
	exit('<div class="no-images">No images found. Please check your webcam connection.</div>');
}

// Prepare image data for JavaScript
$image_data = array();
foreach ($r as $index => $image) {
    $file_path = $image_dir."/".$image;
    if (file_exists($file_path)) {
        $timeZone = new DateTimeZone($timezone);
        $time = new DateTime();
        $time->setTimestamp(filectime($file_path))->setTimezone($timeZone);
        
        $image_data[] = array(
            'filename' => $image,
            'path' => $file_path,
            'timestamp' => filectime($file_path),
            'formatted_time' => $time->format('g:ia'),
            'formatted_date' => $time->format('l j F'),
            'index' => $index
        );
    }
}

// Avoid caching
header("Cache-Control: no-store, no-cache, must-revalidate, max-age=0");
header("Cache-Control: post-check=0, pre-check=0", false);
header("Pragma: no-cache");
header('Expires: '. gmdate('D, d M Y H:i:s \G\M\T', time() + (60 * 10))); // 10 minutes

?><!DOCTYPE html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 WebCam Gallery</title>
  <meta http-equiv="cache-control" content="max-age=0" />
  <meta http-equiv="cache-control" content="no-cache" />
  <meta http-equiv="expires" content="0" />
  <meta http-equiv="expires" content="Tue, 01 Jan 1980 1:00:00 GMT" />
  <meta http-equiv="pragma" content="no-cache" />
  <meta http-equiv="refresh" content="900" > 

  <!-- External CSS -->
  <link rel="stylesheet" href="styles.css">

  <!-- External JavaScript -->
  <script src="scripts.js"></script>
  
  <!-- Initialize image data -->
  <script>
    // Pass PHP data to JavaScript
    document.addEventListener('DOMContentLoaded', function() {
      initializeImageData(<?php echo json_encode($image_data); ?>);
    });
  </script>
</head>
<body>
  <div class="header">
    <h1>🎥 ESP32 WebCam Gallery</h1>
    <div class="live-stream-container">
      <button class="stream-btn" onclick="showESP32Stream()">Show Live Stream</button>
    </div>
  </div>

  <div id="livestream-container" class="livestream-container">
    <div class="livestream-title">📡 Live Stream</div>
    <img class="livestream-image" src="<?php echo $stream_remote_img_src; ?>" onerror="this.onerror=null; this.src='stream_error.jpg'" alt="ESP32 Live Stream">
  </div>

  <!-- Thumbnail Timeline -->
  <div class="thumbnail-timeline">
    <div class="timeline-title">📸 Photo Timeline</div>
    <div class="timeline-container">
      <?php foreach ($image_data as $index => $data): ?>
        <div class="timeline-item" data-index="<?php echo $index; ?>" onclick="selectImage(<?php echo $index; ?>)">
          <img class="timeline-thumb" src="<?php echo $data['path']; ?>" alt="Thumbnail">
          <div class="timeline-time"><?php echo $data['formatted_time']; ?></div>
        </div>
      <?php endforeach; ?>
    </div>
  </div>

  <!-- Selected Image Display -->
  <div id="selected-image-container" class="selected-image-container">
    <img id="selected-image" class="selected-image" src="" alt="Selected Image">
    <div id="selected-info" class="selected-info"></div>
  </div>

  <!-- Full Gallery -->
  <div class="gallery-container">
    <div class="gallery-title">📅 Complete Gallery</div>

    <?php
    $counter = 0;
    $prev_date = "";

    foreach ($image_data as $data) {
      $file_path = $data['path'];
      
      if ($counter > 16) {
        unlink($file_path);
        continue;
      }	
      
      if (file_exists($file_path)) {
        $header_date = $data['formatted_date'];
        
        if ($prev_date != $header_date) { 
          echo "<div class='date-header'>📅 " . $header_date . "</div>";
          $prev_date = $header_date; 
        }
        
        echo "<div id='image_$counter' class='image-item'>";
        echo "<div class='image-time'>🕐 " . $data['formatted_time'] . "</div>";
        echo "<img class='gallery-image' src='" . $file_path . "' alt='Webcam capture' onclick='scrollToGalleryImage($counter)' />";
        echo "</div>";
        $counter++;
      }
    }
    ?>
  </div>

</body>
</html>
