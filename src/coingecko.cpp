#include <coingecko.h>
#include <exception>
#include <quickchart.h>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>

using json = nlohmann::json;

// Function to set locale (currency format) settings
// Return locale::classic() if desired locale not available on running system
std::locale get_locale(const std::string& name) {
    try {
        return std::locale(name);
    } catch (const std::runtime_error&) {
        std::cerr << "Warning: Locale '" << name << "' not available. Using classic locale." << std::endl;
        return std::locale::classic();
    }
}

// Determine precision value from str
int determine_precision_from_str(std::string value) {
    // Find the decimal point
    size_t decimal_pos = value.find('.');
    if (decimal_pos == std::string::npos) return 0;  // No decimal point

    // Calculate precision
    int precision = value.length() - decimal_pos - 1;

    // Ensure precision is under 10
    return std::max(0, std::min(precision, 10));
}

// This callback function is called by curl_easy_perform() to write the response
// data into a string
size_t gecko::write_callback(char* ptr, size_t size, size_t nmemb,
                             std::string* data) {
  data->append(ptr, size * nmemb);
  return size * nmemb;
}

void gecko::fetch_price(dpp::slashcommand_t event) {
    bool has_id = event.get_parameter("coingecko_id").index() != 0;
    bool has_ticker = event.get_parameter("ticker").index() != 0;

    std::cout << "=============================" << std::endl;
    std::cout << ">> fetch_price called with:" << std::endl;
    std::cout << "has_id: " << has_id << std::endl;
    std::cout << "has_ticker: " << has_ticker << std::endl;

    if (!has_id && !has_ticker) {
        event.reply(":exclamation: Please provide either coingecko_id or ticker");
        return;
    }

    if (has_ticker) {
        std::string ticker = std::get<std::string>(event.get_parameter("ticker"));

        // Strip $ if it exists at the beginning
        if (!ticker.empty() && ticker[0] == '$') {
            ticker = ticker.substr(1);
            std::cout << ">> stripped $ from ticker: " << ticker << std::endl;
        }

        // Convert ticker to lowercase
        std::transform(ticker.begin(), ticker.end(), ticker.begin(), ::tolower);
        std::cout << ">> fetching by ticker (lowercase): " << ticker << std::endl;

        CURL* list_curl = curl_easy_init();
        if (!list_curl) {
            std::cerr << "Error: could not initialize libcurl" << std::endl;
            return;
        }

        std::string list_url = "https://api.coingecko.com/api/v3/coins/list";
        std::string list_response;

        curl_easy_setopt(list_curl, CURLOPT_URL, list_url.c_str());
        curl_easy_setopt(list_curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(list_curl, CURLOPT_WRITEDATA, &list_response);

        CURLcode list_res = curl_easy_perform(list_curl);
        if (list_res != CURLE_OK) {
            std::cerr << "Error: curl_easy_perform() failed: "
                      << curl_easy_strerror(list_res) << std::endl;
            curl_easy_cleanup(list_curl);
            event.reply(":exclamation: Failed to fetch coin list");
            return;
        }

        curl_easy_cleanup(list_curl);

        try {
            json coins_list = json::parse(list_response);
            std::vector<std::pair<std::string, std::string>> matching_coins; // pairs of (id, name)

            std::cout << ">> searching for matching IDs for ticker: " << ticker << std::endl;

            for (const auto& coin : coins_list) {
                // Convert coin symbol to lowercase for comparison
                std::string coin_symbol = coin["symbol"].get<std::string>();
                std::transform(coin_symbol.begin(), coin_symbol.end(), coin_symbol.begin(), ::tolower);

                if (coin_symbol == ticker) {
                    matching_coins.push_back({
                        coin["id"].get<std::string>(),
                        coin["name"].get<std::string>()
                    });
                }
            }

            std::cout << ">> found " << matching_coins.size() << " matching IDs" << std::endl;

            if (matching_coins.empty()) {
                event.reply(":exclamation: No coins found with ticker: " + ticker);
                return;
            }

            if (matching_coins.size() == 1) {
                // If only one match, fetch price directly
                const dpp::interaction_create_t& interaction_event = event;
                fetch_single_price(matching_coins[0].first, interaction_event);
            } else {
                // Create a select menu for multiple matches
                dpp::message msg(event.command.channel_id, "Multiple coins found with ticker " + ticker + ". Please select one:");

                // Create select menu
                dpp::component select_menu;
                select_menu.type = dpp::cot_selectmenu;
                select_menu.custom_id = "coin_select_" + ticker;
                select_menu.placeholder = "Select a coin";

                // Add options to the select menu
                for (const auto& coin : matching_coins) {
                    dpp::select_option option;
                    option.label = coin.second;         // Display name
                    option.value = coin.first;          // CoinGecko ID
                    option.description = "CoinGecko ID: " + coin.first;
                    select_menu.options.push_back(option);
                }

                // Create action row
                dpp::component action_row;
                action_row.type = dpp::cot_action_row;
                action_row.components.push_back(select_menu);

                // Add the action row to the message
                msg.components.push_back(action_row);

                // Reply with the selection menu
                event.reply(msg);
            }

        } catch (const std::exception& e) {
            std::cerr << "Error parsing coin list: " << e.what() << std::endl;
            event.reply(":exclamation: Error processing coin list");
            return;
        }
    } else {
        std::string coingecko_id = std::get<std::string>(event.get_parameter("coingecko_id"));
        std::cout << ">> fetching by coingecko_id: " << coingecko_id << std::endl;
        fetch_single_price(coingecko_id, event);
    }
}

void gecko::fetch_single_price(const std::string& coingecko_id, const dpp::interaction_create_t& event) {
    std::cout << "=============================" << std::endl;
    std::cout << ">> fetch_single_price called for: " << coingecko_id << std::endl;

    // Acknowledge the interaction first
    event.thinking();

    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Error: could not initialize libcurl" << std::endl;
        return;
    }

    std::string url = "https://api.coingecko.com/api/v3/simple/price?ids=" +
                      coingecko_id + "&vs_currencies=usd%2Cidr";

    std::cout << ">> requesting URL: " << url << std::endl;

    std::string response_data;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "Error: curl_easy_perform() failed: "
                  << curl_easy_strerror(res) << std::endl;
        curl_easy_cleanup(curl);
        event.reply(":exclamation: Failed to fetch price data for " + coingecko_id);
        return;
    }

    try {
        json response_json = json::parse(response_data);
        std::cout << ">> coingecko response: " << response_json << std::endl;

        // Check if the response contains error status
        if (response_json.contains("status") && response_json["status"].contains("error_code")) {
            if (response_json["status"]["error_code"] == 429) {
                event.edit_original_response(dpp::message(":exclamation: Rate limit exceeded. Please try again later."));
                curl_easy_cleanup(curl);
                return;
            }
            std::stringstream error_msg;
            error_msg << ":exclamation: Error fetching price data for " << coingecko_id
                     << " , (" << response_json["status"]["error_code"].get<int>()
                     << ") " << response_json["status"]["error_message"].get<std::string>();
            event.edit_original_response(dpp::message(error_msg.str()));
            curl_easy_cleanup(curl);
            return;
        }

        // Check if the response contains data for the requested coin
        if (response_json.empty() || !response_json.contains(coingecko_id)) {
            event.edit_original_response(dpp::message(":exclamation: No price data found for " + coingecko_id));
            curl_easy_cleanup(curl);
            return;
        }

        // Check if the coin data contains both USD and IDR prices
        if (!response_json[coingecko_id].contains("usd") ||
            !response_json[coingecko_id].contains("idr")) {
            event.edit_original_response(dpp::message(":exclamation: Incomplete price data for " + coingecko_id));
            curl_easy_cleanup(curl);
            return;
        }

        // Safely extract price values
        std::string usd_value_str;
        std::string idr_value_str;

        // Handle different numeric types in the JSON
        if (response_json[coingecko_id]["usd"].is_number()) {
            usd_value_str = response_json[coingecko_id]["usd"].dump();
            std::cout << ">> USD value string: " << usd_value_str << std::endl;
        } else {
            event.edit_original_response(dpp::message(":exclamation: Invalid USD price format for " + coingecko_id));
            curl_easy_cleanup(curl);
            return;
        }

        if (response_json[coingecko_id]["idr"].is_number()) {
            idr_value_str = response_json[coingecko_id]["idr"].dump();
            std::cout << ">> IDR value string: " << idr_value_str << std::endl;
        } else {
            event.edit_original_response(dpp::message(":exclamation: Invalid IDR price format for " + coingecko_id));
            curl_easy_cleanup(curl);
            return;
        }

        // Remove any quotes that might be present in the string
        usd_value_str.erase(remove(usd_value_str.begin(), usd_value_str.end(), '"'), usd_value_str.end());
        idr_value_str.erase(remove(idr_value_str.begin(), idr_value_str.end(), '"'), idr_value_str.end());

        // Safe conversion to double with error checking
        double usd_value, idr_value;
        try {
            size_t usd_pos, idr_pos;
            usd_value = std::stod(usd_value_str, &usd_pos);
            idr_value = std::stod(idr_value_str, &idr_pos);

            std::cout << ">> Converted USD value: " << usd_value << std::endl;
            std::cout << ">> Converted IDR value: " << idr_value << std::endl;

            if (usd_pos != usd_value_str.length() || idr_pos != idr_value_str.length()) {
                throw std::invalid_argument("Invalid number format");
            }
        } catch (const std::exception& e) {
            std::cerr << "Error converting price values: " << e.what() << std::endl;
            event.edit_original_response(dpp::message(":exclamation: Invalid price format for " + coingecko_id));
            curl_easy_cleanup(curl);
            return;
        }

        // Format the values
        std::stringstream ss_usd;
        ss_usd.imbue(get_locale("en_US.UTF-8"));
        ss_usd << std::fixed
               << std::setprecision(determine_precision_from_str(usd_value_str))
               << usd_value;

        std::stringstream ss_idr;
        ss_idr.imbue(get_locale("id_ID.UTF-8"));
        ss_idr << std::fixed
               << std::setprecision(determine_precision_from_str(idr_value_str))
               << idr_value;

        std::cout << ">> Formatted USD: " << ss_usd.str() << std::endl;
        std::cout << ">> Formatted IDR: " << ss_idr.str() << std::endl;

        // Reply with formatted price
        event.edit_original_response(dpp::message(":information_source: " + coingecko_id +
                   " price: $" + ss_usd.str() +
                   " / Rp." + ss_idr.str()));

    } catch (const json::parse_error& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        event.edit_original_response(dpp::message(":exclamation: Failed to parse price data for " + coingecko_id));
    } catch (const std::exception& e) {
        std::cerr << "Error processing price data: " << e.what() << std::endl;
        event.edit_original_response(dpp::message(":exclamation: Error processing price data for " + coingecko_id));
    }

    curl_easy_cleanup(curl);
}

