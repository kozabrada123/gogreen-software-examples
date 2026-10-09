
// wired so that pressing the button connects the pin to GND
#define BUTTON_IN_PIN 22
#define OUT_PIN 23

unsigned long last_button_press_ms = 0;

bool last_button_state = false;
bool toggle_status = false;

void setup() {
  pinMode(BUTTON_IN_PIN, INPUT_PULLUP);
  pinMode(OUT_PIN, OUTPUT);
  digitalWrite(OUT_PIN, toggle_status);
} 


void loop() {

  unsigned long now = millis();
  // ! because we expect 0 for pressed and 1 for not pressed
  bool button_state = !digitalRead(BUTTON_IN_PIN);

  if ((button_state && !last_button_state) && now - last_button_press_ms > 200) {
    toggle_status = !toggle_status;
    last_button_press_ms = now;
    digitalWrite(OUT_PIN, toggle_status);
  }

  last_button_state = button_state;

  // update 50x a second
  delay(20);
}
