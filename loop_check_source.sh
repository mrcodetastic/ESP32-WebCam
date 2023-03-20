#!/bin/bash
#
# Because the ESP32-Cam can only stream to one client at a time and that I don't want
# to expose it directly to the internet, I use a golang based mjpeg proxy running on a 
# cheap micro linux vps server
#
# As a result we can stream to an unlimited number of end users from a single esp32-cam.
#
# However, the proxy needs to be restarted should the source address change.
#
# https://github.com/mrfaptastic/mjpeg-proxy
#

# Initialize the previous value
previous_value=""

dest_source_addr_file="https://xxxxxxxxxxxxx/remote_addr.txt" # a file that contains a domain or changing ip address
dest_source_port="123456"

while true
do
    # Get the value from the HTTP query parameter
    value=$(curl --silent --fail "$dest_source_addr_file")
    echo "Getting source addr to see if a change has occurred: $value"
    
    # Check if the value has changed
    if [[ "$value" != "$previous_value" ]] && [[ "$value" != "" ]]; then

        # Kill the processes that match the wildcard
        PIDS=$(pgrep "mjpeg-proxy*")
        if [[ -z "$PIDS" ]]; then
            echo "No processes found matching the search criteria."
        else
            echo "Killing existing mjpeg-proxy processes: "
            echo "$PIDS"
            kill $PIDS
        fi
        
        # Start the Golang process with the new value as a parameter
        echo "Source address has changed. Restarting mjpeg proxy"
        go run mjpeg-proxy.go chunker.go digest.go pubsub.go  -bind ":20000" -source "http://$value:$dest_source_port" &
        
        # Update the previous value
        previous_value="$value"
    fi
    
    # Sleep for 30 minutes
    sleep 1800
	
done
