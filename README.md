# Pokémon Crystal Randomizer Auto-Tracker

This program works as a middle layer between the Gameboy emulator Gambatte-speedrun and any Item Randomizer tracker that decides to implement Auto-tracking support.

It works by finding where the Operating System is keeping Gambatte-speedrun's virtual memory space, finding the base address of the Gameboy's WRAM as stored by Gambatte-speedrun itself and then reading the addressses in WRAM periodically in order to find out what has the player already found, then communicating via a WebSocket endpoint available for trackers to request the current game state.

Please note that it is still in its early stages of development, which means that there are still features planned for the future, and bugs might still be present. As of now, only Windows is supported.

## Dependencies

- WebSocket++ (built on top of boost::asio): If using Visual Studio, I recommend [vcpkg](https://vcpkg.io/en/index.html) for managing dependencies.

## Planned future features

- Add *nix support (and modularize the code in order to accomodate for it better);
- Come up with a cleaner way to find the base address of WRAM (since we know that when the game start, we start in our room, near the left-corner of the table, we look for the sequence of bytes that represents the first tiles in memory of the room in order to find the allocated memory space for Gambatte);
- Improve the WebSocket endpoint API to allow for better interaction with trackers as well as proper error-handling.

## How does it work

1. Open gambatte-speedrun;
2. Open the Auto-tracker;
3. The Auto-tracker now needs to find where the Operating System is storing Gambatte-speedrun's memory space, as well as the location of WRAM. In order to do so, the player needs to be at the game start's spawn point:

<p align="center">
  <img 
    src="https://i.imgur.com/ZC4Alu0.png"
  >
</p>

4. Once the allocated space in virtual memory is found, the WebSocket endpoint is opened for requests. Right now, it can be found in ``ws://localhost:9002``.

## Sending requests to the WebSocket endpoint

Right now, only two requests are supported, issued by sending a JSON string:

- ```{"action":"stop"}``` : Closes the web socket. Useful for desktop trackers to be able to dynamically open the auto-tracker and close the WebSocket it opens in a clean way. A 1000 code (normal closure) is sent back to the client.
- ```{"action":"read"}``` : Requests the current state of the game. The response is sent as a JSON string. Example of a response (note that warps not found by the player yet are not included):

```json
{
	"CURRENT MAP": "NEW_BARK_TOWN",
	"COORDINATES": "(13,6)",
	"POKEGEAR": true,
	"RADIO_CARD": true,
	"EXPN_CARD": false,
	"MAP_CARD": false,
	"HM01": true,
	"HM02": true,
	"HM03": true,
	"HM04": true,
	"HM05": true,
	"HM06": true,
	"HM07": false,
	"TM08": true,
	"POKEDEX": true,
	"UNOWN_DEX": false,
	"ZEPHYRBADGE": true,
	"HIVEBADGE": true,
	"PLAINBADGE": true,
	"FOGBADGE": true,
	"MINERALBADGE": false,
	"STORMBADGE": true,
	"GLACIERBADGE": false,
	"RISINGBADGE": false,
	"BOULDERBADGE": false,
	"CASCADEBADGE": false,
	"THUNDERBADGE": false,
	"RAINBOWBADGE": false,
	"SOULBADGE": false,
	"MARSHBADGE": false,
	"VOLCANOBADGE": false,
	"EARTHBADGE": false,
	"SQUIRTBOTTLE": true,
	"SECRETPOTION": false,
	"CARD_KEY": true,
	"S_S_TICKET": false,
	"PASS": false,
	"MACHINE_PART": false,
	"CLEAR_BELL": true,
	"RAINBOW_WING": true,
	"SILVER_WING": false,
	"BASEMENT_KEY": true,
	"LOST_ITEM": false,
	"RED_SCALE": false,
	"MYSTERY_EGG": false,
	"BICYCLE": true,
	"BLUE_CARD": false,
	"COIN_CASE": true,
	"ITEMFINDER": false,
	"OLD_ROD": false,
	"GOOD_ROD": true,
	"SUPER_ROD": false,
	"WATER_STONE": false,
	"WARPS": {
		"NEW_BARK_TOWN": {
			"PLAYERS_HOUSE_1F": "PLAYERS_HOUSE_1F"
		},
		"PLAYERS_HOUSE_1F": {
			"NEW_BARK_TOWN": "NEW_BARK_TOWN",
			"PLAYERS_HOUSE_2F": "PLAYERS_HOUSE_2F"
		},
		"PLAYERS_HOUSE_2F": {
			"PLAYERS_HOUSE_1F": "PLAYERS_HOUSE_1F"
		}
	}
}
```

