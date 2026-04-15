#include "time_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* DAY_NAMES[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char* MONTH_NAMES[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

bool getLocalTimeForTZ(const char* posixTz, struct tm* result) {
    // Save current TZ, set new one, get time, restore
    const char* oldTz = getenv("TZ");
    char savedTz[128] = {0};
    if (oldTz) {
        strncpy(savedTz, oldTz, sizeof(savedTz) - 1);
    }

    setenv("TZ", posixTz, 1);
    tzset();

    time_t now;
    time(&now);
    localtime_r(&now, result);

    // Restore previous TZ
    if (oldTz) {
        setenv("TZ", savedTz, 1);
    } else {
        unsetenv("TZ");
    }
    tzset();

    return result->tm_year > 100;  // Year > 2000 means NTP has synced
}

void formatTime12h(const struct tm* t, char* buf, size_t bufSize) {
    int hour = t->tm_hour;
    const char* ampm = "AM";
    if (hour >= 12) {
        ampm = "PM";
        if (hour > 12) hour -= 12;
    }
    if (hour == 0) hour = 12;
    snprintf(buf, bufSize, "%d:%02d %s", hour, t->tm_min, ampm);
}

void formatTime24h(const struct tm* t, char* buf, size_t bufSize) {
    snprintf(buf, bufSize, "%d:%02d", t->tm_hour, t->tm_min);
}

void formatDate(const struct tm* t, char* buf, size_t bufSize) {
    snprintf(buf, bufSize, "%s %s %d",
             DAY_NAMES[t->tm_wday],
             MONTH_NAMES[t->tm_mon],
             t->tm_mday);
}

void formatDayShort(const struct tm* t, char* buf, size_t bufSize) {
    snprintf(buf, bufSize, "%s", DAY_NAMES[t->tm_wday]);
}

void formatMonthDay(const struct tm* t, char* buf, size_t bufSize) {
    snprintf(buf, bufSize, "%s %d", MONTH_NAMES[t->tm_mon], t->tm_mday);
}
