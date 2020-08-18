// Arduino Web Controller, written by Kerry Wang, Summer 2020
// Web server based on eth_websrv_SD by W. A. Smith, found at https://startingelectronics.org/tutorials/arduino/ethernet-shield-web-server-tutorial/SD-card-web-server/

#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <LiquidCrystal.h>

// Variables for ethernet
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 177); // IP address, may need to change depending on network, TODO: add DHCP
EthernetServer server(80);  // create a server at port 80

// Variables for SD
File webFile;
String HttpRequest, PostRequest;
byte buffer[512]; // buffer to speed up SD card read

// Variables for LCD
const int rs = 8, en = 9, d4 = 3, d5 = 5, d6 = 6, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Other variables
int front, back;

void setup()
{
  Ethernet.begin(mac, ip);  // initialize Ethernet device
  server.begin();           // start to listen for clients
  Serial.begin(9600);       // for debugging

  // initialize SD card
  Serial.println("Initializing SD card...");
  if (!SD.begin(4)) {
    Serial.println("ERROR - SD card initialization failed!");
    return;    // init failed
  }
  Serial.println("SUCCESS - SD card initialized.");
  // check for index.htm file
  if (!SD.exists("index.htm")) {
    Serial.println("ERROR - Can't find index.htm file!");
    return;  // can't find index file
  }
  Serial.println("SUCCESS - Found index.htm file.");

  //initialize LCD
  lcd.begin(16, 2);
  lcd.print(Ethernet.localIP());

  // initialize SPI
  pinMode(SS2, OUTPUT);
  digitalWrite(SS2, HIGH);
  SPI.begin();
  spi_transact(0);
}

long spi_transact(long byteTx) { // sends and receives a long via SPI
  long byteRx;
  uint16_t upper = (byteTx & 4294901760) >> 16; // clear out lower 16 bits and shift right to get the upper 2 bytes
  uint16_t lower = byteTx & 65535; // clear out upper 16 bits
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
  digitalWrite(SS2, LOW);
  byteRx = SPI.transfer16(upper); // send upper 2 bytes
  byteRx = byteRx << 16; // shift result left 16 bits to make room
  byteRx = byteRx | SPI.transfer16(lower); // send lower 2 bytes, fill in lower 16 bits of response
  digitalWrite(SS2, HIGH);
  SPI.endTransaction();
  return byteRx;
}

void sendPage(EthernetClient client) { // send the webpage
  // send a standard http response header
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Connection: close");
  client.println();
  // send web page
  webFile = SD.open("index.htm");        // open web page file
  if (webFile) {
    while (size_t size = webFile.available()) { // read and send file in chunks of 512 bytes
      if (size > 512) {
        size = 512;
      }
      webFile.read(buffer, size);
      client.write(buffer, size); // send web page to client
    }
    webFile.close();
  }
}

void loop()
{ // handle ethernet connections, if any
  EthernetClient client = server.available();  // try to get client

  if (client) {  // got client?
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {   // client data available to read
        char c = client.read(); // read 1 byte (character) from client
        HttpRequest += c;
        // last line of client request is blank and ends with \n
        // respond to client only after last line received
        if (c == '\n' && currentLineIsBlank) {
          Serial.print(HttpRequest);
          HttpRequest.remove(4); // get first line of header

          Serial.println("Request is " + HttpRequest);
          if (HttpRequest == "POST") { // handle data from post
            Serial.println("Got POST");
            HttpRequest = ""; // clear string
            bool err = false;
            while (client.available()) { // read rest of message
              char c = client.read();
              if (isDigit(c) || c == ',') { // check if in proper format
                HttpRequest += c;
              } else {
                err = true;
                break;
              }
            }
            if (HttpRequest.indexOf(",,") != -1) {
              err = true;
            }
            if (err) { // finished server-side validation, send proper response
              client.println("HTTP/1.1 400 Bad Request");
              client.println("Access-Control-Allow-Origin: *");
              client.println();
              Serial.println("Bad Request");
            } else {
              PostRequest = HttpRequest;
              Serial.println(PostRequest);
              front = 0;
              back = 0;
              lcd.clear();
              lcd.print("Table loaded");
              client.println("HTTP/1.1 202 Accepted");
              client.println("Access-Control-Allow-Origin: *");
              client.println();
              client.print(PostRequest); // read the request back to the client
            }
          } else {
            sendPage(client); // if not post, send webpage
          }
          break;
        }
        // every line of text received from the client ends with \r\n
        if (c == '\n') {
          // last character on line of received text
          // starting new line with next character read
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          // a text character was received from client
          currentLineIsBlank = false;
        }
      } // end if (client.available())
    } // end while (client.connected())
    delay(1);      // give the web browser time to receive the data
    client.stop(); // close the connection
    HttpRequest = "";
  } // end if (client)

  // handle SPI output
  if (Serial.available()) {
    if (Serial.read() == '\n') {
      lcd.clear();
      if (front != -1) {
        back = PostRequest.indexOf(',', front);
        long toPrint;
        if (back != -1) {
          toPrint = PostRequest.substring(front, back).toInt();
          spi_transact(toPrint);
          Serial.println(toPrint);
          lcd.print(toPrint);
          front = back + 1;
        } else {
          toPrint = PostRequest.substring(front).toInt();
          spi_transact(toPrint);
          Serial.println(toPrint);
          lcd.print(toPrint);
          front = -1;
        }
        lcd.print(" Hz");
      } else {
        Serial.println("out of entries");
        lcd.print("Out of entries");
      }
    }
  }
}
