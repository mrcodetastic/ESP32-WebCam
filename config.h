/****** Remote Capture Server address ******/
// To send the image to a remote server
String serverName    = "xxxxxxxxxxxxxx";   
String serverPath    = "/webcam/upload.php";     // The default serverPath should be upload.php
const int serverPort = 80;

/******  Capture Frequency (Minutes). ******/
const int timerInterval       = 60;    // time between each HTTP POST image = Every hour
const int photoLightThreshold = 50;  // Value from 0-255 - what is the  brightness of the photo before we upload a photo?



