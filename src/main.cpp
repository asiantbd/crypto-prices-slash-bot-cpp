#include <coingecko.h>
#include <dpp/dpp.h>

int main() {
    // For slash commands and components, we only need default intents
    dpp::cluster bot(std::getenv("DISCORD_TOKEN"), dpp::i_default_intents);

    bot.on_slashcommand([](const dpp::slashcommand_t& event) {
        std::cout << "Received slash command: " << event.command.get_command_name() << std::endl;

        auto input = event.command.get_command_name();
        if (input == "ping") {
            event.reply("Pong!");
        }
        if (input == "price") {
            gecko::fetch_price(event);
        }
        if (input == "coins") {
            gecko::fetch_tokens(event);
        }
        if (input == "market") {
            gecko::fetch_market_chart(event);
        }
    });

    // Add select menu handler
    bot.on_select_click([](const dpp::select_click_t& event) {
        std::cout << "Received select menu interaction with custom_id: " << event.custom_id << std::endl;

        if (event.custom_id.find("coin_select_") == 0) {
            std::cout << "Processing coin selection..." << std::endl;
            std::string selected_id = event.values[0];
            std::cout << "Selected coin ID: " << selected_id << std::endl;

            // Then fetch and send the price
            try {
                gecko::fetch_single_price(selected_id, event);
            } catch (const std::exception& e) {
                std::cerr << "Error in select menu handler: " << e.what() << std::endl;
                event.edit_response(":exclamation: Error processing selection");
            }
        }
    });

    bot.on_ready([&bot](const dpp::ready_t& event) {
        std::cout << "Bot is ready!" << std::endl;
        if (dpp::run_once<struct register_bot_commands>()) {
            std::cout << "Registering commands..." << std::endl;

            dpp::slashcommand command_coins;
            command_coins.set_name("coins")
                .set_description("List of available tokens from Coingecko.")
                .set_application_id(bot.me.id);
            bot.global_command_create(command_coins);

            dpp::slashcommand command_price;
            command_price.set_name("price")
                .set_description(
                    "Get the price of a crypto token using ID or ticker symbol")
                .set_application_id(bot.me.id)
                .add_option(
                    dpp::command_option(dpp::co_string, "ticker",
                                      "Ticker ($) symbol of the token", false))
                .add_option(
                    dpp::command_option(dpp::co_string, "coingecko_id",
                                      "Coingecko ID of the token", false));

            bot.global_command_create(command_price);

            dpp::slashcommand command_market;
            command_market.set_name("market")
                .set_description(
                    "Get market chart of a crypto token (1 day interval).")
                .set_application_id(bot.me.id)
                .add_option(
                    dpp::command_option(dpp::co_string, "token_id",
                                      "(Coingecko) Id of the token: ", true))
                .add_option(
                    dpp::command_option(dpp::co_string, "currency",
                                      "(Coingecko) Currency for the price: ", true));
            bot.global_command_create(command_market);

            std::cout << "Commands registered successfully!" << std::endl;
        }
    });

    std::cout << "Starting bot..." << std::endl;
    bot.start(dpp::st_wait);
    return 0;
}
