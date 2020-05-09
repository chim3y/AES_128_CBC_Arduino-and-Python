//Reference: https://platformio.org/lib/show/5533/AESLib
//https://python-forum.io/Thread-AES-encryption-does-not-match-between-arduino-and-python-3-crypto
//https://github.com/suculent/thinx-aes-lib/issues/24

#include <WiFi.h>
#include <WiFiUdp.h>
#include "AESLib.h"
#include <base64.h>

extern "C"{
#include "crypto/base64.h"  
}
AESLib aesLib;
#define UDP_TX_PACKET_MAX_SIZE 8192
/* WiFi network name and password */
const char * ssid = "iPhone";
const char * pwd = "happyme123";
const char * udpAddress = "172.20.10.2"; //Assign here the ip address of your computer on which python is run
const int udpPort = 44444;

//create UDP instance
WiFiUDP udp;

char packetBuffer[UDP_TX_PACKET_MAX_SIZE];
char packetBuffer1[UDP_TX_PACKET_MAX_SIZE];
String plaintext = "HELLO WORLD!";
char cleartext[256];
char ciphertext[512];
char ciphertext1[512];

#define BLOCK_SIZE 16

//key and iv is in hexadecimal format
uint8_t aes_key[BLOCK_SIZE] = { 0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31, 0x31, 0x31 };
//uint8_t aes_iv[BLOCK_SIZE] = { 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30 };
uint8_t enc_iv[BLOCK_SIZE] = { 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30 };// iv_block gets written to, provide own fresh copy...

//Authentication list
#define ARRAYSIZE 4
String credentials[ARRAYSIZE] = { "Bob", "Password11!", "Alice", "Happyme11!"};

//Control led
int LED=2;
int LED2=4;

String encrypt(char * msg, uint16_t msgLen, byte iv[]) {
  int cipherlength = aesLib.get_cipher64_length(msgLen);
  char encrypted[cipherlength]; // AHA! needs to be large, 2x is not enough
  aesLib.encrypt64(msg, msgLen, encrypted, aes_key, sizeof(aes_key), iv);
  Serial.print("encrypted = "); Serial.println(encrypted);
  return String(encrypted);
}

String decrypt(char * msg, uint16_t msgLen, byte iv[]) {
  char decrypted[msgLen];
  aesLib.decrypt64(msg, msgLen, decrypted, aes_key, sizeof(aes_key), iv);
  return String(decrypted);
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void check_authentication(String user, String passw){
    int i;
    bool state=false;
    for(int i=0;i<sizeof(credentials);i=i+2){ 
      if(user==credentials[i] && passw==credentials[i+1]){
      state=true;
      break;
      }else{
      state=false;
      }
    }
    if(state){
      udp.beginPacket(udpAddress, udpPort);
      udp.print((char *)"CONNECTED");
      Serial.print("Arduino to python:");
      Serial.println("CONNECTED");
      udp.endPacket();
    }
    else{
      udp.beginPacket(udpAddress, udpPort);
      udp.print((char *)"CONNECTION NOT ALLOWED");
      Serial.print("Arduino to python :");
      Serial.println("CONNECTION NOT ALLOWED");
      udp.endPacket();  
    }
}

void blink_led(String user, int times){
  //Bob can control both led
  //Alice can control only one led
  if(user==credentials[0]){
      for(int i=0; i<times; i++){
      digitalWrite(LED, HIGH);   // turn the LED on (HIGH is the voltage level)
      digitalWrite(LED2, HIGH);   // turn the LED on (HIGH is the voltage level)
      delay(1000);                       // wait for a second
      digitalWrite(LED, LOW);    // turn the LED off by making the voltage LOW
      digitalWrite(LED2, LOW);   // turn the LED on (HIGH is the voltage level)
      delay(1000); 
      Serial.println("Python to Arduino: BLINK_LED_A AND BLINK_ON_LED_B");
      }
   }
   else if(user==credentials[2]){
      for(int i=0; i<times; i++){
      digitalWrite(LED, HIGH);   // turn the LED on (HIGH is the voltage level)
      delay(1000);                       // wait for a second
      digitalWrite(LED, LOW);    // turn the LED off by making the voltage LOW
      delay(1000); 
      Serial.println("Python to Arduino: BLINK_LED_A ONLY");
      }
   }
}

void setup() {
  Serial.begin(115200);
   //Connect to the WiFi network
  WiFi.begin(ssid, pwd);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  //This initializes udp and transfer buffer


  Serial.begin(9600);
  udp.begin(udpPort);
  //  aes_init();
  aesLib.set_paddingmode(paddingMode::Array);
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED, OUTPUT);
  pinMode(LED2, OUTPUT);
  
}

