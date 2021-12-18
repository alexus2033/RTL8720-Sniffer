/*
 * RTL8720_Sniffer.ino
 *
 * Created: 18/12/2021 10:35:07 AM
 * Author: Alexus2033
 */ 

#include "rtc.h"
#include <WiFi.h>
#include <stdio.h>
#include <time.h>
#include <SimpleCLI.h>
#include <wifi_conf.h>
//undefine invalid defines from stl_algobase.h
#undef max
#undef min

#include <list>
#define CONFIG_PROMISC 1

//switch serial output on/off
#define DEBUG 1

#if DEBUG == 1
  #define debug(x) Serial.print(x)
  #define debugfm(x,y) Serial.print(x,y)
  #define debugln(x) Serial.println(x)
#else
  #define debug(x)
  #define debugln(x)
#endif

RTC rtc;
SimpleCLI sCLI;
Command cmdScan;
Command cmdStation;
Command cmdTime;
int32_t seconds;
struct tm *timeinfo;
static rtw_result_t scan_result_handler(rtw_scan_handler_result_t* malloced_scan_result);
static unsigned int currentCh = 0;

struct WiFiSignal {
    unsigned char addr[6];
    unsigned int channel; 
    signed char rssi;
};

std::list<WiFiSignal> _signals;
std::list<WiFiSignal> _stations;

void setup() {
    rtc.Init();
    pinMode(LED_BUILTIN, OUTPUT); //green LED
    pinMode(LED_R, OUTPUT);  //red LED
    pinMode(LED_B, OUTPUT);  //blue LED
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(LED_R, HIGH);
    digitalWrite(LED_B, LOW);

    Serial.begin(9600);
    cmdScan = sCLI.addCmd("scan");
    cmdScan.addPositionalArgument("ch","32");
    cmdScan.addPositionalArgument("to","0");
    cmdScan.addFlagArgument("R"); //repeat
    cmdTime = sCLI.addCmd("time", time_callback);
    cmdTime.addPositionalArgument("h","12");
    cmdTime.addPositionalArgument("m","0");
    cmdTime.addPositionalArgument("s","0");
    cmdStation = sCLI.addCmd("station", station_callback);
    
    debug("Firmware V");
    String fv = WiFi.firmwareVersion();  
    debug(fv);
    debugln(" detected");
    Serial.println("Commands: station ,scan [from] [to], time [h] [m] [s]");
    delayMicroseconds(50);
    WiFi.disablePowerSave();
    wifi_on(RTW_MODE_PROMISC);
  //wext_set_bw40_enable(true);
    wifi_enter_promisc_mode();
    
    // 1 = promisc for all ethernet frames.
    // 2 = promisc for Broadcast/Multicast ethernet frames.
    // 3 = promisc for all 802.11 frames.
    // 4 = promisc for Broadcast/Multicast 802.11 frames.
    int x = wifi_set_promisc(RTW_PROMISC_ENABLE_2, promisc_callback, 0);
    if(x != RTW_SUCCESS){
      debugln("Init promisc failed!");
    }
}

//scan defined wifi-channel for MACs
void scanChannel(u8 chan, u32 scanTimePerChannel){
    currentCh = chan;
    digitalWrite(LED_R, LOW);
    if(wifi_set_channel(chan) == RTW_SUCCESS){
       delay(scanTimePerChannel); 
    } else {
       debug("Scan failed for Ch ");
       debugln(chan);
    }
    printSignals();
}

//list available stations
void station_callback(cmd* c){
    wifi_set_promisc(RTW_PROMISC_DISABLE, NULL, 0);
    wifi_on(RTW_MODE_STA);
    _stations.clear();
    digitalWrite(LED_BUILTIN, LOW);
    //WiFi.scanNetworks() doesn´t return channel-IDs at the moment(!) 
    if(wifi_scan_networks(scan_result_handler, NULL) != RTW_SUCCESS){
      debugln("wifi_scan_networks() failed");
      return;
    }
    // Wait for scan finished.
    vTaskDelay(6000);
    printStations();
    if(wifi_set_promisc(RTW_PROMISC_ENABLE_2, promisc_callback, 0) != RTW_SUCCESS){
      debugln("Init promisc failed!");
    }
}

void scanChannels(u8 *channels, u8 numberOfChannels, u32 scanTimePerChannel){
    _signals.clear();
    for(u8 ch = 0; ch < numberOfChannels; ch ++) {
        if(wifi_set_channel(channels[ch]) == RTW_SUCCESS){
          delay(scanTimePerChannel); 
        }
    }
    printSignals();
}

//set current time to RTC
void time_callback(cmd* c) {
    Command cmd(c); // Create wrapper object
    Argument arg = cmd.getArgument("h");
    String argVal = arg.getValue();
    int newHour = argVal.toInt();
   
    arg = cmd.getArgument("m");
    argVal = arg.getValue();
    int newMin = argVal.toInt();

    arg = cmd.getArgument("s");
    argVal = arg.getValue();
    int newSec = argVal.toInt();

    //actually, the year can´t be changed
    int epochTime = rtc.SetEpoch(2020, 1, 1, newHour, newMin, newSec);
    rtc.Write(epochTime);
    seconds = rtc.Read();
    printTimeString();
}

