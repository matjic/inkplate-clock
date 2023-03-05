#include "Network.h"
#include "Arduino.h"
#include "Inkplate.h"
#include "Fonts/FreeSansBold18pt7b.h"
#include "Fonts/FreeSansBold12pt7b.h"
#include "Fonts/FreeSansBold9pt7b.h"


// Conversion factor for micro seconds to seconds
#define uS_TO_S_FACTOR 1000000ULL
// Time between ESP32 wakeups (in seconds)
#define WAKEUP_INTERVAL 3600

Inkplate display;
// create object with all networking functions
Network network;

void setup() {
  Serial.begin(115200);
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
   switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }


  Serial.println("Connecting to network...");
  network.begin();
  Serial.println("Connected to network");

  display.begin();
  display.clearDisplay();

  Serial.println("Drawing virtually");
  mainDraw();
  Serial.println("Completed drawing virtually");

  Serial.println("Drawing physically");
  display.display();
  Serial.println("Completed drawing physically");

  uint64_t time_secs = time(nullptr);
  uint64_t timeToSleep = WAKEUP_INTERVAL - (time_secs % WAKEUP_INTERVAL);

  network.end();
  
  Serial.println("Setting up deep sleep for " + String(timeToSleep) + " seconds");
  // Go to sleep for TIME_TO_SLEEP seconds
  Serial.println("esp_sleep_enable_timer_wakeup: " + String(esp_sleep_enable_timer_wakeup(timeToSleep * uS_TO_S_FACTOR)));

  // Enable wakeup from deep sleep on gpio 36 (wake button)
  Serial.println("esp_sleep_enable_ext0_wakeup: " + String(esp_sleep_enable_ext0_wakeup(GPIO_NUM_36, LOW)));

  Serial.println("Going to sleep");
  // Go to sleep
  esp_deep_sleep_start();
}

void loop() {}

extern Inkplate display;

const int SCREEN_WIDTH_X = 600;
const int SCREEN_WIDTH_Y = 448;
const int PADDING_X = 50;
const int PROGRESS_BAR_HEIGHT =  50;
const int PROGRESS_BAR_WIDTH = SCREEN_WIDTH_X - PADDING_X*2;
const int PROGRESS_BAR_Y_PADDING = 30;


int dayRect_a_y = 75;
int monthRect_a_y = dayRect_a_y + 1*(PROGRESS_BAR_HEIGHT + PROGRESS_BAR_Y_PADDING);
int yearRect_a_y = dayRect_a_y + 2*(PROGRESS_BAR_HEIGHT + PROGRESS_BAR_Y_PADDING);
int centuryRect_a_y = dayRect_a_y + 3*(PROGRESS_BAR_HEIGHT + PROGRESS_BAR_Y_PADDING);

const int PERCENTAGE_TEXT_POS = 468;

void drawProgressBar(int16_t  y, String label, float percentage){
    display.drawRect(PADDING_X, y, PROGRESS_BAR_WIDTH, PROGRESS_BAR_HEIGHT, INKPLATE_BLACK);
    display.fillRect(PADDING_X, y, (int)(PROGRESS_BAR_WIDTH*percentage), PROGRESS_BAR_HEIGHT, INKPLATE_BLACK);
    display.setFont(&FreeSansBold12pt7b);
    int textY =  y+PROGRESS_BAR_HEIGHT+PROGRESS_BAR_Y_PADDING*.75;
    display.setCursor(PERCENTAGE_TEXT_POS,textY);
    display.print(String(percentage*100,2)+"%");
    display.setCursor(PADDING_X,textY);
    display.print(label);
}

String toLocalTimeString(struct tm* timeinfo)
{
    char timeStr[256];
    strftime(timeStr, 256, "%B %e %Y %r", timeinfo);
    return String(timeStr);
}

int getMonthDays(struct tm *timeinfo){
  int month = timeinfo->tm_mon+1;
  switch(month){
    case 1:
      return 31;
    case 2:
      if(timeinfo->tm_year % 4 == 0){
        return 29;
      }
      return 28;
    case 3:
      return 31;
    case 4:
      return 30;
    case 5:
      return 31;
    case 6:
      return 30;
    case 7:
      return 31;
    case 8:
      return 31;
    case 9:
      return 30;
    case 10:
      return 31;
    case 11:
      return 30;
    case 12:
      return 31;
  }
}

void mainDraw() {
    display.setTextColor(0, 7);
    //display.setTextSize(1);    
    // Get seconds since 1.1.1970.
    time_t nowSecs = time(nullptr);

    // Used to store time
    struct tm *timeinfo = localtime(&nowSecs);
    // heading
    char timeStr[256];
    strftime(timeStr, 256, "%A, %B %e %Y %H:%M", timeinfo);
    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(50, 54);
    display.print(String(timeStr));

    
    float voltage = display.readBattery(); // Read battery voltage
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(SCREEN_WIDTH_X-50,SCREEN_WIDTH_Y-20);
    display.print(String(voltage,2)+"V");    

    int daySecs = timeinfo->tm_sec + timeinfo->tm_min*60 + timeinfo->tm_hour*60*60;
    float dayFrac = (float)daySecs/(24.0*60*60);
    // Sunday, March  5 2023
    strftime(timeStr, 256, "%B %e %Y", timeinfo);
    drawProgressBar(dayRect_a_y, String(timeStr), dayFrac);

    
    // March 2023
    strftime(timeStr, 256, "%B %Y", timeinfo);
    float monthFrac = ((timeinfo->tm_mday-1)*86400.0+daySecs)/(86400.0*getMonthDays(timeinfo));
    drawProgressBar(monthRect_a_y, String(timeStr), monthFrac);

    
    // 2023
    strftime(timeStr, 256, "%Y", timeinfo);
    int maybeLeapDay = timeinfo->tm_year % 4 == 0 ? 1 : 0;
    float yearFrac = ((timeinfo->tm_yday)*86400.0+daySecs)/(86400.0*(365 + maybeLeapDay));
    drawProgressBar(yearRect_a_y, String(timeStr), yearFrac);

    // hard coded 21st century
    float yearInCent = timeinfo->tm_year-100; // 2023 represented as 123 (1900+123=2023) => 123 - 100 = 23
    float centFrac = (yearInCent+yearFrac)/100.0; 
    drawProgressBar(centuryRect_a_y, "21st Century", centFrac);

}
