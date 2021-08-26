#pragma once

namespace dosa {

char const* sensor_svc_id = "19b10000-e8f2-537e-4f6c-d104768a1214";
char const* sensor_char_id = "19b10001-e8f2-537e-4f6c-d104768a1214";

unsigned int const max_devices = 5;
unsigned long const scan_freq = 5000;
unsigned long const poll_freq = 1000;
unsigned long const conn_timeout = 10000;

}  // namespace dosa