void loop() {
    if (Serial.available()) {
        // Read out string from the serial monitor
        String input = Serial.readStringUntil('\n');
        // Parse the user input into the CLI
        sCLI.parse(input);
    }
    if(sCLI.available()){
      Command cmd = sCLI.getCommand();
      if(cmd == cmdScan){
        Argument arg = cmd.getArgument("ch");
        String argVal = arg.getValue();
        int from = argVal.toInt();
        Serial.print(from);
        arg = cmd.getArgument("to");
        argVal = arg.getValue();
        int to = argVal.toInt();
        to = std::max(from, to);
        Serial.print("-");
        Serial.println(to);
        for(byte ch = from; ch < to+1; ch ++){
          //Channel 1..13    for 2.4 GHz (802.11b/g/n/ax)
          //Channel 32..173  for 5 GHz (802.11a/h/j/n/ac/ax)
          if(ch > 14 && (ch % 2)){
            debug("Skip ch ");
            debugln(ch);
          } else {
            scanChannel(ch,1000);
          }
        }
        _signals.clear();
        printTimeString();         
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
    seconds = rtc.Read();
    rtc.Wait(1);
}

/*  Make callback simple to prevent latency to wlan rx when promiscuous mode */
static void promisc_callback(unsigned char *buf, unsigned int len, void* userdata)
{
    const ieee80211_frame_info_t *frameInfo = (ieee80211_frame_info_t *)userdata;
//    if(frameInfo->i_addr1[0] == 0xff && frameInfo->i_addr1[1] == 0xff && frameInfo->i_addr1[2] == 0xff && frameInfo->i_addr1[3] == 0xff && frameInfo->i_addr1[4] == 0xff && frameInfo->i_addr1[5] == 0xff)
//        return;
    if(frameInfo->rssi == 0)
        return;
    digitalWrite(LED_BUILTIN, HIGH);
    WiFiSignal wifisignal;
    wifisignal.rssi = frameInfo->rssi;
    wifisignal.channel = currentCh;
    memcpy(&wifisignal.addr, &frameInfo->i_addr2, 6);

    std::list<WiFiSignal>::iterator next = _signals.begin();
    while(next != _signals.end())
    {
        if(next->addr[0] == wifisignal.addr[0] &&
          next->addr[1] == wifisignal.addr[1] &&
          next->addr[2] == wifisignal.addr[2] &&
          next->addr[3] == wifisignal.addr[3] &&
          next->addr[4] == wifisignal.addr[4] &&
          next->addr[5] == wifisignal.addr[5]
          )
            _signals.erase(next);
        next++;
    }
    _signals.push_back(wifisignal);
}

static rtw_result_t scan_result_handler(rtw_scan_handler_result_t* malloced_scan_result){
  static int ApNum = 0;
  if (malloced_scan_result->scan_complete != RTW_TRUE) {
    rtw_scan_result_t* record = &malloced_scan_result->ap_details;
    record->SSID.val[record->SSID.len] = 0; /* Ensure the SSID is null terminated */

    debug(++ApNum);
    debug("|");
    String tempstr = reinterpret_cast<const char*>(record->SSID.val);
    debug(tempstr);
    debug("|");
    debug(record->channel);
    debug("|");
    debug(record->signal_strength);
    debug("|");
    debug(record->wps_type);
    debugln(( record->bss_type == RTW_BSS_TYPE_ADHOC ) ? "|Adhoc" : "|Infra" );
    WiFiSignal wifisignal;
    wifisignal.rssi = record->signal_strength;
    wifisignal.channel = record->channel;
    memcpy(&wifisignal.addr, record->BSSID.octet, 6);
    _stations.push_back(wifisignal);
  }
  return RTW_SUCCESS;
}

//print helper functions
void printSignals() {
    delayMicroseconds(50);
    std::list<WiFiSignal>::iterator next = _signals.begin();
    while(next != _signals.end())
    {
        printMac(next->addr);
        Serial.println(next->rssi);
        next++;
    }
    printf("\r\n");
}


void printStations() {
    delayMicroseconds(50);
    std::list<WiFiSignal>::iterator next = _stations.begin();
    while(next != _stations.end())
    {
        printMac(next->addr);
        Serial.print(" ");
        Serial.print(next->channel);
        Serial.println(next->rssi);
        next++;
    }
}

void printTimeString(void) {
    timeinfo = localtime(&seconds);
    Serial.print(timeinfo->tm_hour);
    Serial.print(":");
    Serial.print(timeinfo->tm_min);
    Serial.print(":");
    Serial.println(timeinfo->tm_sec);
}

void printMac(const unsigned char mac[6]) {
    for(u8 x = 0; x < 6; x ++){
        Serial.print(mac[x], HEX);
        //printf(" %02x", mac[x]);
        if(x < 5){
           Serial.print("-");  
        }
    }
}
