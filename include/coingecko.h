#include <curl/curl.h>
#include <dpp/dpp.h>

#include <cstdlib>
#include <iostream>

#include "nlohmann/json.hpp"

namespace gecko {

void fetch_tokens(dpp::slashcommand_t event);
void fetch_price(dpp::slashcommand_t event);
void fetch_market_chart(dpp::slashcommand_t event);
size_t write_callback(char* ptr, size_t size, size_t nmemb, std::string* data);

}  // namespace gecko
