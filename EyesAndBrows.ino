/**
 * EyesAndBrows.ino
 * 
 * Expressive animatronic art piece/desktop toy made from two ping pong balls, 
 * hinges, brass scrap, analog joystick w/button and 4 hobby servos.
 * Originally written for the ESP32 microcontroller.
 * 
 * https://github.com/ripred/EyesAndBrows
 * 
 * Written: May 2021 Trent M. Wyatt
 * 
 */

////////////////////////////////////////////////////////////////////////////////
/// Project-specific pin usage:
/// *NOTE: These pins are for the ESP32.  
/// Define a separate set for other microcontrollers.

#define DBGLED              4

// *NOTE: BUTTON1, IN1, and IN2 are the button and the two potentiometer inputs
// from an analog joystick

#define BUTTON1             5
#define IN1                 34
#define IN2                 35

// servos for the eyes
#define PAN_SERVO_PIN       17  // look left/right
#define TILT_SERVO_PIN      16  // look up/down

// servos for the eyebrows
#define FOREHEAD_SERVO_PIN  18  // raise/lower eyebrows
#define EYEBROW_SERVO_PIN   19  // angle of eyebrows

////////////////////////////////////////////////////////////////////////////////

#define ARRAYSIZE(a) (sizeof(a) / sizeof(*a))

#define JITTER_FILTER   0.035

#include "extservo.h"

ExtServo pan(PAN_SERVO_PIN, IN1);
ExtServo tilt(TILT_SERVO_PIN, IN2);
ExtServo eyebrow(EYEBROW_SERVO_PIN, IN1);
ExtServo forehead(FOREHEAD_SERVO_PIN, IN2);

#define BOUNCING_BALL           0   // moves the eyes in a "vcr screensaver" type of pattern
#define TRACK_JOYSTICK_EYES     1   // track the joystick and control the eyes with it
#define TRACK_JOYSTICK_EYEBROW  2   // track the joystick and control the eyebrows with it
#define RANDOM_1                3   // pick a random spot and move to it, delay, loop

int mode = TRACK_JOYSTICK_EYES;

#define STATUS_LED_ON           0x01
uint32_t status = 0x0000;

bool enable_sleep_blink = false;    // blink when true and in sleep mode

#define CHECK_PERIOD    15000
#define BLINK_PERIOD       30
#define RANDOM_PERIOD    2000
#define TRACKING_PERIOD    50
#define DBGPRNT_PERIOD   1500
#define IDLE_SECONDS        3

int durations[] = { 500, 50, 10, 5 };
int duration_index = 0;

void set_led(bool bvalue=true) {
    digitalWrite(DBGLED, bvalue);
    status = bvalue ? (status | STATUS_LED_ON) : (status & ~STATUS_LED_ON);
}

void toggle_led() {
    bool led_on = (status & STATUS_LED_ON) == STATUS_LED_ON;
    for (int i=0; i < 3; ++i) {
        set_led(led_on = !led_on);
        delay(BLINK_PERIOD);
    }
}

void blink_led() {
    toggle_led();
    delay(BLINK_PERIOD);
    toggle_led();
}

void update_led() {
    static uint32_t timer = millis() + CHECK_PERIOD;
    if ((CHECK_PERIOD > 0) && millis() >= timer) {
        timer = millis() + CHECK_PERIOD;
        bool blink = false;

        if (mode == TRACK_JOYSTICK_EYEBROW) {
            blink = eyebrow.get_idle() && forehead.get_idle();
        } else {
            blink = pan.get_idle() && tilt.get_idle();
        }

        if (enable_sleep_blink && blink) {
            blink_led();
        }

        set_led(status & STATUS_LED_ON);
    }
}


