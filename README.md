# LoRa-gpstrack

A **LoRa GPS tracker** sensor for 
 - Arduino Uno and 
 - Dragino LoRa GPS Shield

for use in LoRaWAN networks.

This application sends
 - current GPS position,
 - altitude and
 - hdop

as soon as a GPS fix is aquired. This data is sent continously using LMIC library. The device is powered by any source that fits to Arduino Uno. We use powerbanks for mobile phone charging. They run a sensor for weeks.

We use this tracker to measure the range of the LoRaWAN Gateways in our network. **The application's output format is compatible for use with [TTNmapper](http://ttnmapper.org).**

It might suite for any other GPS tracking use case as well. We just included to the values we required but it can easily be extended to more values. Keep in mind that more data means longer transmission times and duty cycles, especially on high spreading factors.

## Installation
What you need:
 - Arduino Uno and 
 - Dragino LoRa GPS Shield
 - power source for powering Arduino

Put together the pieces as described in the vendor's manual.

Change the 
 - NWKSKEY
 - APPSKEY
 - DEVADDR

variable accordint to values of the device you created in the The Things Network Console.

Program the application to your Arduino Uno. If you have problems while programming, try pressing the reset button of the Shield during programming.

### Configuration Options

Optionally you can adjust these values:
- TX_INTERVAL
Set to define the pause time in seconds between transmissions. (Default is 60 to send one message per minute.)
 - GPS_INTERVAL
poll interval of GPS DATA
 - lmic_pinmap
PIN mapping if not using Dragino GPS LoRa Shield
 - LMIC_setDrTxpow
set Spreading Factor (SF7 -> SF12) and transmission power in dB
 - Debug Mode (DEBUG_SS)
see chapter Debug Mode

### Debug mode
Debug mode enables debugging of the reception of GPS data via SoftSerial. On Arduino Uno it is not possible to run debug mode and LoRa transmission at the same time due to memory restrictions. To enable debug mode define DEBUG_SS:

    #define DEBUG_SS

# FAQ & known problems

##### I get an error when trying to program the Arduino, what to do?
Press the reset button of the Shield while programming.

##### I want to only transmit on one channel, is this possible?
By default the LMIC library transmits on three default channels. You can disable the wanted channels by looking for these lines in the code:
        
    LMIC_disableChannel(0);  // uncomment to disable channel 0
    LMIC_disableChannel(1);  // uncomment to disable channel 1
    LMIC_disableChannel(2);  // uncomment to disable channel 2

##### Can I use this code also with other boards?
This should be possible. You will have to adjust the PIN mapping constant called lmic_pinmap. For GPS a serial port with 9600 baud is used.

# Credits
This script is based on the work by Thomas Telkamp, Matthijs Kooijman and Andreas Spiess.
