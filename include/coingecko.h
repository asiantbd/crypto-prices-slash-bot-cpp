#include <curl/curl.h>
#include <dpp/dpp.h>

#include <cstdlib>
#include <iostream>

#include "nlohmann/json.hpp"

namespace gecko {

void fetch_tokens(dpp::slashcommand_t event);
void fetch_price(dpp::slashcommand_t event);

}  // namespace gecko
