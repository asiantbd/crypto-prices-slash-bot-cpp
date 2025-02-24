# Crypto Price Discord Bot C++ with Slash Command

This is a Discord bot that can be used to query crypto prices using the Coingecko API. It supports slash commands, so you can use it to get the latest prices for any cryptocurrency.

### Requirements
- [DPP - C++ Discord API Bot Library](https://github.com/brainboxdotcc/DPP/)
- [nlohmann_json](https://github.com/nlohmann/json)

### Usage

#### Docker

```
docker run -it --platform=linux/amd64 -e DISCORD_TOKEN=xxxxx ghcr.io/asiantbd/crypto-prices-slash-bot:latest
```

or

```
docker compose -f docker-compose.yml up
```

### Build

#### Build Using External Precompiled Dependency (Recommended)
- Install Precompiled Lib (Debian, Ubuntu, Derivatives), check [this link](https://dpp.dev/installing.html) for other distro.

  DPP:
  ```
  wget -O dpp.deb https://dl.dpp.dev/
  sudo dpkg -i dpp.deb

  ```
  nlohmann_json:
  ```
  sudo apt-get install nlohmann-json3-dev
  ```

- Build (CMake > v3.11)
  ```
  git clone https://github.com/asiantbd/crypto-prices-slash-bot-cpp.git
  cd crypto-prices-slash-bot-cpp
  mkdir build && cd build

  cmake .. -B./
  make
  ```

#### Build Using Embedded Dependency (FetchContent)
- There's two CMake option to turn off the use of external dependency, and will fetch library content then build it on the local computer.
  ```
  USE_EXTERNAL_DPP
  USE_EXTERNAL_JSON
  ```
  Set both or one of the option to OFF (default ON) to fetch and build the library.

- Build (CMake > v3.11)
  ```
  git clone https://github.com/asiantbd/crypto-prices-slash-bot-cpp.git
  cd crypto-prices-slash-bot-cpp
  mkdir build && cd build

  cmake -DUSE_EXTERNAL_DPP=OFF -DUSE_EXTERNAL_JSON=OFF .. -B./
  make
  ```

### Available Commands

The bot supports the following slash commands:

#### Price Query
Get cryptocurrency prices in USD and IDR:
- Using CoinGecko ID: `/price coingecko_id:bitcoin`
- Using ticker symbol: `/price ticker:btc` or `/price ticker:$BTC`

Note: When multiple coins share the same ticker (e.g., "ETH" for both Ethereum and Ethereum Classic),
a dropdown menu will appear allowing you to select the specific coin.


### Additional
- In case you're got error while trying `make` that caused by `curl` try install it first. ex: `sudo apt-get install libcurl4-openssl-dev`

### Running the binary
- Mandatory environment variable for Discord token: `DISCORD_TOKEN`
- Run:
```
DISCORD_TOKEN=xxxx.xxx.xxx ./asiantbd_bot
```
