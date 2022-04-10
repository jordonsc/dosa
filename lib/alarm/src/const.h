#pragma once

/**
 * Alert LED
 *
 * Red LED, blinks slowly when hearing a security alert, blinks rapidly when hearing a breach.
 */
#define DOSA_LED_ALERT 5

/**
 * Time alert LED is active for.
 *
 * (indefinite)
 */
#define DOSA_ALERT_DURATION 0

/**
 * Activity LED
 *
 * Blue LED, steady-on for short duration when receiving a trigger message in the listen list.
 */
#define DOSA_LED_ACTIVITY 6

/**
 * Time activity LED is active for.
 *
 * 10 seconds.
 */
#define DOSA_ACTIVITY_DURATION 10000

/**
 * Alert button pin.
 */
#define DOSA_ALERT_BUTTON 7
#define DOSA_BUTTON_LONG_PRESS 1500

/**
 * Sequence timings
 */
#define DOSA_ACTIVITY_SEQ_ON DOSA_ACTIVITY_DURATION
#define DOSA_ACTIVITY_SEQ_OFF 0

#define DOSA_ALERT_SEQ_ON 1000
#define DOSA_ALERT_SEQ_OFF 500

#define DOSA_BREACH_SEQ_ON 250
#define DOSA_BREACH_SEQ_OFF 150

#define DOSA_ERROR_SEQ_ON 5000
#define DOSA_ERROR_SEQ_OFF 0
