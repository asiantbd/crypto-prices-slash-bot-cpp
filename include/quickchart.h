#include <curl/curl.h>

#include <cstdlib>
#include <iostream>
#include <vector>

#include "nlohmann/json.hpp"

namespace qchart {

std::string generate_chart(std::string label1, std::vector<long> data1,
                           std::string label2, std::vector<long> data2);
size_t write_callback(char* ptr, size_t size, size_t nmemb, std::string* data);
}  // namespace qchart
