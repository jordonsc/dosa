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
 * Time (in ms) before firing a second trigger message.
 */
#define REFIRE_DELAY 5000
