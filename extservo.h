/**
 * ExtServo.h
 * 
 * class for managing a servo with additional features specific to this project
 * 
 */
#ifndef    EXTSERVO_H_INC
#define    EXTSERVO_H_INC

#ifdef ESP32
//#warning ESP32 mode
#define MAX_ANALOG_IN   4095
#else
//#warning Arduino mode
#define MAX_ANALOG_IN   255
#endif

#include    <ESP32Servo.h>
#include    <string>
#include    <vector>

using std::string;

struct ExtServo {
public:
    // boolean bit flags for status value
    #define     DEBUG_MSGS      0x01
    #define     OUTPUT_IDLE     0x02
    #define     ENABLE_IDLE     0x04

    uint32_t    status;     // status/config value
    string      name;       // display name for debug/log

    int32_t    input_pin;   // pot pin
    int32_t    servo_pin;   // servo pin

    uint32_t    max_jitter; // max input jitter ignored
    uint32_t    last;       // last input pot value read

    float       starting;   // starting servo pos
    float       target;     // desired servo pos
    float       current;    // current servo pos
    float       step_amt;   // calculated steps / mS

    uint32_t    start;      // start time for servo write/update
    uint32_t    timer;      // end time for servo write/update
    uint32_t    idle_wait;  // time to wait before putting servos to sleep

    float       upper;      // upper servo limit
    float       lower;      // lower servo limit
    
    Servo       servo;      // servo actuator

    uint32_t    last_out;   // last value actually written to servo

public:
    bool    get_debug() { return (status & DEBUG_MSGS) == DEBUG_MSGS; }
    void    set_debug() { status |= DEBUG_MSGS; }
    void    clr_debug() { status &= ~DEBUG_MSGS; }

    bool    get_idle() { return (status & OUTPUT_IDLE) == OUTPUT_IDLE; }
    void    set_idle() { status |= OUTPUT_IDLE; }
    void    clr_idle() { status &= ~OUTPUT_IDLE; }

    bool    get_idle_enable() { return (status & ENABLE_IDLE) == ENABLE_IDLE; }
    void    set_idle_enable() { status |= ENABLE_IDLE; }
    void    clr_idle_enable() { status &= ~ENABLE_IDLE; }

    float   range() { return (lower < upper) ? upper - lower : lower - upper; }

private:
    void    write_current_to_servo() {
        if ((uint32_t) current != last_out) {
            if (get_idle_enable()) {
                if (get_idle()) {
                    pinMode(servo_pin, OUTPUT);
                    servo.attach(servo_pin);
                    clr_idle();
                    if (get_debug()) {
                        Serial.printf("Setting '%s' servo as output\n", name.c_str());
                    }
                }
            }

            last_out = (uint32_t) current;
            servo.write(last_out);

            if (get_debug()) {
                Serial.printf("Writing to '%s' servo: %d\n", name.c_str(), last_out);
            }
        }
    }

public:
    ExtServo(const int32_t pin_servo, const int32_t pin_input = -1) : 
      input_pin(pin_input),
      servo_pin(pin_servo) {
        status = ENABLE_IDLE;
        name = "";
        max_jitter = (float) MAX_ANALOG_IN * JITTER_FILTER;
        last = 0;
        target = 0;
        current = 0;
        step_amt = 0;
        start = timer = millis();
        idle_wait = 10;
        lower = 0;
        upper = 0;
        last_out = 0;
    }

    void begin(uint32_t hertz, uint32_t lower, uint32_t upper) {
        (*this).lower = lower;
        (*this).upper = upper;

        if (input_pin >= 0) {
            pinMode(input_pin, INPUT);
        }
        
        servo.setPeriodHertz(hertz);
        servo.attach(servo_pin,  500, 2400);
        
        if (get_idle_enable()) {
            clr_idle();
        }
    }
    
    void write(float const value, float const period = 0) {
        if ((uint32_t) target != (uint32_t) value || (uint32_t) target != (uint32_t) current) {
            start = millis();
            timer = start;
            target = value;
            starting = current;
            if (period <= 0) {
                current = value;
                step_amt = 0;
            } else {
                timer += period;
                step_amt = (value - current) / period;
                current += step_amt;
            }
        }
        write_current_to_servo();
    }

    bool update(bool wake=false) {
        bool result = true;
        uint32_t now = millis();
        if (!wake && current == target) {
            if (get_idle_enable() && !get_idle()) {
                uint32_t elapsed = now - start;
                if (elapsed >= idle_wait * 1000) {
                    servo.detach();
                    pinMode(servo_pin, INPUT);
                    set_idle();
                    if (get_debug()) {
                        Serial.printf("Setting '%s' servo to sleep\n", name.c_str());
                    }
                }
            }
        } else {
            if (now >= timer) {
                current = target = (uint32_t) target;
            } else {
                current = starting + (now - start) * step_amt;
                result = false;
            }
            write_current_to_servo();
        }
        return result;
    }

    int read_input() {
        return analogRead(input_pin);
    }

    
    void track(int const period = 0) {
        int input = read_input();
        if (input != last) {
            if (((last > input) && (last - input) >= max_jitter) || ((input > last) && (input - last) >= max_jitter)) {
                last = input;
                write(map(input, 0, MAX_ANALOG_IN, lower, upper), period);
            }
        }
    }
};

#endif // #ifndef EXTSERVO_H_INC
