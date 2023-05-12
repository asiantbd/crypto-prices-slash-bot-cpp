#include <dpp/dpp.h>
#include <cstdlib>
#include <curl/curl.h>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

// This callback function is called by curl_easy_perform() to write the response data into a string
size_t write_callback(char* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append(ptr, size * nmemb);
    return size * nmemb;
}

int main() {
  dpp::cluster bot(std::getenv("DISCORD_TOKEN"));

  bot.on_slashcommand([](auto event) {
    if (event.command.get_command_name() == "ping") {
      event.reply("Pong!");
    }

    if (event.command.get_command_name() == "price") {
      // Get the coingecko id symbol from the command arguments.
      std::string coingecko_id = std::get<std::string>(event.get_parameter("coingecko_id"));

      CURL* curl = curl_easy_init();
      if (!curl) {
          std::cerr << "Error: could not initialize libcurl" << std::endl;
          return 1;
      }

      // Set the URL of the API to request
      std::string url = "https://api.coingecko.com/api/v3/simple/price?ids="+coingecko_id+"&vs_currencies=usd%2Cidr";
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

      // Set the callback function to handle the response data
      std::string response_data;
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

      // Perform the API request
      CURLcode res = curl_easy_perform(curl);
      if (res != CURLE_OK) {
          std::cerr << "Error: curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
          curl_easy_cleanup(curl);
          return 1;
      }

      // Parse the JSON response
      try {
          json response_json = json::parse(response_data);
          std::cout << "=============================" << std::endl;
          std::cout << ">> coingecko response: " << response_json << std::endl;

          // Reply to Discord
          event.reply(":information_source: " + coingecko_id + " price: $" + to_string(response_json[coingecko_id]["usd"]) +" / Rp. " + to_string(response_json[coingecko_id]["idr"]));

      } catch (const std::exception& e) {
          std::cerr << "Error: failed to parse JSON response: " << e.what() << std::endl;
          curl_easy_cleanup(curl);

          // Reply to Discord
          event.reply(coingecko_id + ":exclamation: price: error failed to call API data.");
      }

      curl_easy_cleanup(curl);

    }

    return 0;
  });

  bot.on_ready([&bot](auto event) {
    if (dpp::run_once<struct register_bot_commands>()) {
      dpp::slashcommand command_price;
      command_price.set_name("price")
	            .set_description("Get the price of a crypto token")
	            .set_application_id(bot.me.id)
	            .add_option(
	                dpp::command_option(dpp::co_string, "coingecko_id", "(Coingecko) ID of the token: ", true)
	            );

      bot.global_command_create(command_price);
    }
  });

  bot.start(dpp::st_wait);
  return 0;
}