void gecko::fetch_tokens(dpp::slashcommand_t event) {
  CURL* curl = curl_easy_init();
  if (!curl) {
    std::cerr << "Error: could not initialize libcurl" << std::endl;
    return;
  }

  std::string url = "https://api.coingecko.com/api/v3/coins/";
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

  std::string response_data;
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    std::cerr << "Error: curl_easy_perform() failed: "
              << curl_easy_strerror(res) << std::endl;
    curl_easy_cleanup(curl);
    return;
  }

  try {
    json response_json = json::parse(response_data);
    std::cout << "=============================" << std::endl;
    std::cout << ">> coingecko response: " << response_json << std::endl;
    std::string tokens;
    for (auto& token : response_json) {
      tokens.append("- ");
      tokens.append(token["id"]);
      tokens.append("\n");
    }

    event.reply(tokens);

  } catch (const std::exception& e) {
    std::cerr << "Error: failed to parse JSON response: " << e.what()
              << std::endl;
    curl_easy_cleanup(curl);

    event.reply(":exclamation: coins: error failed to call API data.");
  }

  curl_easy_cleanup(curl);
}

void gecko::fetch_market_chart(dpp::slashcommand_t event) {
  std::string token_id = std::get<std::string>(event.get_parameter("token_id"));
  std::string currency = std::get<std::string>(event.get_parameter("currency"));

  CURL* curl = curl_easy_init();
  if (!curl) {
    std::cerr << "Error: could not initialize libcurl" << std::endl;
    return;
  }

  std::string url = "https://api.coingecko.com/api/v3/coins/" + token_id +
                    "/market_chart?vs_currency=" + currency + "&days=1";
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

  std::string response_data;
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    std::cerr << "Error: curl_easy_perform() failed: "
              << curl_easy_strerror(res) << std::endl;
    curl_easy_cleanup(curl);
    return;
  }

  try {
    json response_json = json::parse(response_data);
    std::vector<long> timestamps;
    std::vector<double> prices;
    for (auto& chart : response_json["prices"]) {
      if (timestamps.size() == 250 && prices.size() == 250) {
        break;
      }
      timestamps.push_back(chart.at(0));
      prices.push_back(chart.at(1));
    }
    auto chart = qchart::generate_chart(timestamps, token_id, prices);
    event.reply(chart);
    std::cout << chart << std::endl;

  } catch (const std::exception& e) {
    std::cerr << "Error: failed to parse JSON response: " << e.what()
              << std::endl;
    curl_easy_cleanup(curl);

    event.reply(":exclamation: coins: error failed to call API data.");
  }

  curl_easy_cleanup(curl);
}
