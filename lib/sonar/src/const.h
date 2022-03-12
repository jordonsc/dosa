/**
 * Sensor calibration constants.
 */

#pragma once

/**
 * Time (in ms) before firing a second trigger message.
 */
#define REFIRE_DELAY 3000

/**
 * Percentage of previous distance that's considered a trigger.
 *
 * This is different to the entropy sensitivity setting, SONAR_TRIGGER_THRESHOLD, found in settings.h.
 */
#define SONAR_TRIGGER_COEFFICIENT 0.8
