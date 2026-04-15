#pragma once
#include <time.h>

// Get local time for a given POSIX timezone string
bool getLocalTimeForTZ(const char* posixTz, struct tm* result);

// Format time as "2:34 PM"
void formatTime12h(const struct tm* t, char* buf, size_t bufSize);

// Format time as "14:34"
void formatTime24h(const struct tm* t, char* buf, size_t bufSize);

// Format date as "Mon Apr 6"
void formatDate(const struct tm* t, char* buf, size_t bufSize);

// Format day name as "Mon", "Tue", etc.
void formatDayShort(const struct tm* t, char* buf, size_t bufSize);

// Format as "Apr 15"
void formatMonthDay(const struct tm* t, char* buf, size_t bufSize);

// Hardware RTC (PCF8563)
bool rtcInit();                // Init RTC on I2C, returns true if found
void rtcSyncFromSystem();      // Write system time to RTC (after NTP sync)
bool rtcRestoreToSystem();     // Read RTC and set system clock, returns true if valid
