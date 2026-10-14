#ifndef SPEED_PROTOCOL_H
#define SPEED_PROTOCOL_H

// Convert an ASCII command ("0" through "100") into a PWM percentage.
// Returns -1 for malformed or out-of-range input.
static int parse_speed(const char *s) {
    unsigned int value = 0;
    unsigned int digits = 0;
    while (*s) {
        if (*s < '0' || *s > '9' || digits == 3) return -1;
        value = value * 10 + (unsigned int)(*s - '0');
        ++digits;
        ++s;
    }
    return digits && value <= 100 ? (int)value : -1;
}

#endif