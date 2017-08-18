#include <lmic.h>
#include <hal/hal.h>
#include <TinyGPS.h>

// enable/disable debug on SoftSerial //#define DEBUG_SS -> disabled
// NOTE: on UNO debug and LorRa Tx isn't possible at the same time due to memory limitation. 
//       If you enable debug only output on SoftSerial is generated but no LoRa Tx will take place
//
//#define DEBUG_SS
#ifdef DEBUG_SS
 #include <SoftwareSerial.h>
 // debug string for data send over LoRa
 String toLog;
 // define pins for SoftSerial Tx/Rx
 SoftwareSerial ss(3, 4);
#endif

// init TinyGPS
TinyGPS gps;

// Buffer data for LoRa packet transmission
uint8_t txBuffer[9];
// variables for GPS data we want to transmit
uint8_t hdo_gps;
uint16_t alt_gps;
uint32_t lat_gps_b, lon_gps_b;

// Insert your KEYs and DEVADDR here
static const u1_t NWKSKEY[16] = { 0x66, 0x1A, 0x0A, 0xA2, 0x76, 0x21, 0x6A, 0x60, 0x04, 0x4F, 0x4A, 0x6A, 0xD1, 0xC1, 0x68, 0xDD };
static const u1_t APPSKEY[16] = { 0xBF, 0xD8, 0x80, 0x47, 0x50, 0x3F, 0xE6, 0x13, 0x90, 0x26, 0x91, 0xC2, 0xD0, 0x91, 0xF1, 0xDF };
static const u4_t DEVADDR = 0x26011A80;

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static osjob_t sendjob;

// TX_INTERVAL = seconds to wait after LoRa TX for new transmission
// GPS_INTERVAL = poll interval of GPS DATA in case of NO lock to satelite
const unsigned TX_INTERVAL = 60;
const unsigned GPS_INTERVAL = 2;

// Pin mapping Dragino Shield
const lmic_pinmap lmic_pins = {
  .nss = 10,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 9,
  .dio = {2, 6, 7},
};

// catch LoRa TXCOMPLETE and restart GPS position sending(do_send) after TX_INTERVAL
void onEvent (ev_t ev) {
  if (ev == EV_TXCOMPLETE) {
    #ifdef DEBUG_SS 
      ss.println(F("EV_TXCOMPLETE (includes waiting for RX windows)")); 
    #endif
    os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
  }
}

void do_send(osjob_t* j) {
  
  unsigned long age = 0;
  float flat, flon;
  unsigned long fix_age;
  
  gpsdelay(1000);
  gps.f_get_position(&flat, &flon, &fix_age);
  
  if (fix_age == TinyGPS::GPS_INVALID_AGE) {
    #ifdef DEBUG_SS
      ss.print("No fix detected @ ");ss.println(os_getTime());
    #endif
    os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(GPS_INTERVAL), do_send);
   } 
  else if (fix_age > 5000) {
    #ifdef DEBUG_SS
      ss.print("Warning: possible stale data! @ ");ss.println(os_getTime());
      ss.print("Age: ");ss.println(fix_age);
    #endif
    os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(GPS_INTERVAL), do_send);
   } 
  else if (fix_age < 5000){
    #ifdef DEBUG_SS
      ss.print("Data is fresh. @"); ss.println(os_getTime());
      ss.print("Age: ");ss.println(fix_age);
    #endif
    lat_gps_b = ((flat + 90) / 180) * 16777215;
    lon_gps_b = ((flon + 180) / 360) * 16777215;
    txBuffer[0] = ( lat_gps_b >> 16 ) & 0xFF;
    txBuffer[1] = ( lat_gps_b >> 8 ) & 0xFF;
    txBuffer[2] = lat_gps_b & 0xFF;

    txBuffer[3] = ( lon_gps_b >> 16 ) & 0xFF;
    txBuffer[4] = ( lon_gps_b >> 8 ) & 0xFF;
    txBuffer[5] = lon_gps_b & 0xFF;
  
    alt_gps = gps.f_altitude();
    txBuffer[6] = ( alt_gps >> 8 ) & 0xFF;
    txBuffer[7] = alt_gps & 0xFF;
  
    hdo_gps = gps.hdop()*10;
    txBuffer[8] = hdo_gps & 0xFF;
    #ifdef DEBUG_SS
      toLog = "";
      for(size_t i = 0; i<sizeof(txBuffer); i++)
      {
        char buffer[3];
        sprintf(buffer, "%02x", txBuffer[i]);
        toLog = toLog + String(buffer);
      }
      ss.print("[INFO] Lat:"); ss.println(flat);
      ss.print("[INFO] Lng:"); ss.println(flon);
      ss.print("[INFO] Alt:"); ss.println(alt_gps);
      ss.print("[INFO] HDOP:"); ss.println(hdo_gps);
      ss.print("[INFO] HEX:"); ss.println(toLog);
    #endif
      if (LMIC.opmode & OP_TXRXPEND) {
          #ifdef DEBUG_SS
            ss.println(F("OP_TXRXPEND, not sending"));
          #endif
      } 
      else {
         #ifdef DEBUG_SS
          ss.println("SIMULATING sending of uplink packet...waiting for TX_COMPLETE");
          os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
         #else
          // Prepare upstream data transmission at the next possible time.
          LMIC_setTxData2(1, txBuffer, sizeof(txBuffer), 0);
          // Next TX is scheduled after TX_COMPLETE event.
         #endif
      }
   }
}

static void gpsdelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (Serial.available())
    {
      if (gps.encode(Serial.read()))
      {
        #ifdef DEBUG_SS
          ss.println("NEW GPS Data...");
        #endif
      }
    }
  } while (millis() - start < ms);
}

void setup() {
  Serial.begin(9600);
  #ifdef DEBUG_SS
    ss.begin(9600); // port logging - not all speeds are suported on SoftSerial
    ss.print("STARTING");
  #endif
  os_init();
  LMIC_reset();
  LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
  // Disable link check validation
  LMIC_setLinkCheckMode(0);
  // TTN uses SF9 for its RX2 window.
  LMIC.dn2Dr = DR_SF9;
  // disable channels not known to single channel gateways
  // LMIC_disableChannel(0);  // uncomment to disable channel 0
  // LMIC_disableChannel(1);  // uncomment to disable channel 1
  // LMIC_disableChannel(2);  // uncomment to disable channel 2
  // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
  LMIC_setDrTxpow(DR_SF7, 14);
  // Start job
  do_send(&sendjob);
}

void loop() {
  os_runloop_once();
}
