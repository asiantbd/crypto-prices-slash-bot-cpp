#include <coingecko.h>
#include <dpp/dpp.h>

int main() {
  dpp::cluster bot(std::getenv("DISCORD_TOKEN"));

  bot.on_slashcommand([](auto event) {
    if (event.command.get_command_name() == "ping") {
      event.reply("Pong!");
    }

    if (event.command.get_command_name() == "price") {
      gecko::fetch(event);
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
              dpp::command_option(dpp::co_string, "coingecko_id",
                                  "(Coingecko) ID of the token: ", true));

      bot.global_command_create(command_price);
    }
  });

  bot.start(dpp::st_wait);
  return 0;
}
