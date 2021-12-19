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

//additional serial output
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
Command cmdAdd ,cmdScan ,cmdStation;
Command cmdFilter ,cmdTime ,cmdScanTime;

int32_t seconds;
int32_t last_seen;

//-> Enter MAC here or pass it via Serial-Command "filter"
const char searchedDevice[18] = "09:0b:a2:33:44:AA";  
unsigned char device[6];
struct tm *timeinfo;
static rtw_result_t scan_result_handler(rtw_scan_handler_result_t* malloced_scan_result);
static unsigned int statCount = 0;
static unsigned int sigCount = 0;
static unsigned int currentCh = 0;
static unsigned int scanTimePerChannel = 1000;
static bool isRepeatActive = false;
static bool verboseScan = false;
static bool verboseStat = false;
static int chFrom = 0;
static int chTo = 0;

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

    Serial.begin(115200);
    cmdScan = sCLI.addCmd("scan", scan_callback);
    cmdScan.addPositionalArgument("ch","0");
    cmdScan.addPositionalArgument("to","0");
    cmdScan.addFlagArgument("v"); //verbose
    cmdScan.addFlagArgument("r"); //repeat
    cmdTime = sCLI.addCmd("time", time_callback);
    cmdTime.addPositionalArgument("h","12");
    cmdTime.addPositionalArgument("m","0");
    cmdTime.addPositionalArgument("s","0");
    cmdStation = sCLI.addCmd("station", station_callback);
    cmdStation.addFlagArgument("v"); //verbose
    cmdFilter = sCLI.addCmd("filter", filter_callback);
    cmdFilter.addPositionalArgument("mac");
    cmdScanTime = sCLI.addCmd("scanTime", scanTime_callback);
    cmdScanTime.addPositionalArgument("t","1000");

    //pass MAC-Address to filter
    sscanf( searchedDevice, "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx", 
          &device[0], &device[1], &device[2], &device[3], &device[4], &device[5]);
          
    debug("Firmware V");
    String fv = WiFi.firmwareVersion();  
    debugln(fv);
    
    station_callback(NULL);
        
    Serial.println("Commands: station ,scan [from] [to], filter [mac], time [h] [m] [s]");
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

//sniff a single wifi-channel for MACs
void listen2Channel(byte chan){
    currentCh = chan;
    digitalWrite(LED_R, LOW);
    if(wifi_set_channel(chan) == RTW_SUCCESS){
       delay(scanTimePerChannel); 
    } else {
       debug("listening failed for Ch ");
       debugln(chan);
    }
    checkSignals();
    _signals.clear();
}

//scan given range of channels
void scanChannelRange(byte from, byte to){
   digitalWrite(LED_BUILTIN, LOW);
   for(byte ch = from; ch < to+1; ch ++){
     //Channel 1..13    for 2.4 GHz (802.11b/g/n/ax)
     //Channel 32..173  for 5 GHz (802.11a/h/j/n/ac/ax)
     if(ch > 14 && (ch % 2)){
       debug("Skip ch ");
       debugln(ch);
     } else {
       listen2Channel(ch);
     }
   }
}

//scan channels used by stations
void scanUsedChannels(void){
  digitalWrite(LED_BUILTIN, LOW);
  std::list<WiFiSignal>::iterator next = _stations.begin();
  while(next != _stations.end())
  {
    listen2Channel(next->channel);
    next++; 
  }
}

void scanMACs(void){
  if(chFrom == 0){
    //no channel parameter was set
    scanUsedChannels();     
  } else {
    scanChannelRange(chFrom, chTo);
  }         
  printTimeString();
  Serial.print(sigCount);
  Serial.println(" MACs found");
  sigCount=0;  
}

//list available stations
void station_callback(cmd* c){
    Argument verbo = cmdStation.getArgument("v");
    verboseStat=(verbo.isSet());
    wifi_set_promisc(RTW_PROMISC_DISABLE, NULL, 0);
    wifi_on(RTW_MODE_STA);
    _stations.clear();
    statCount = 0;
    //WiFi.scanNetworks() doesn´t return channel-IDs with Version 3.1.0(!) 
    if(wifi_scan_networks(scan_result_handler, NULL) != RTW_SUCCESS){
      debugln("wifi_scan_networks() failed");
      return;
    }
    // Wait for scan finished.
    vTaskDelay(5100);
    Serial.print(statCount);
    Serial.print(" Stations found, "); 
    Serial.print(_stations.size());
    Serial.println(" Channels used.");
    
    if(wifi_set_promisc(RTW_PROMISC_ENABLE_2, promisc_callback, 0) != RTW_SUCCESS){
      debugln("Init promisc failed!");
    }
}