void setup() {
    Serial.begin(115200);
    
    pinMode(BUTTON1, INPUT_PULLDOWN);
    pinMode(DBGLED, OUTPUT);

    set_led(true);
    toggle_led();

    pan.name = "pan";
    pan.set_debug();

    tilt.name = "tilt";
    tilt.set_debug();

    eyebrow.name = "eyebrow";
    eyebrow.set_debug();

    forehead.name = "forehead";
    forehead.set_debug();
    
    /**
     * Eye Servo Ranges / Orientation:
     *  Pan:      Right: ~  25  Left: ~ 100,
     * Tilt:      Down: ~   5     Up: ~  70,
     * 
     * Eyebrow:  Angry: ~ 46   Anxious: ~  94
     * Forehead: Raise: ~ 64   Lower:   ~ 118
     * 
     */
     pan.begin(50, 104,  29);
    tilt.begin(50,   5,  70);

     eyebrow.begin(50,  38, 145);
    forehead.begin(50, 118,  63);

    do {
        pan.track(30);
        tilt.track(30);
        eyebrow.track(30);
        forehead.track(30);
    } while (!pan.update() || !tilt.update() || !eyebrow.update() || !forehead.update());

    pan.idle_wait = IDLE_SECONDS;
    tilt.idle_wait = IDLE_SECONDS;
    eyebrow.idle_wait = IDLE_SECONDS;
    forehead.idle_wait = IDLE_SECONDS;

    set_led(true);

    pan.clr_debug();
    tilt.clr_debug();
    eyebrow.clr_debug();
    forehead.clr_debug();
} 

void loop() {
    update_led();

    switch (mode) {
        case TRACK_JOYSTICK_EYES:
             pan.track(TRACKING_PERIOD);
            tilt.track(TRACKING_PERIOD);
        break;

        case TRACK_JOYSTICK_EYEBROW:
             eyebrow.track(TRACKING_PERIOD);
            forehead.track(TRACKING_PERIOD);
        break;

        case RANDOM_1: {
            static uint32_t timer = millis() + RANDOM_PERIOD;
            int period = RANDOM_PERIOD - 100;

            if (millis() >= timer) {
                timer = millis() + RANDOM_PERIOD;
                pan.write(min(pan.lower, pan.upper) + random(pan.range()), period);
                tilt.write(min(tilt.lower, tilt.upper) + random(tilt.range()), period);
                eyebrow.write(min(eyebrow.lower, eyebrow.upper) + random(eyebrow.range()), period);
                forehead.write(min(forehead.lower, forehead.upper) + random(forehead.range()), period);
            }
        }
        break;

        case BOUNCING_BALL: {
            static float x = 40;
            static float y = 35;
            static float dx = 1;
            static float dy = 0.5;

            float lower = min(pan.lower, pan.upper);
            float upper = max(pan.lower, pan.upper);
            if (x+dx < lower || x-dx < lower || x+dx > upper || x-dx > upper) { dx *= -1; }

            lower = min(tilt.lower, tilt.upper);
            upper = max(tilt.lower, tilt.upper);
            if (y+dy < lower || y-dy < lower || y+dy > upper || y-dy > upper) { dy *= -1;
            }

            x += dx;
            y += dy;

             pan.write(x, durations[duration_index]);
            tilt.write(y, durations[duration_index]);

            uint32_t timer = millis() + durations[duration_index];
            while (millis() < timer) {
                 pan.update();
                tilt.update();
            }
        }
        break;
    }
    
    pan.update();
    tilt.update(); 
    eyebrow.update();
    forehead.update();

    if (digitalRead(BUTTON1)) {
        delay(34);
        uint32_t now = millis();
        while (digitalRead(BUTTON1)) {
            if (millis() - now >= DBGPRNT_PERIOD) {
                Serial.printf("Pan: %.01f, Tilt: %.01f, Eyebrow: %.01f, Forehead: %.01f\n", 
                    pan.current, tilt.current, eyebrow.current, forehead.current);
                blink_led();
                while (digitalRead(BUTTON1)) { }
                return;
            }
        }

        blink_led();

        switch (mode) {
            case TRACK_JOYSTICK_EYES:
            mode = TRACK_JOYSTICK_EYEBROW;
            break;

            case TRACK_JOYSTICK_EYEBROW:
            mode = RANDOM_1;
            break;

            case RANDOM_1:
            mode = TRACK_JOYSTICK_EYES;
            break;

            case BOUNCING_BALL:
            duration_index = (duration_index + 1) % ARRAYSIZE(durations);
            if (duration_index == 0) {
                mode = TRACK_JOYSTICK_EYES;
                do {
                  pan.track();
                  tilt.track();
                } while (!pan.update() || !tilt.update());
            }
            break;
        }
        set_led(mode != 0);
    }
}
