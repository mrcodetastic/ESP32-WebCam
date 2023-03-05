
/*********** Wifi Config. ***********/
//Replace with your network credentials
const char* ssid = "xxxxxxxxxx";
const char* password = "xxxxxxxxxx";


#define USE_DHCP

// If 'USE_DHCP' is defined, no need to adjust with manual entries below.

// it wil set the static IP address to 192, 168, 1, 184
IPAddress local_IP(192, 168, 8, 101);
//it wil set the gateway static IP address to 192, 168, 1,1
IPAddress gateway(192, 168, 8, 1);

// Following three settings are optional
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8); 
IPAddress secondaryDNS(8, 8, 4, 4);

/****** Remote Capture Server address ******/
// To send the image to a remote server
String serverName = "xxxxxxxxxx";   
String serverPath = "/webcam/upload.php";     // The default serverPath should be upload.php
const int serverPort = 80;

/******  Capture Frequency (Minutes). ******/
const int timerInterval = 60;    // time between each HTTP POST image = Every hour
const int photoLightThreshold = 50;  // Value from 0-255 - what is the  brightness of the photo before we upload a photo?



