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
	"COORDINATES": "(7,13)",
	"POKEGEAR": "PLAYERS_HOUSE_2F",
	"RADIO_CARD": null,
	"EXPN_CARD": null,
	"MAP_CARD": "PLAYERS_HOUSE_2F",
	"HM01": "PLAYERS_HOUSE_2F",
	"HM02": "PLAYERS_HOUSE_2F",
	"HM03": "PLAYERS_HOUSE_2F",
	"HM04": "PLAYERS_HOUSE_2F",
	"HM05": "PLAYERS_HOUSE_2F",
	"HM06": "PLAYERS_HOUSE_2F",
	"HM07": null,
	"TM08": null,
	"POKEDEX": null,
	"UNOWN_DEX": null,
	"ZEPHYRBADGE": "PLAYERS_HOUSE_2F",
	"HIVEBADGE": "PLAYERS_HOUSE_2F",
	"PLAINBADGE": "PLAYERS_HOUSE_2F",
	"FOGBADGE": "PLAYERS_HOUSE_2F",
	"MINERALBADGE": null,
	"STORMBADGE": "PLAYERS_HOUSE_2F",
	"GLACIERBADGE": "PLAYERS_HOUSE_2F",
	"RISINGBADGE": null,
	"BOULDERBADGE": null,
	"CASCADEBADGE": null,
	"THUNDERBADGE": null,
	"RAINBOWBADGE": null,
	"SOULBADGE": null,
	"MARSHBADGE": null,
	"VOLCANOBADGE": null,
	"EARTHBADGE": null,
	"SQUIRTBOTTLE": "PLAYERS_HOUSE_2F",
	"SECRETPOTION": null,
	"CARD_KEY": "PLAYERS_HOUSE_2F",
	"S_S_TICKET": null,
	"PASS": null,
	"MACHINE_PART": null,
	"CLEAR_BELL": "PLAYERS_HOUSE_2F",
	"RAINBOW_WING": null,
	"SILVER_WING": null,
	"BASEMENT_KEY": "PLAYERS_HOUSE_2F",
	"LOST_ITEM": null,
	"RED_SCALE": "PLAYERS_HOUSE_2F",
	"MYSTERY_EGG": null,
	"BICYCLE": "PLAYERS_HOUSE_2F",
	"BLUE_CARD": null,
	"COIN_CASE": null,
	"ITEMFINDER": null,
	"OLD_ROD": null,
	"GOOD_ROD": null,
	"SUPER_ROD": null,
	"WATER_STONE": null,
	"WARPS": {
		"ELMS_HOUSE": {
			"NEW_BARK_TOWN": "NEW_BARK_TOWN"
		},
		"ELMS_LAB": {
			"NEW_BARK_TOWN": "NEW_BARK_TOWN"
		},
		"NEW_BARK_TOWN": {
			"ELMS_LAB": "ELMS_LAB",
			"PLAYERS_HOUSE_1F": "PLAYERS_HOUSE_1F",
			"PLAYERS_NEIGHBORS_HOUSE": "PLAYERS_NEIGHBORS_HOUSE",
			"ELMS_HOUSE": "ELMS_HOUSE"
		},
		"PLAYERS_HOUSE_1F": {
			"NEW_BARK_TOWN": "NEW_BARK_TOWN",
			"PLAYERS_HOUSE_2F": "PLAYERS_HOUSE_2F"
		},
		"PLAYERS_HOUSE_2F": {
			"PLAYERS_HOUSE_1F": "PLAYERS_HOUSE_1F"
		},
		"PLAYERS_NEIGHBORS_HOUSE": {
			"NEW_BARK_TOWN": "NEW_BARK_TOWN"
		}
	}
}
```

