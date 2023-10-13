#include <coingecko.h>

using json = nlohmann::json;

// This callback function is called by curl_easy_perform() to write the response
// data into a string
size_t write_callback(char* ptr, size_t size, size_t nmemb, std::string* data) {
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
  try {
    json response_json = json::parse(response_data);
    std::cout << "=============================" << std::endl;
    std::cout << ">> coingecko response: " << response_json << std::endl;

    // Reply to Discord
    event.reply(":information_source: " + coingecko_id + " price: $" +
                to_string(response_json[coingecko_id]["usd"]) + " / Rp. " +
                to_string(response_json[coingecko_id]["idr"]));

  } catch (const std::exception& e) {
    std::cerr << "Error: failed to parse JSON response: " << e.what()
              << std::endl;
    curl_easy_cleanup(curl);

    // Reply to Discord
    event.reply(coingecko_id +
                ":exclamation: price: error failed to call API data.");
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
