/***********************************************************************/
/*                                                                     */
/*  FILE        :connect_wifi.ino                                                */
/*  DATE        :2019/10/16                                              */
/*  DESCRIPTION :Connect WiFi sample PROGRAM                                  */
/*  CPU TYPE    :STM32F405                                              */
/*  AUTHOR      :Ichiro Shirasaka                                      */
/*  NOTE:                                    */
/*                                                                     */
/***********************************************************************/

void getstr(char *buf);

void setup() {
  delay(1000);
}

void loop() {
/* WiFi 設定 */
  char linebuf[50];
  
  pinMode(PB1, OUTPUT);   //RESET信号を出力設定
  digitalWrite(PB1, LOW); //RESET信号をLow
  delay(100);
  digitalWrite(PB1, HIGH);//RESET信号をHigh
  for(int n = 0; n < 5; n++){ //初期化処理の終了待ち
     Serial.print("*");
     delay(1000);
  }
  Serial.println();
  Serial2.begin(115200);
  Serial.print("SSID?:"); 
  getstr(linebuf);        //SSID入力
  String ssid = linebuf;
  Serial.print("PASS?:");
  getstr(linebuf);        //パスワード入力
  String passwd = linebuf; 
  Serial.println("Connecting!!");
  /* AT命令　アクセスポイントへの接続 */
  Serial.println(atout("AT+CWJAP_DEF=\""+ssid+"\",\""+passwd+"\""));
  Serial.print("Push Enter key:");
  getstr(linebuf);
}

void getstr(char *buf){
  int n;  
  for(n = 0;; n++){
    while(!Serial.available());
    buf[n] = Serial.read();
    Serial.write(buf[n]);
    if(buf[n] == 0x08) n -= 2;
    if(buf[n] == '\r') break;
  }
  buf[n] = '\0';
  Serial.read();  
}

/* AT命令出力 */
String atout(String data) {
  String resp = "";
  String res;

  SerialUART2.println(data);  //UART2からAT命令出力
  while (1) {
    while (!SerialUART2.available()); //レスポンス待ち
    res = SerialUART2.readStringUntil(10);
    resp += res;
    if(res.indexOf("OK") != -1) return resp;  //OKレスポンス
    if(res.indexOf("ERROR") != -1) return ""; //ERRORレスポンス
  }  
}


