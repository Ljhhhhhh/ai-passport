#include "passport_alert.h"
#include <assert.h>
#include <stddef.h>

int main(void)
{
    uint32_t last = 0;
    assert(!passport_alert_accept(NULL, 1));
    assert(!passport_alert_accept(&last, 0));
    assert(passport_alert_accept(&last, 1));
    assert(!passport_alert_accept(&last, 1));
    assert(passport_alert_accept(&last, 3));
    assert(!passport_alert_accept(&last, 2));
    last = 0; /* reconnect starts a fresh session */
    assert(passport_alert_accept(&last, 1));

    passport_alert_play(PASSPORT_ALERT_NONE);
    passport_alert_play(PASSPORT_ALERT_WAIT);
    passport_alert_play(PASSPORT_ALERT_DONE);
    passport_alert_play(PASSPORT_ALERT_ERROR);
    passport_alert_play(PASSPORT_ALERT_NEW_MSG);
    passport_alert_play_chime();

    bool voice = false;
    uint8_t vol = 0;
    passport_alert_set_settings(false, 50);
    passport_alert_get_settings(&voice, &vol);
    assert(!voice && vol == 50);

    passport_alert_set_settings(true, 120); // capped at 100
    passport_alert_get_settings(&voice, &vol);
    assert(voice && vol == 100);

    return 0;
}
