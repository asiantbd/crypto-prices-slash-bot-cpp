#include <curl/curl.h>

#include <cstdlib>
#include <iostream>
#include <vector>

#include "nlohmann/json.hpp"

namespace qchart {

std::string generate_chart(std::vector<long> data1, std::string label,
                           std::vector<double> data2);
size_t write_callback(char* ptr, size_t size, size_t nmemb, std::string* data);
}  // namespace qchart
