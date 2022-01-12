#include "RollScreen.h"

unsigned DEV_RollScreen::active_any = false;

DEV_RollScreen::DEV_RollScreen(unsigned number, Pin MOTOR_A, Pin MOTOR_B, Pin SENSOR, Pin SENSOR_ACTIVE): Service::WindowCovering() {
    this->number        = number;
    this->MOTOR_A       = MOTOR_A;
    this->MOTOR_B       = MOTOR_B;
    this->SENSOR        = SENSOR;
    this->SENSOR_ACTIVE = SENSOR_ACTIVE;
    pinMode(MOTOR_A,       OUTPUT);
    pinMode(MOTOR_B,       OUTPUT);
    pinMode(SENSOR,        INPUT);
    pinMode(SENSOR_ACTIVE, OUTPUT);
    digitalWrite(MOTOR_A,       LOW);
    digitalWrite(MOTOR_B,       LOW);
    digitalWrite(SENSOR_ACTIVE, LOW);
    // CHANNEL_A = number * 2;
    // CHANNEL_B = number * 2 + 1;
    // ledcSetup(CHANNEL_A, 10000, 8);
    // ledcSetup(CHANNEL_B, 10000, 8);
    // ledcAttachPin(MOTOR_A, CHANNEL_A);
    // ledcAttachPin(MOTOR_B, CHANNEL_B);

    log_d("RollScreen #%d initialized with motor pin (%d, %d), sensor pin (%d, %d)",
          number, MOTOR_A, MOTOR_B, SENSOR, SENSOR_ACTIVE);
    get_ROM();
    log_d("current position: %d, minimum: %d, maximum: %d",
          position.current, position.min, position.max);

    current = new Characteristic::CurrentPosition(to_range(position.current));
    target  = new Characteristic::TargetPosition(to_range(position.target));
    target->setRange(0,100,1);
    state   = new Characteristic::PositionState(2);
}

boolean DEV_RollScreen::update() {
    if (abs(position.current - position.target) > 1) {
        if ((position.target > position.current) !=
            (target->getNewVal<int>() > target->getNewVal<int>())) {
            position.target = position.current;
            return false;
        }
        position.target = from_range(target->getNewVal<int>());
    }
    else {
        position.target = from_range(target->getNewVal<int>());
    }
    return true;
}

void DEV_RollScreen::loop() {
    unsigned long t = millis();
    float dt = (t - time) / 1000.0;
    time = t;

    if (dt * 1000 > 100) log_d("overload #%d %f %d", number, dt * 1000);

    // Start moving
    if (!active && abs(position.current - position.target) > 1) {
        active = true;
        active_any++;
        log_d("active: %d", active_any);
        digitalWrite(SENSOR_ACTIVE, HIGH);
        delay(20);
        encoder = read_encoder();
        last_edge_detected = millis();
        speed = 0;
        direction = position.target > position.current ? 1 : -1;
    }
    // Reset origin
    // else if (active && position.target == position.max) {
    //     if (speed < 1.0) {
    //         log_d("reset origin");
    //         position.current = position.target;
    //         active = false;
    //         active_any--;
    //         log_d("active: %d", active_any);
    //         speed = 0;
    //     }
    // }
    // Stop moving
    else if (active && abs(position.current - position.target) <= 1) {
        active = false;
        active_any--;
        log_d("active: %d", active_any);
        speed = 0;
    }

    if (active_any == 0 && millis() > last_edge_detected + 2000) {
        // log_d("sensor disabled");
        digitalWrite(SENSOR_ACTIVE, LOW);
    }

    if (millis() - last_edge_detected > 1000) speed = 0;


    if (read_encoder() != encoder && millis() < last_edge_detected + 1000) {
        position.current += direction;
        encoder = read_encoder();
        speed = SPEED_FILTER * speed + direction *
            (1.0 - SPEED_FILTER) * 1000.0 / (millis() - last_edge_detected);
        last_edge_detected = millis();
        put_ROM();
        log_d("Screen #%d moved: %d", number, position.current);
    }

    if (position.target > position.current + 1) {
        target_speed = SPEED;
        // log_d("current: %d, target: %d, speed: %f, target speed: %f",
        //       position.current, position.target, speed, target_speed);
    }
    else if (position.target < position.current - 1) {
        target_speed = - SPEED;
        // log_d("current: %d, target: %d, speed: %f, target speed: %f",
        //       position.current, position.target, speed, target_speed);
    }
    else target_speed = 0;

    // if (target_speed == 0) duty = 0;
    // else {
    //     duty += (target_speed - speed) * dt * SPEED_GAIN * 256;
    //     log_d("duty: %f", duty);
    // }

    if (target_speed > 0) {
        digitalWrite(MOTOR_A, HIGH);
        digitalWrite(MOTOR_B, LOW);
    }
    else if (target_speed < 0) {
        digitalWrite(MOTOR_A, LOW);
        digitalWrite(MOTOR_B, HIGH);
    }
    else {
        digitalWrite(MOTOR_A, LOW);
        digitalWrite(MOTOR_B, LOW);
    }

    // if (duty > 0) {
    //     ledcWrite(CHANNEL_A, constrain(duty, 0, 256));
    //     ledcWrite(CHANNEL_B, 0);
    // }
    // else if (duty < 0) {
    //     ledcWrite(CHANNEL_A, 0);
    //     ledcWrite(CHANNEL_B, constrain(- duty, 0, 256));
    // }
    // else {
    //     ledcWrite(CHANNEL_A, 0);
    //     ledcWrite(CHANNEL_B, 0);
    // }

    int pos = to_range(position.current);
    if (abs(position.current - position.target) <= 1)
        pos = to_range(position.target);

    if (t - last_pushed > 1000 &&
        current->getVal() != pos) {
        current->setVal(pos);
        int s = 2;
        if (target_speed > 0) s = 1;
        if (target_speed < 0) s = 0;
        state->setVal(s);
        last_pushed = t;

        log_d("#%d, current: %d, target: %d, speed: %f",
              number, position.current, position.target, speed);
    }
}


void DEV_RollScreen::set_position(Position p) {
    log_d("current: %d, target: %d, minimum: %d, maximum: %d",
          p.current, p.target, p.min, p.max);
    position = p;
    put_ROM();
    current->setVal(to_range(position.current));
    target->setVal(to_range(position.target));
}

bool DEV_RollScreen::read_encoder() {
    return analogRead(SENSOR) > 100;
}

int DEV_RollScreen::to_range(int pos) {
    if (position.max == position.min) return 0;
    float range = (pos - position.min) * 100.0 / (position.max - position.min);
    return constrain(round(range), 0, 100);
}
int DEV_RollScreen::from_range(int range) {
    return
        round(position.min + (position.max - position.min) * (range / 100.0));
}

void DEV_RollScreen::get_ROM() {
    EEPROM.get(number * sizeof(Position), position);
}
void DEV_RollScreen::put_ROM() {
    EEPROM.put(number * sizeof(Position), position);
    EEPROM.commit();
}
