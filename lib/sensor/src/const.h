/**
 * Sensor calibration constants.
 */

#pragma once


/**
 * Polling time (in ms) to read and compare IR grid numbers. Increasing this number will normally result in larger
 * changes in the delta, thus increasing sensitivity. Changing this will potentially throw off other calibrations
 * dramatically.
 */
#define IR_POLL 500

/**
 * Temp change (in Celsius) before considering any single pixel as "changed". This is a de-noising threshold, increase
 * this number to reduce the amount of noise the algorithm is sensitive to.
 */
#define SINGLE_DELTA_THRESHOLD 0.6  // Ideal for a thin shield covering the sensor
//#define SINGLE_DELTA_THRESHOLD 1.5  // Ideal for a naked sensor, exposed without any shielding

/**
 * The total temperature delta across all pixels before firing a trigger. This is the primary sensitivity metric, it
 * is also filtered against noise by SINGLE_DELTA_THRESHOLD so it won't show a true full-grid delta.
 */
#define TOTAL_DELTA_THRESHOLD 15.0  // Ideal for a thin shield
//#define TOTAL_DELTA_THRESHOLD 25.0    // Ideal for nungu sensors

/**
 * Minimum number of pixels that are considered 'changed' before we accept a trigger. Increase this to eliminate
 * single-pixel or edge anomalies.
 */
#define MIN_PIXELS_THRESHOLD 1

/**
 * Time (in ms) before firing a second trigger message.
 */
#define REFIRE_DELAY 3000
