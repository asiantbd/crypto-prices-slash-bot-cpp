#include <coingecko.h>
#include <dpp/dpp.h>

int main() {
  dpp::cluster bot(std::getenv("DISCORD_TOKEN"));

  bot.on_slashcommand([](auto event) {
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
    return 0;
  });

  bot.on_ready([&bot](auto event) {
    if (dpp::run_once<struct register_bot_commands>()) {
      dpp::slashcommand command_coins;
      command_coins.set_name("coins")
          .set_description("List of available tokens from Coingecko.")
          .set_application_id(bot.me.id);
      bot.global_command_create(command_coins);

      dpp::slashcommand command_price;
      command_price.set_name("price")
          .set_description(
              "Get the price of a crypto token. (Please use /coins to get all "
              "listed tokens.)")
          .set_application_id(bot.me.id)
          .add_option(
              dpp::command_option(dpp::co_string, "coingecko_id",
                                  "(Coingecko) ID of the token: ", true));
      bot.global_command_create(command_price);
    }
  });

  bot.start(dpp::st_wait);
  return 0;
}
