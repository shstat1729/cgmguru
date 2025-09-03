#include "utils.h"
#include <ctime>

// Helper function to format a POSIXct timestamp
std::string format_timestamp(double timestamp) {
  time_t raw_time = static_cast<time_t>(timestamp);
  struct tm* time_info = gmtime(&raw_time);
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", time_info);
  std::string result(buffer);
  result += " UST";
  return result;
}
