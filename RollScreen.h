#include <Arduino.h>
#include <EEPROM.h>
#include <HomeSpan.h>

#define Pin int

struct Position {
    int current;
    int target;
    int min;
    int max;
};

class DEV_RollScreen: Service::WindowCovering {
private:
    const float SPEED = 5.0;
    const float SPEED_GAIN = 0.5; // 1 / s^2
    const float SPEED_FILTER = 0.0;
    /* const unsigned  */

    unsigned number;
    Pin MOTOR_A;
    Pin MOTOR_B;
    Pin SENSOR;
    Pin SENSOR_ACTIVE;
    /* unsigned CHANNEL_A; */
    /* unsigned CHANNEL_B; */

    static unsigned active_any;
    static unsigned long last_active;
    
    bool active = false;

    unsigned long time = 0;
    unsigned long last_pushed = 0;

    bool encoder;
    unsigned long last_edge_detected = 0;

    int direction = 0;

    float speed = 0; // step/s
    float target_speed = 0;
    float duty = 0;

    SpanCharacteristic *current;
    SpanCharacteristic *target;
    SpanCharacteristic *state;

public:

    Position position;

    DEV_RollScreen(unsigned number, Pin a, Pin b, Pin SENSOR, Pin SENSOR_ACTIVE);
    void set_position(Position p);

private:
    boolean update();
    void loop();

    bool read_encoder();

    int to_range(int pos);
    int from_range(int range);

    void get_ROM();
    void put_ROM();

};
