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
 */
#define DOSA_SONAR_TRIGGER_THRESHOLD 0.8

/**
 * Number of times we report an increased distance before actually increasing the calibration.
 */
#define DOSA_SONAR_CALIBRATION_THRESHOLD 3
