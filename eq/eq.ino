//
// EEW Check
//
// Ver.1.1_M5StickCPlus
//

//#include <M5StickC.h>
#include <M5StickCPlus.h>

#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>

#include <ArduinoJson.h>
#include "efontEnableJa.h"
#include "efont.h"
#include "efontM5StickCPlus.h"

WiFiMulti   wifiMulti;

RTC_TimeTypeDef RTC_TimeStruct;
RTC_DateTypeDef RTC_DateStruct;

void checkEEW(String msg) {

    const size_t CAPACITY = JSON_OBJECT_SIZE(256);
    StaticJsonDocument<CAPACITY> doc;

    deserializeJson(doc, msg);
    JsonObject object = doc.as<JsonObject>();
    
    String request_time = object["request_time"];
    String region_name = object["region_name"];
    String magunitude = object["magunitude"];
    String depth = object["depth"];
    String is_training = object["is_training"];
    String is_cancel = object["is_cancel"];
    String is_final = object["is_final"];
    String alertflg = object["alertflg"];

    Serial.println(object.size());
    Serial.println(request_time);
    Serial.println(region_name);

    M5.Lcd.fillScreen(BLACK);

    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(1);
    M5.Lcd.print(request_time.substring(0,4) + "/" + request_time.substring(4,6) + "/" + request_time.substring(6,8) + " " +
                request_time.substring(8,10) + ":" + request_time.substring(10,12) + ":" + request_time.substring(12,14));
    
    if ( region_name != "" && is_training == "false" && is_cancel == "false" ) {

        char tmp[128];
        uint16_t bkColor;
        bool beep = false;

        bkColor = BLACK;
        if ( alertflg == "警報" ) { bkColor = RED; beep = true; }
        else if ( alertflg == "予報" ) { bkColor = YELLOW; }
        if ( is_final == "true" ) { bkColor = BLUE; }

        // Display Flash
        M5.Lcd.fillScreen(bkColor);

        // LED Flash
        pinMode(GPIO_NUM_10, OUTPUT);
        digitalWrite(GPIO_NUM_10, LOW);
        
        if ( beep ) { M5.Beep.tone(880); }

        delay(100);

        if ( beep ) { M5.Beep.mute(); }
        
        // LED Flash Off
        pinMode(GPIO_NUM_10, OUTPUT);
        digitalWrite(GPIO_NUM_10, HIGH);

        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.setTextColor(WHITE,BLACK);
        M5.Lcd.print(request_time.substring(0,4) + "/" + request_time.substring(4,6) + "/" + request_time.substring(6,8) + " " +
                request_time.substring(8,10) + ":" + request_time.substring(10,12) + ":" + request_time.substring(12,14));
        sprintf(tmp,"%s\0",region_name.c_str());
        printEfont(tmp, 0, 16*1);
        sprintf(tmp,"M %s\0",magunitude.c_str());
        printEfont(tmp, 0, 16*2);
        sprintf(tmp,"%s\0",depth.c_str());
        printEfont(tmp, 0, 16*3);

    } else {

        M5.Lcd.setTextColor(WHITE,BLACK);
        printEfont("情報なし", 0, 16*1);
    }
}

void setup() {
    M5.begin();
    Serial.begin(115200);

    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setRotation(1);

    Serial.println("setup");

    wifiMulti.addAP("SSID1","passphrase");
//    wifiMulti.addAP("SSID2","passphrase");
//    wifiMulti.addAP("SSID3","passphrase");

    while(wifiMulti.run() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println("Wifi Connected.");
    configTime(9 * 3600,0,"ntp.nict.jp");

    struct tm timeInfo;
    if (getLocalTime(&timeInfo)) {
        // Set RTC time
        RTC_TimeStruct.Hours   = timeInfo.tm_hour;
        RTC_TimeStruct.Minutes = timeInfo.tm_min;
        RTC_TimeStruct.Seconds = timeInfo.tm_sec;
        M5.Rtc.SetTime(&RTC_TimeStruct);

        RTC_DateStruct.WeekDay = timeInfo.tm_wday;
        RTC_DateStruct.Month = timeInfo.tm_mon + 1;
        RTC_DateStruct.Date = timeInfo.tm_mday;
        RTC_DateStruct.Year = timeInfo.tm_year + 1900;
        M5.Rtc.SetData(&RTC_DateStruct);
    }

    M5.Beep.tone(880);
    delay(100);
    M5.Beep.mute();
}

void loop() {

    char tmp[128] = "";
    String url;

    if((wifiMulti.run() == WL_CONNECTED)) {

        HTTPClient http;

        M5.Rtc.GetTime(&RTC_TimeStruct);
        M5.Rtc.GetData(&RTC_DateStruct);


        sprintf(tmp,"http://www.kmoni.bosai.go.jp/webservice/hypo/eew/%04d%02d%02d%02d%02d%02d.json",
            RTC_DateStruct.Year,
            RTC_DateStruct.Month,
            RTC_DateStruct.Date,
            RTC_TimeStruct.Hours,
            RTC_TimeStruct.Minutes,
            RTC_TimeStruct.Seconds);

        url = tmp;

        Serial.println(url);
        http.begin(url);

        int httpCode = http.GET();
        if(httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
            Serial.printf("[HTTP] GET... code: %d\n", httpCode);

            // file found at server
            if(httpCode == HTTP_CODE_OK) {
                String payload = http.getString();
                Serial.println(payload);
                checkEEW(payload);
            }
        }

        http.end();
    }

    delay(950);
}
