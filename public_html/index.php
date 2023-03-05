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
//print_r($r); 

if ( count($r) == 0)
{
	exit();
}

// Avoid caching
header("Cache-Control: no-store, no-cache, must-revalidate, max-age=0");
header("Cache-Control: post-check=0, pre-check=0", false);
header("Pragma: no-cache");
header('Expires: '. gmdate('D, d M Y H:i:s \G\M\T', time() + (60 * 10))); // 10 minutes

?><!DOCTYPE html>
<head>
  <meta charset="utf-8">
  <title>WebCam</title>
	<meta http-equiv="cache-control" content="max-age=0" />
	<meta http-equiv="cache-control" content="no-cache" />
	<meta http-equiv="expires" content="0" />
	<meta http-equiv="expires" content="Tue, 01 Jan 1980 1:00:00 GMT" />
	<meta http-equiv="pragma" content="no-cache" />
	<meta http-equiv="refresh" content="900" > 

  <style>
  
 	body { font-family:Arial; }
	
	button {
	  display: inline-block;
	  background-color: #7b38d8;
	  padding: 20px;
	  width: 200px;
	  color: #ffffff;
	  text-align: center;
	  border: 4px double #cccccc; /* add this line */
	  border-radius: 10px; /* add this line */
	  font-size: 16px; /* add this line */
	}
	

	img {
      width: 100%;
      height: auto;
    }
   
   </style>
   
   <script>
   var stream_is_visible = false;
   function showESP32Stream() 
   {
	   
	   if (!stream_is_visible) {
		document.getElementById('jpg_stream').innerHTML = '<div id="livestream" style="display:block; padding-bottom: 1.5em; font-size: 2em;"><strong>Livestream</strong><br /><img src="<?php echo $stream_remote_img_src; ?>" onerror="this.onerror=null; this.src=\'stream_error.jpg\'" alt="" alt="ESP32 based live stream" width="1600" height="1200">';
		
		console.log ("Showing ESP32 JPG stream.");
	   } 
	  
	}
   </script>

   
</head>
<body>
<div class="content">
<div id="jpg_stream"><button onclick="showESP32Stream()">Show live stream</button></div>

<?php

$counter = 0;
$prev_date = "";

foreach ($r AS $image)
{
	$file_path = $image_dir."/".$image ;
	
	if ($counter > 16)
	{
		unlink($file_path);
		continue;
	}	
	
	if (file_exists($file_path)) 
	{
		
    	$unixTime = time();
		$timeZone = new DateTimeZone('Europe/London');		
		$timeZone2 = new DateTimeZone('Europe/London');		// alternative if required
		
		$time = new DateTime();

		$time->setTimestamp(filectime($file_path))->setTimezone($timeZone);
		$formattedTime = $time->format('g:ia');

		$header_date = $time->format('l j F');
				
		if ( $prev_date !=  $header_date) { 
				if ($prev_date != "") echo "<hr />";
				
				echo "<h1>". $header_date ."</h1>";
				$prev_date = $header_date; 
		}
		
		echo "<div id=\"image_$counter\" style=\"display:block; padding-bottom: 1.5em; font-size: 2em;\"><strong>".$formattedTime ."</strong><br />";    	
		
		echo '<img src="'. $file_path .'" style="padding-top:0.5em;" alt="" /></div>';
		$counter++;
		
	}

}
?>
</div>	

</body>
</html>
