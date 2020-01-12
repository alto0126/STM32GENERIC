/**********************************************************************/
/*                                                                    */
/*  FILE        :radioclient.c                                        */
/*  DATE        :2020/1/1                                             */
/*  DESCRIPTION :First Bee HTTP Radio client                          */
/*  CPU TYPE    :ESP-WROOM-02                                         */
/*  AUTHOR      :Ichiro Shirasaka                                     */
/*  NOTE:                                                             */
/*                                                                    */
/**********************************************************************/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <string>

#define TCP_MSS 1460        //MSS size 1460B
#define BLEN (1024*32)      //Data buffer size 32KB

ESP8266WiFiMulti WiFiMulti;
uint8_t buff[BLEN] = { 0 }; //Data buffer
int wp = 0, rp = 0;         //Ring buffer pointer
//unsigned int c; 

/* アクセスポイントへの接続 */
void ap_connect(String ssid, String pwd){
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pwd.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(""); 
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

/* HTTP Client処理 */
void client(String url, String wnd){    // url...radio server URL　  wnd...window size
  Serial.println(url);
  TCP_WND = atoi(wnd.c_str()) * TCP_MSS;
  while(1){   
   if ((WiFiMulti.run() == WL_CONNECTED)) {
      WiFiClient client;
      HTTPClient http; //must be declared after WiFiClient for correct destruction       
      Serial.print("[HTTP] begin...\n");
        
      // configure server and url
      http.begin(client, url.c_str());   
     
      Serial.print("[HTTP] GET...\n"); 
      // start connection and send HTTP header
      int httpCode = http.GET();
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);
  
        // file found at server
        if (httpCode == HTTP_CODE_OK) {
          // get tcp stream
          WiFiClient * stream = http.getStreamPtr();
          // read data from server
          wp = rp = 0;
          stream->setTimeout(500); //Timeout 500mS
          while (http.connected()){
            if(stream->available() >= 512){   //512Bデータを受信したら読み出してリングバッファに入れる
              if(BLEN - (wp - rp) > 512){
                if(stream->readBytes((uint8_t*)(buff + (wp % BLEN)), 512) != 512) break;
                wp += 512;
              }
            }
            if(Serial.availableForWrite() >= 128){  //UARTのFIFOが128B空いたら書き込み
              if(wp - rp > 128){
                // write it to Serial
                noInterrupts();      //割込み禁止        
                Serial.write((uint8_t*)(buff + (rp % BLEN)), 128);
                rp += 128;
                interrupts();        //割込み許可
              }
            }
            if(Serial.available()){
              if(Serial.read() == 'e'){//終了指示をチェック
                http.end();
                return;
              }
            }
            delay(1);                //WDT対策
          }
         }
        Serial.println();
        Serial.print("[HTTP] connection closed or file end.\n");
      }
      http.end();
    }
  }
}

/* split Function */
int split(String data, char delimiter, String *dst){
    int index = 0;
    int arraySize = (sizeof(data)/sizeof((data)[0]));  
    int datalength = data.length();
    for (int i = 0; i < datalength; i++) {
        char tmp = data.charAt(i);
        if ( tmp == delimiter ) {
            index++;
            if ( index > (arraySize - 1)) return -1;
        }
        else dst[index] += tmp;
    }
    return (index + 1);
}

void setup() {
  String script;
  Serial.println();
  Serial.begin(921600);
  while(Serial.available()){
     Serial.read();
  }
  pinMode(13, FUNCTION_4); //CTS ON
  U0C0 |= 1<<UCTXHFE; //add this sentense to add a tx flow control via MTCK( CTS )
  Serial.setTimeout(1000);
}

void loop() {
  String script[3] = {"\0"};
  
  Serial.println(">");
  while(!Serial.available());
  String command = Serial.readStringUntil(10);
  Serial.println(command);
  int index = split(command, ',', script);
  switch(script[0].charAt(0)){
    case 'a':               //a command... wifi connect
      ap_connect(script[1], script[2]);
      break;
    case 'c':               //c command... send http request
      client(script[1], script[2]);
  }
} 
