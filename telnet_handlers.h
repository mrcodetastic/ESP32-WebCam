ESPTelnet telnet;

void onTelnetConnect(String ip) {
  Serial.print("- Telnet: ");
  Serial.print(ip);
  Serial.println(" connected");
  
  telnet.println("\nWelcome " + telnet.getIP());
  telnet.println("(Use ^] + q  to disconnect.)");
}

void onTelnetInput(String str) {
  // checks for a certain command
  if (str == "ping") {
    telnet.println("> pong");
    Serial.println("- Telnet: pong");
  // disconnect the client
  } else if (str == "bye") {
    telnet.println("> disconnecting you...");
    telnet.disconnectClient();
  }
   else if (str == "capture")
   {
     telnet.println("Capturing photo for upload.");
     sendPhoto();
   }
   else if (str == "status")
   {
      if ( (millis() - lastStreamMillis) < 5000 ) 
      {
       telnet.println("Stream is active. Somebody is streaming.");
       } else { telnet.println("Streaming NOT currently being used."); }

       unsigned long lastCapture = millis() - previousMillis;

       telnet.print("Last photo capture upload taken ");
       telnet.print(String((lastCapture/1000/60)));
       telnet.println(" minutes ago.");

   }   
  }


void setupTelnet() {  
  // passing on functions for various telnet events
  telnet.onConnect(onTelnetConnect);
//  telnet.onConnectionAttempt(onTelnetConnectionAttempt);
//  telnet.onReconnect(onTelnetReconnect);
//  telnet.onDisconnect(onTelnetDisconnect);
  telnet.onInputReceived(onTelnetInput);

  Serial.print("- Telnet: ");
  if (telnet.begin(23)) {
    Serial.println("running");
  } else {
    Serial.println("error.");
   // errorMsg("Will reboot...");
  }
}  