void loop() {
//1. Receive authentication from python
    aesLib.gen_iv(enc_iv);  
    int packetSize = udp.parsePacket();
    if (packetSize) {
    int len1 = udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);

    String authenticationrequest=(char*) packetBuffer;
    int wait_time=0;
    while(authenticationrequest=="Request for connection" && wait_time<50){
      //2. Send iv from server 
        Serial.println();
        Serial.print("Python to Arduino (Authentication Request):");
        Serial.println(authenticationrequest);
//        int i;
//        for (i = 0; i < sizeof(enc_iv); i++)
//        {
//             Serial.print(enc_iv[i]);
//             Serial.print(",");
//        }
//        Serial.println();
        udp.beginPacket(udpAddress, udpPort);
        int k= 0;
//Send iv to python
          char send_data[sizeof(enc_iv)];
          for (k = 0; k < sizeof(enc_iv); k++)
          {
             send_data[k]=enc_iv[k];
          }
          send_data[17]='\0';
          Serial.print("Arduino to Python (iv):");
          udp.print(send_data);
          Serial.println((char *)send_data);
          udp.endPacket();
//Receive username as plaintext and password as encrypted
    int packetSize1 = udp.parsePacket();
    if (packetSize1) {
    int len = udp.read(packetBuffer1, UDP_TX_PACKET_MAX_SIZE);
    if ((len > 0) &&  (packetBuffer1 !="Request for connection")){ 
    packetBuffer1[len] = 0;
    //message received using udp must be first checked with parse packet
//    Serial.print("Received(Python to Arduino):");
//    Serial.println((char *) packetBuffer1);
    Serial.print("Python to Arduino (Username):");
    String username = getValue(packetBuffer1,';',0);
    Serial.println(username);
    Serial.print("Python to Arduino (Password):");
    String encrypted = getValue(packetBuffer1,';',1);
//    int k;
//    for (k = 0; k < sizeof(encrypted); k++)
//    {
//       ciphertext[k]=encrypted[k];
//    }
//       ciphertext[sizeof(encrypted)-1]='\0';

//    String encrypted= (char *) packetBuffer1;
    Serial.println(encrypted);
    sprintf(ciphertext, "%s", encrypted.c_str());

//Decryption
    uint16_t dlen = encrypted.length();
    //uint8_t dec_iv[BLOCK_SIZE] = { 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30 };  // iv_block gets written to, provide own fresh copy...
    String decrypted = decrypt(ciphertext, dlen, enc_iv);
    Serial.print("Decrypted text:");
    Serial.println(decrypted.c_str());

    //check authentication
    check_authentication(username, decrypted);

    //blinking of led
    int blink_count=1;
    int i=0;
    while(i<10){
    packetSize = udp.parsePacket();
    Serial.println(packetSize);
    if(packetSize){
    int len1 = udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    if(len1>0){
        blink_led(username, blink_count);
        blink_count=1;
      }
    }
    i+=1;
    }
    authenticationrequest="completed";
    break;
  }
  wait_time++;
  if(wait_time > 50)
  {
    break;
  }
  }
  wait_time++;
  Serial.print("Waiting...: ");
  Serial.println(wait_time);
  }
  }
  delay(5000);
}
