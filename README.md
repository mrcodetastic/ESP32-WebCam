# ESP32-WebCam
 Streaming and static photo capture webcam with website.

 * Select 'AI Thinker ESP32-CAM' in Arduino to compile.
 * Based on 'RandomNerdTutorials' and the 'ESP32-Cam' repositories. 

 ## Functionality

 * Brightness / luminosity based photo capture. Only take photos during the day when the brightness is of a sufficient level.
 * Uploads captures periodically (based on configuration) to remote server as well as web stream.
 * Telnet interface to support basic diagnostics.
 
 
 ## Best used with a mjpeg-proxy
Because the ESP32-Cam can only stream to one client at a time and that I don't want to expose it directly to the internet, I use a golang based mjpeg proxy running on a  cheap micro linux vps server

As a result we can stream to an unlimited number of end users from a single esp32-cam.

However, the proxy needs to be restarted should the source address change. Refer to the 'loop_check_source.sh' script.


![Image 1](images/website.jpg)

![Image 2](images/telnet.png)



