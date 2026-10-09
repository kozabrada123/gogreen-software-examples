#include "adafruit_io.h"

// Small example to publish data to Adafruit IO
//
// Also see https://github.com/adafruit/Adafruit_IO_Arduino/tree/master/examples
//
// Change your wifi credentials and IO username and key in adafruit_io.h

bool updated_once = false;

int last_published_data_point = 0;
unsigned long last_update_time_s = 0;
AdafruitIO_Feed *io_data_point = io.feed("data-point");

void setup() {
  Serial.begin(115200);

  pinMode(21, INPUT_PULLUP);
  io.connect();

  while(io.status() < AIO_CONNECTED) {
    io.run();
    Serial.print(".");
    delay(500);
  }

  // we are connected
  Serial.println();
  Serial.println(io.statusText());
}

void loop() {

  int data_point = !digitalRead(21);

  unsigned long now_s = millis() / 1000;

  io.run();

  if (io.status() >= AIO_CONNECTED) {
    if (now_s - last_update_time_s >= 3) {
      if ((last_published_data_point != data_point || now_s - last_update_time_s >= 30) || !updated_once) {
        Serial.println("Publishing data to io..");

        io_data_point->save(data_point);

        last_update_time_s = now_s;
        last_published_data_point = data_point;
        if (!updated_once) updated_once = true;
      }
    } 
  }
}
