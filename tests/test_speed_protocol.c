#include <assert.h>
#include "../msp430/speed_protocol.h"

int main(void) {
    assert(parse_speed("0") == 0);
    assert(parse_speed("7") == 7);
    assert(parse_speed("50") == 50);
    assert(parse_speed("100") == 100);
    assert(parse_speed("001") == 1);
    assert(parse_speed("") == -1);
    assert(parse_speed("101") == -1);
    assert(parse_speed("-1") == -1);
    assert(parse_speed("12a") == -1);
    assert(parse_speed("1000") == -1);
    return 0;
}