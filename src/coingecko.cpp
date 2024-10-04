#include <coingecko.h>
#include <exception>
#include <quickchart.h>
#include <iomanip>
#include <locale>
#include <sstream>

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
// This callback function is called by curl_easy_perform() to write the response
// data into a string
size_t gecko::write_callback(char* ptr, size_t size, size_t nmemb,
                             std::string* data) {
  data->append(ptr, size * nmemb);
  return size * nmemb;
}

void gecko::fetch_price(dpp::slashcommand_t event) {
  // Get the coingecko id symbol from the command arguments.
  std::string coingecko_id =
      std::get<std::string>(event.get_parameter("coingecko_id"));

  CURL* curl = curl_easy_init();
  if (!curl) {
    std::cerr << "Error: could not initialize libcurl" << std::endl;
    return;
  }

  // Set the URL of the API to request
  std::string url =
      "https://api.coingecko.com/api/v3/simple/price?ids=" + coingecko_id +
      "&vs_currencies=usd%2Cidr";
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

  // Set the callback function to handle the response data
  std::string response_data;
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

  // Perform the API request
  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    std::cerr << "Error: curl_easy_perform() failed: "
              << curl_easy_strerror(res) << std::endl;
    curl_easy_cleanup(curl);
    return;
  }

  // Parse the JSON response
  json response_json;
  try {
    response_json = json::parse(response_data);
    std::cout << "=============================" << std::endl;
    std::cout << ">> coingecko response: " << response_json << std::endl;

  } catch (const std::exception& e) {
    std::cerr << "Error: failed to parse JSON response: " << e.what()
              << std::endl;
    curl_easy_cleanup(curl);

    // Reply to Discord
    event.reply(coingecko_id +
                ":exclamation: price: error failed to call API data.");
  }

  // Format response value
  try {
    // Format USD
    std::stringstream ss_usd;
    ss_usd.imbue(get_locale("en_US.UTF-8"));
    ss_usd << std::fixed << std::setprecision(2)
            << response_json[coingecko_id]["usd"].get<double>();

    // Format IDR
    std::stringstream ss_idr;
    ss_idr.imbue(get_locale("id_ID.UTF-8"));
    ss_idr << std::fixed << std::setprecision(2)
            << response_json[coingecko_id]["idr"].get<double>();

    // Reply to Discord
    event.reply(":information_source: " + coingecko_id +
                " price: $" +  ss_usd.str() +
                " / Rp." + ss_idr.str());

  } catch (const std::exception& e) {
    std::cerr << "Error: failed to parse currency value: " << e.what()
            << std::endl;
    curl_easy_cleanup(curl);

    // Reply to Discord
    event.reply(coingecko_id +
                ":exclamation: price: formatting error.");
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
