#include "SamplerNow.h"


int getBestChannel() {
  int numNetworks = WiFi.scanNetworks();
  int chanCount[14] = {0}; // Network counter for each channel
  
  for (int i = 0; i < numNetworks; i++) {
    int ch = WiFi.channel(i);
    if (ch >= 1 && ch <= 13) chanCount[ch]++;
  }
  
  // Compare the three main non-overlapping channels
  int bestChan = 1;
  int minCount = chanCount[1];
  
  if (chanCount[6] < minCount) { minCount = chanCount[6]; bestChan = 6; }
  if (chanCount[11] < minCount) { minCount = chanCount[11]; bestChan = 11; }
  
  Serial.printf("\n📊 Networks detected -> Ch1: %d, Ch6: %d, Ch11: %d\n", chanCount[1], chanCount[6], chanCount[11]);
  return bestChan;
}

void setWifiChannel(uint8_t channel) {
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
}

//Callback send
void onDataSent(const esp_now_send_info_t *tx_info, esp_now_send_status_t status) {  
}

//Callback reception
void onReceive(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
}

void sNowFsaRx_Setup(bool log){
	WiFi.mode(WIFI_STA);							if(log)	Serial.print(F("Set as Station\n"));
	int cleanChannel = getBestChannel();			if(log)	Serial.printf("🚀 Best Channel found: %d\n", cleanChannel);
	esp_wifi_set_mac(WIFI_IF_STA, CollectorMAC);	if(log)	Serial.printf(F("Set spoofing\n"));
	WiFi.disconnect();
	setWifiChannel(cleanChannel);					if(log)	Serial.printf(F("Set Best Channel\n"));
	
	if (esp_now_init() != ESP_OK) {
		Serial.println(F("❌ Critical error ESP-NOW"));
		ESP.restart();
		return;
	}
	esp_now_register_recv_cb(onReceive);
	esp_now_register_send_cb(onDataSent);

}