void scan_callback(cmd* c) {
    Command cmd(c); // Create wrapper object
    Argument repeat = cmd.getArgument("r");
    Argument verbo = cmd.getArgument("v");
    Argument arg = cmd.getArgument("ch");
    String argVal = arg.getValue();
    chFrom = argVal.toInt();
    arg = cmd.getArgument("to");
    argVal = arg.getValue();
    chTo = argVal.toInt();
    chTo = std::max(chFrom, chTo); 
    isRepeatActive=(repeat.isSet());
    verboseScan=(verbo.isSet());      
    if(chFrom != 0){
      debug(chFrom);
      debug("-");
      debugln(chTo);
    }
    scanMACs();    
}

//set new MAC-Address
void filter_callback(cmd* c) {
    Command cmd(c); // Create wrapper object
    Argument arg = cmd.getArgument("mac");
    if(arg.isSet()){
      String mac = arg.getValue();
      char buffa[20];
      mac.toCharArray(buffa, 19);
      sscanf( buffa, "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx", 
          &device[0], &device[1], &device[2], &device[3], &device[4], &device[5]);
      Serial.print("Filter MAC: ");
      printMac(device);
      Serial.println();
    } else {
      Serial.println("Usage: filter 00:AA:BB:CC:DD:EE");
    }
}

//set scan time per Channel parameter
void scanTime_callback(cmd* c) {
    Command cmd(c); // Create wrapper object
    Argument arg = cmd.getArgument("t");
    if(arg.isSet()){
      Serial.print("scanTimePerChannel changed ");
      Serial.print(scanTimePerChannel);
      Serial.print(" -> ");
      String argVal = arg.getValue();
      scanTimePerChannel = argVal.toInt();
      Serial.print(scanTimePerChannel);
    }
}

//set current time to RTC
void time_callback(cmd* c) {
    isRepeatActive = false;
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
    Serial.print("New time: ");
    printTimeString();
    Serial.println();
}

//+++ MAIN LOOP +++
void loop() {
    seconds = rtc.Read();
    if (Serial.available()) {
        // Read out string from the serial monitor
        String input = Serial.readStringUntil('\n');
        // Parse the user input into the CLI
        sCLI.parse(input);
    }
    if(isRepeatActive){
        scanMACs();
    } else {
      rtc.Wait(1);
    }
}

//make callback simple to prevent latency to wlan rx in promiscuous mode
static void promisc_callback(unsigned char *buf, unsigned int len, void* userdata)
{
    const ieee80211_frame_info_t *frameInfo = (ieee80211_frame_info_t *)userdata;
    if(frameInfo->rssi == 0)
        return;
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

//handle station search result
static rtw_result_t scan_result_handler(rtw_scan_handler_result_t* malloced_scan_result){
  if (malloced_scan_result->scan_complete != RTW_TRUE) {
    rtw_scan_result_t* record = &malloced_scan_result->ap_details;
    record->SSID.val[record->SSID.len] = 0; /* Ensure the SSID is null terminated */

    statCount++;
    if(verboseStat){
      Serial.print(statCount);
      Serial.print("|");
      String tempstr = reinterpret_cast<const char*>(record->SSID.val);
      Serial.print(tempstr);
      Serial.print("|");
      Serial.print(record->channel);
      Serial.print("|");
      Serial.print(record->signal_strength);
      Serial.print("|");
      Serial.print(record->wps_type);
      Serial.println(( record->bss_type == RTW_BSS_TYPE_ADHOC ) ? "|Adhoc" : "|Infra" );
    }
    bool found = false;
    std::list<WiFiSignal>::iterator next = _stations.begin();
    while(next != _stations.end())
    {
        if(next->channel == record->channel){
          found = true;
          break; //channel already listed 
        }
        next++;
    }
    if(found == false){
        WiFiSignal wifisignal;
        wifisignal.rssi = record->signal_strength;
        wifisignal.channel = record->channel;
        memcpy(&wifisignal.addr, record->BSSID.octet, 6);
        _stations.push_back(wifisignal); 
    }
  }
  return RTW_SUCCESS;
}

//search the needle in the haystack
void checkSignals() {
    std::list<WiFiSignal>::iterator next = _signals.begin();
    while(next != _signals.end())
    {
        if(verboseScan){  //print out all MACs
          printMac(next->addr);
          Serial.println("");
        }
        if(device[0]==next->addr[0] && device[1]==next->addr[1] && 
           device[2]==next->addr[2] && device[3]==next->addr[3] && 
           device[4]==next->addr[4] && device[5]==next->addr[5]){
          last_seen = seconds;
          digitalWrite(LED_BUILTIN, HIGH);
          Serial.print(" DEVICE DETECTED! Ch ");
          Serial.print(currentCh);
          Serial.print(" RSSI ");
          Serial.println(next->rssi);
        }
        next++;
    }
    sigCount += _signals.size();
}

//print helper functions
void printTimeString(void) {
    timeinfo = localtime(&seconds);
    char buffer[10];
    sprintf(buffer,"%02d:%02d:%02d ",
            timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    Serial.print(buffer);
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
