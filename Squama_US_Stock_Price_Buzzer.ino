// ArduinoJson - hq.sinajs.cn
// MIT License

#include <Ethernet.h>
#include <SPI.h>

#include <Adafruit_NeoPixel_ZeroDMA.h>

#define PIN        12
#define NUM_PIXELS 1

Adafruit_NeoPixel_ZeroDMA strip(NUM_PIXELS, PIN, NEO_GRB);

void setRGB(int r, int g, int b)
{
    strip.setPixelColor(0, r, g, b);
    strip.show();
}

void setup() {
    // Initialize Serial port
   
    Serial.begin(9600);
    while (!Serial) continue;
    Serial.println("sys start...");

    // Initialize Ethernet library
    byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
    if (!Ethernet.begin(mac)) 
    {
        Serial.println(F("Failed to configure Ethernet"));
        return;
    }
    Serial.println(F("Ethernet ok"));
    pinMode(13, OUTPUT);
    pinMode(21, OUTPUT);
    
    strip.begin();
    strip.setBrightness(32);
    strip.show();
}

float makeNum(char *s, int sstart, int sstop)
{
    int nBit = 0;
    int fBit = 0;
    float fushu = 1.0;
    
    int __pow[7] = {1, 10, 100, 1000, 10000, 100000, 1000000}; 
    
    if(s[sstart] == '-')    // < 0
    {
        fushu = -1.0;
        sstart++;
    }

    for(int i=sstart; i<=sstop; i++)
    {
        if(s[i] != '.')nBit++;
        else break;
    }

    fBit = sstop - sstart - nBit;
 
    float ret = 0.0;

    for(int i=0; i<nBit; i++)
    {
        ret += (float)__pow[i] * (float)(s[sstart+nBit-i-1]-'0');     // 3,2
    }

    for(int i=0; i<fBit; i++)
    {
        ret += (float)(s[sstart+nBit+1+i]-'0')/(float)__pow[i+1];
    }

    return (ret*fushu);
    
}

void makeStr(char *s, char *r, int sstart, int sstop)
{
    int inx = 0;
    for(int i=sstart; i<=sstop; i++)
    {
        r[inx++] = s[i];
    }
    r[inx] = '\0';
}


int getUsStock(char *usCode, float *price, char *time, float *rise, float *rise2)      //
{
    
    EthernetClient client;
    client.setTimeout(10000);
    
    if (!client.connect("hq.sinajs.cn", 80)) {
        
        Serial.println(F("Connection failed"));
        return -1;
    }
   
    char strCode[50];
    
    sprintf(strCode, "GET /list=gb_%s HTTP/1.0", usCode);
    
    client.println(strCode);
    client.println(F("Host: hq.sinajs.cn"));
    client.println(F("Connection: close"));
    
    if (client.println() == 0) {
        
        Serial.println(F("Failed to send request"));
        return -3;
    }

    // Check HTTP status
    char status[32] = {0};
    client.readBytesUntil('\r', status, sizeof(status));

    if (strcmp(status, "HTTP/1.1 200 OK") != 0) {

        Serial.print(F("Unexpected response: "));
        Serial.println(status);
        return -4;
    }

    // Skip HTTP headers
    char endOfHeaders[] = "\r\n\r\n";
    if (!client.find(endOfHeaders)) {
        
        Serial.println(F("Invalid response"));
        return -5;
    }

    char str[1000];
    int len = 0;
    
    while(client.available())
    {
        str[len++] = client.read();
    }
    
    int locaMd[200];
    int locaMdLen = 0;
    
    for(int i=0; i<len; i++)
    {
        if(str[i] == '"' || str[i] == ',')
        {
            locaMd[locaMdLen++] = i;
        }
    }

    *price = makeNum(str, locaMd[1]+1, locaMd[2]-1);
    *rise  = makeNum(str, locaMd[2]+1, locaMd[3]-1);        
    *rise2 = makeNum(str, locaMd[4]+1, locaMd[5]-1);      

    makeStr(str, time, locaMd[3]+1, locaMd[4]-1);
    
    client.stop();
    return 1;
}

void loop() 
{
    char usCode[] = "aapl";     // stock code
    
    float priceNow = 0;
    char time[20];
    float rise = 0;
    float rise2 = 0;
    
    if(getUsStock(usCode, &priceNow, time, &rise, &rise2) == 1)
    {
        Serial.print(usCode);
        Serial.print('\t');
        Serial.println(time);
        Serial.print(priceNow, 2);
        Serial.print('\t');
        Serial.print(rise, 2);
        Serial.print('\t');
        Serial.print(rise2, 2);
        Serial.println("\r\n-----------------------------");
        
        static int st = 0;
        st = 1-st;
        
        if(rise >= 0) setRGB(255*st, 0, 0);        
        else setRGB(0, 255*st, 0);
        
        if(priceNow > 130.0)
        {
            digitalWrite(21, st);
        }else digitalWrite(21, LOW);
    }
    
    delay(1000);
}

// END FILE