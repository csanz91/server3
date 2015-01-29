#include <Arduino.h>
/*--------------------------------------------------------------
  Program:      eth_websrv_SD_Ajax_in_out

  Description:  Arduino web server that displays 4 analog inputs,
                the state of 3 switches and controls 4 outputs,
                2 using checkboxes and 2 using buttons.
                The web page is stored on the micro SD card.
  
  Hardware:     Arduino Uno and official Arduino Ethernet
                shield. Should work with other Arduinos and
                compatible Ethernet shields.
                2Gb micro SD card formatted FAT16.
                A2 to A4 analog inputs, pins 2, 3 and 5 for
                the switches, pins 6 to 9 as outputs (LEDs).
                
  Software:     Developed using Arduino 1.0.5 software
                Should be compatible with Arduino 1.0 +
                SD card contains web page called index.htm
  
  References:   - WebServer example by David A. Mellis and 
                  modified by Tom Igoe
                - SD card examples by David A. Mellis and
                  Tom Igoe
                - Ethernet library documentation:
                  http://arduino.cc/en/Reference/Ethernet
                - SD Card library documentation:
                  http://arduino.cc/en/Reference/SD

  Date:         4 April 2013
  Modified:     19 June 2013
                - removed use of the String class
 
  Author:       W.A. Smith, http://startingelectronics.com
--------------------------------------------------------------*/

#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <Wire.h>

// size of buffer used to capture HTTP requests
#define REQ_BUF_SZ   60

//Direcciones de los dispositivos conectados al bus I2C
#define ADDR_I2C_SERVER 1
#define ADDR_LCD    2
#define ADDR_SERVER 3
#define NUM_DATA    5                       //Num de datos que almacenara el array

//Definimos la posicion de cada dato en el array
#define data1   0
#define data2   1

float data[NUM_DATA];                           //Array de datos

// MAC address from Ethernet shield sticker under board
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 46); // IP address, may need to change depending on network
EthernetServer server(80);  // create a server at port 80
File webFile;               // the web page file on the SD card
char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
char req_index = 0;              // index into HTTP_req buffer
boolean LED_state[4] = {0}; // stores the states of the LEDs


void SetLEDs(void);
void XML_response(EthernetClient cl);
void StrClear(char *str, char length);
char StrContains(char *str, char *sfind);

void setup()
{

    Wire.begin(ADDR_SERVER);                   //Nos unimos al bus I2C con la direccion dada
    // disable Ethernet chip
    pinMode(10, OUTPUT);
    digitalWrite(10, HIGH);
    
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
    if (!SD.exists("back.jpg")) {
        Serial.println("ERROR - Can't find background file!");
        return;  // can't find index file
    }
    Serial.println("SUCCESS - Found background file.");
    
    Ethernet.begin(mac, ip);  // initialize Ethernet device
    server.begin();           // start to listen for clients
}

void loop()
{
    EthernetClient client = server.available();  // try to get client

    if (client) {  // got client?
        boolean currentLineIsBlank = true;
        while (client.connected()) {
            if (client.available()) {   // client data available to read
                char c = client.read(); // read 1 byte (character) from client
                // limit the size of the stored received HTTP request
                // buffer first part of HTTP request in HTTP_req array (string)
                // leave last element in array as 0 to null terminate string (REQ_BUF_SZ - 1)
                if (req_index < (REQ_BUF_SZ - 1)) {
                    HTTP_req[req_index] = c;          // save HTTP request character
                    req_index++;
                }
                // last line of client request is blank and ends with \n
                // respond to client only after last line received
                if (c == '\n' && currentLineIsBlank) {
                    // send a standard http response header
                    client.println("HTTP/1.1 200 OK");
                    // remainder of header follows below, depending on if
                    // web page or XML page is requested
                    // Ajax request - send XML file
                    if (StrContains(HTTP_req, "ajax_inputs")) {
                        // send rest of HTTP header
                        client.println("Content-Type: text/xml");
                        client.println("Connection: keep-alive");
                        client.println();
                        //SetLEDs();
                        // send XML file containing input states
                        XML_response(client);
                    }else if(StrContains(HTTP_req, "back")){
                        client.println("Content-Type: image/jpeg");
                        client.println();
                        webFile = SD.open("back.jpg");
                    	//sendFile(webFile, client);
                        
                    }
                    else {  // web page request
                        // send rest of HTTP header
                        client.println("Content-Type: text/html");
                        client.println("Connection: keep-alive");
                        client.println();
                        // send web page
                        webFile = SD.open("index.htm");        // open web page file
                        sendFile(webFile, client);
                    }
                    // display received HTTP request on serial port
                    Serial.print(HTTP_req);
                    // reset buffer index and all buffer elements to 0
                    req_index = 0;
                    StrClear(HTTP_req, REQ_BUF_SZ);
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
    } // end if (client)
}

void sendFile(File webFile, EthernetClient client){
	if (webFile) {

		byte clientBuf[64];
		int clientCount = 0;              

		while (webFile.available()) 
		{
		    clientBuf[clientCount] = webFile.read();
		    clientCount++;

		    if(clientCount > 63)
		    {
		        client.write(clientBuf,64);
		        clientCount = 0;
		    }                
		}
		if(clientCount > 0) client.write(clientBuf,clientCount);            
		    webFile.close();
		}
	else{

        Serial.println("Error al cargar el background");
    }
	
}


// send the XML file with analog values, switch status
//  and LED status
void XML_response(EthernetClient cl)
{

    cl.print("<?xml version = \"1.0\" ?>");
    cl.print("<inputs>");
    // read analog inputs
    for (int i = 0; i < NUM_DATA; i++) {
        cl.print("<data>");
        cl.print(12345.67);
        cl.println("</data>");
    }
    
    cl.print("</inputs>");
}

// sets every element of str to 0 (clears array)
void StrClear(char *str, char length)
{
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
}

// searches for the string sfind in the string str
// returns 1 if string found
// returns 0 if string not found
char StrContains(char *str, char *sfind)
{
    char found = 0;
    char index = 0;
    char len;

    len = strlen(str);
    
    if (strlen(sfind) > len) {
        return 0;
    }
    while (index < len) {
        if (str[index] == sfind[found]) {
            found++;
            if (strlen(sfind) == found) {
                return 1;
            }
        }
        else {
            found = 0;
        }
        index++;
    }

    return 0;
}

//Llama al I2C server para que le devuelva el array de datos
void askForData(){

    byte data_received[4];

    //Hace una llamada al slave por cada dato que se quiere obtener
    for(int x=0; x<NUM_DATA; x++){
        Wire.requestFrom(ADDR_I2C_SERVER, 4);                       //Pide 4 bytes a la direccion del servidor I2C

        //Lee los cuatro bytes de los que se forma el float y los vuelve a unir
        int i = 0;
        while(Wire.available())    // slave may send less than requested
        { 
            data_received[i] = Wire.read(); // receive a byte as character  
            i++;
        }
        //A union datatypes makes the byte and float elements share the same piece of memory, which enables conversion from a byte array to a float possible
        union float_tag {byte data_b[4]; float data_fval;} float_Union;    
        float_Union.data_b[0] = data_received[0];
        float_Union.data_b[1] = data_received[1];
        float_Union.data_b[2] = data_received[2];
        float_Union.data_b[3] = data_received[3];    
        data[x] = float_Union.data_fval;
    }
}