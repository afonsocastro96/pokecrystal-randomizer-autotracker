#include <windows.h>
#include <Psapi.h>
#include <tchar.h>
#include <iostream>

#include "ItemRando.hpp"

std::string boolToJsonBool(bool b) { return b ? "true" : "false"; }

// RandoState Class

uint8_t RandoState::getByte(LPVOID* buffer, uint8_t position) {
	uint8_t i = position / 4;
	uint8_t byte = position % 4;

	switch (byte) {
	case 0:
		return (((uint32_t)buffer[i] & 0x000000ff) >> 0);
	case 1:
		return (((uint32_t)buffer[i] & 0x0000ff00) >> 8);
	case 2:
		return (((uint32_t)buffer[i] & 0x00ff0000) >> 16);
	case 3:
		return (((uint32_t)buffer[i] & 0xff000000) >> 24);
	default:
		throw std::invalid_argument("Warning eater, this should never happen");
	}
}

gambatte_wram_info RandoState::GetBaseAddress(DWORD processId, TCHAR* processName) {
	TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
	uint32_t base_address = NULL;
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
		PROCESS_VM_READ,
		FALSE, processId);

	if (NULL != hProcess) {
		HMODULE hMod;
		DWORD cbNeeded;

		if (EnumProcessModulesEx(hProcess, &hMod, sizeof(hMod),
			&cbNeeded, LIST_MODULES_32BIT | LIST_MODULES_64BIT))
		{
			GetModuleBaseName(hProcess, hMod, szProcessName,
				sizeof(szProcessName) / sizeof(TCHAR));
			if (!_tcsicmp(processName, szProcessName)) {
				std::wcout << "Process: " << processName << " (PID: " << processId << ")" << std::endl;
				std::wcout << "Waiting for the player to be in game start's spawn point..." << std::endl << std::endl;
				uint32_t offset = 0x000004EA;
				LPVOID buffer[64];
				SIZE_T bytes_read;

				while (base_address == 0)
					for (uint32_t p = 0; p < 0x10000000; p += 0x00001000) {
						LPCVOID pointer = (LPCVOID)(p + offset);
						if (ReadProcessMemory(hProcess, pointer, buffer, 32, &bytes_read)) {
							if (buffer[0] == (LPCVOID)0x02020202 && buffer[1] == (LPCVOID)0x02020202 && buffer[2] == (LPCVOID)0x06050202) {
								base_address = p + gambatte_offset;
								std::wcout << "Found base WRAM address - location of $C000: 0x" << (LPCVOID)base_address << std::endl << std::endl;
							}
						}
					}
			}
		}
		CloseHandle(hProcess);
	}
	gambatte_wram_info gwi = { processId, base_address };
	return gwi;
}

void RandoState::updateBadges(HANDLE hProcess) {
	LPCVOID johto_badges_pointer = (LPCVOID)(this->base_address + johto_badges);
	LPVOID buffer[BADGES_NBYTES/4+1];
	SIZE_T bytes_read;

	ReadProcessMemory(hProcess, johto_badges_pointer, buffer, BADGES_NBYTES, &bytes_read);
	uint8_t johto_badges_byte = getByte(buffer, 0);
	uint8_t kanto_badges_byte = getByte(buffer, 1);

	for (uint8_t i = 0; i < 8; ++i) {
		kantoBadges[i] = kanto_badges_byte & (1<<i);
		johtoBadges[i] = johto_badges_byte & (1<<i);
	}

}

void RandoState::updateTMsAndHMs(HANDLE hProcess) {
	LPCVOID tms_hms_pointer = (LPCVOID)(this->base_address + tm_list);
	LPVOID buffer[TMHM_NBYTES/4+1];
	SIZE_T bytes_read;
	ReadProcessMemory(hProcess, tms_hms_pointer, buffer, TMHM_NBYTES, &bytes_read);

	tmsHmsOwned[HM01] = getByte(buffer, HM01_ID);
	tmsHmsOwned[HM02] = getByte(buffer, HM02_ID);
	tmsHmsOwned[HM03] = getByte(buffer, HM03_ID);
	tmsHmsOwned[HM04] = getByte(buffer, HM04_ID);
	tmsHmsOwned[HM05] = getByte(buffer, HM05_ID);
	tmsHmsOwned[HM06] = getByte(buffer, HM06_ID);
	tmsHmsOwned[HM07] = getByte(buffer, HM07_ID);
	tmsHmsOwned[TM08] = getByte(buffer, TM08_ID);
}

void RandoState::updatePokegear(HANDLE hProcess) {
	LPCVOID pokegear_pointer = (LPCVOID)(this->base_address + pokegear_byte);
	LPVOID buffer[POKEGEAR_NBYTES/4+1];
	SIZE_T bytes_read;

	ReadProcessMemory(hProcess, pokegear_pointer, buffer, POKEGEAR_NBYTES, &bytes_read);

	uint8_t pokegear_info = getByte(buffer, 0);
	pokegear[POKEGEAR] = pokegear_info & (1 << POKEGEAR);
	pokegear[MAP_CARD] = pokegear_info & (1 << MAP_CARD);
	pokegear[RADIO_CARD] = pokegear_info & (1 << RADIO_CARD);
	pokegear[EXPN_CARD] = pokegear_info & (1 << EXPN_CARD);
}

void RandoState::updatePokedex(HANDLE hProcess) {
	LPCVOID pokedex_pointer = (LPCVOID)(this->base_address + pokedex_byte);
	LPVOID buffer[POKEDEX_NBYTES/4+1];
	SIZE_T bytes_read;

	ReadProcessMemory(hProcess, pokedex_pointer, buffer, POKEDEX_NBYTES, &bytes_read);

	uint8_t pokedex_info = getByte(buffer, 0);
	hasPokedex = pokedex_info & (1 << POKEDEX);
	hasUnownDex = pokedex_info & (1 << UNOWN_DEX);
}

void RandoState::updateKeyItems(HANDLE hProcess) {
	LPCVOID keyitem_pointer = (LPCVOID)(this->base_address + number_of_key_items);
	LPVOID buffer[KEYITEMS_NBYTES/4+1];
	SIZE_T bytes_read;
	ReadProcessMemory(hProcess, keyitem_pointer, buffer, KEYITEMS_NBYTES, &bytes_read);

	uint8_t nItems = getByte(buffer, 0);
	itemsOwned.resize(nItems);

	for (uint8_t i = 0; i < nItems; ++i) {
		itemsOwned[i] = getByte(buffer, i+1);
	}
}

void RandoState::updateItems(HANDLE hProcess) {
	// We only care about the Water Stone here
	LPCVOID item_pointer = (LPCVOID)(this->base_address + number_of_items);
	LPVOID buffer[ITEMS_NBYTES / 4 + 1];
	SIZE_T bytes_read;
	ReadProcessMemory(hProcess, item_pointer, buffer, ITEMS_NBYTES, &bytes_read);

	uint8_t nItems = getByte(buffer, 0);
	for (int i = 1; i < nItems; i+=2) { // We don't care about quantities
		ITEM item = getByte(buffer, i);
		if (item == WATER_STONE) {
			itemsOwned.push_back(WATER_STONE);
			return;
		}
	}
}

void RandoState::updateMapInfo(HANDLE hProcess) {
	LPCVOID item_pointer = (LPCVOID)(this->base_address + warp_number);
	LPVOID buffer[MAPS_NBYTES / 4 + 1];
	SIZE_T bytes_read;
	ReadProcessMemory(hProcess, item_pointer, buffer, MAPS_NBYTES, &bytes_read);
	
	uint8_t map_bank = getByte(buffer, 1);
	uint8_t map_number = getByte(buffer, 2);
	uint8_t x = getByte(buffer, 3);
	uint8_t y = getByte(buffer, 4);

	Map* currentMap = getMap(map_bank, map_number);


	if (this->currentMap == NULL) {
		this->currentMap = currentMap;
	}
	else {
		if (this->currentMap != currentMap) {
			int8_t warp_id = this->currentMap->getWarpId(this->x, this->y);
			if(warp_id != -1) // Filter out map transitions (ex: New Bark to Route 29)
				if (this->currentMap->getWarps()[warp_id].get_map_destination() == NULL) { // No need to update if we've been here before
					this->currentMap->setWarp(warp_id, currentMap);
					#if DEBUG
					std::cout << "DEBUG: Set Warp #" << warp_id + 1 << " of " << this->currentMap->getName() << " to redirect to " << currentMap->getName() << std::endl;
					#endif // DEBUG
					int8_t new_warp_id = currentMap->getWarpId(x, y);
					this->currentMap->getWarpId(this->x, this->y);
					currentMap->setWarp(new_warp_id, this->currentMap);
					#if DEBUG
					std::cout << "DEBUG: Set Warp #" << new_warp_id + 1 << " of " << currentMap->getName() << " to redirect to " << this->currentMap->getName() << std::endl << std::endl;
					#endif // DEBUG
				}
			this->currentMap = currentMap;
		}
	}

	this->x = x;
	this->y = y;
}

Map* RandoState::getMap(uint8_t map_group, uint8_t map_const) {
	for (uint16_t i = 0; i < this->maps.size(); ++i) {
		if (this->maps[i].getMapGroup() == map_group && this->maps[i].getMapConst() == map_const)
			return &(this->maps[i]);
	}
	throw std::invalid_argument("Unknown map");
}

RandoState::RandoState() {
	DWORD aProcesses[1024];
	DWORD cbNeeded;
	DWORD cProcesses;

	// Get the list of process identifiers.
	EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded);

	// Calculate how many process identifiers were returned.
	cProcesses = cbNeeded / sizeof(DWORD);

	// Check the names of all the processess (Case insensitive)
	gambatte_wram_info gwi = { 0,0 };
	for (uint32_t i = 0; i < cProcesses; i++) {
		TCHAR name[] = TEXT("gambatte_speedrun.exe");
		gambatte_wram_info gwi = GetBaseAddress(aProcesses[i], name);
		if (gwi.base_address != 0) {
			this->processId = gwi.processId;
			this->base_address = gwi.base_address;
			initialiseMaps();
			return;
		}
	}
	throw std::runtime_error("Gambatte-Speedrun must be running before starting the auto-tracker!");
}

void RandoState::updateStatus() {
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
		PROCESS_VM_READ,
		FALSE, this->processId);

	updateBadges(hProcess);
	updatePokedex(hProcess);
	updatePokegear(hProcess);
	updateKeyItems(hProcess);
	updateItems(hProcess);
	updateTMsAndHMs(hProcess);
	updateMapInfo(hProcess);

	CloseHandle(hProcess);
}

std::ostream& operator<<(std::ostream& stream, const RandoState& status) {
	std::stringstream ss;
	ss << "{";
	ss << "\"CURRENT MAP\": \"" << status.currentMap->getName() << "\",";
	ss << "\"COORDINATES\": \"" << "(" << +status.y << "," << +status.x << ")" << "\",";
	ss << "\"POKEGEAR\":" << boolToJsonBool(status.pokegear[POKEGEAR]) << ",";
	ss << "\"RADIO_CARD\":" << boolToJsonBool(status.pokegear[RADIO_CARD]) << ",";
	ss << "\"EXPN_CARD\":" << boolToJsonBool(status.pokegear[EXPN_CARD]) << ",";
	ss << "\"MAP_CARD\":" << boolToJsonBool(status.pokegear[MAP_CARD]) << ",";
	ss << "\"HM01\":" << boolToJsonBool(status.tmsHmsOwned[HM01]) << ",";
	ss << "\"HM02\":" << boolToJsonBool(status.tmsHmsOwned[HM02]) << ",";
	ss << "\"HM03\":" << boolToJsonBool(status.tmsHmsOwned[HM03]) << ",";
	ss << "\"HM04\":" << boolToJsonBool(status.tmsHmsOwned[HM04]) << ",";
	ss << "\"HM05\":" << boolToJsonBool(status.tmsHmsOwned[HM05]) << ",";
	ss << "\"HM06\":" << boolToJsonBool(status.tmsHmsOwned[HM06]) << ",";
	ss << "\"HM07\":" << boolToJsonBool(status.tmsHmsOwned[HM07]) << ",";
	ss << "\"TM08\":" << boolToJsonBool(status.tmsHmsOwned[TM08]) << ",";
	ss << "\"POKEDEX\":" << boolToJsonBool(status.hasPokedex) << ",";
	ss << "\"UNOWN_DEX\":" << boolToJsonBool(status.hasUnownDex) << ",";
	ss << "\"ZEPHYRBADGE\":" << boolToJsonBool(status.johtoBadges[ZEPHYRBADGE]) << ",";
	ss << "\"HIVEBADGE\":" << boolToJsonBool(status.johtoBadges[HIVEBADGE]) << ",";
	ss << "\"PLAINBADGE\":" << boolToJsonBool(status.johtoBadges[PLAINBADGE]) << ",";
	ss << "\"FOGBADGE\":" << boolToJsonBool(status.johtoBadges[FOGBADGE]) << ",";
	ss << "\"MINERALBADGE\":" << boolToJsonBool(status.johtoBadges[MINERALBADGE]) << ",";
	ss << "\"STORMBADGE\":" << boolToJsonBool(status.johtoBadges[STORMBADGE]) << ",";
	ss << "\"GLACIERBADGE\":" << boolToJsonBool(status.johtoBadges[GLACIERBADGE]) << ",";
	ss << "\"RISINGBADGE\":" << boolToJsonBool(status.johtoBadges[RISINGBADGE]) << ",";
	ss << "\"BOULDERBADGE\":" << boolToJsonBool(status.kantoBadges[BOULDERBADGE]) << ",";
	ss << "\"CASCADEBADGE\":" << boolToJsonBool(status.kantoBadges[CASCADEBADGE]) << ",";
	ss << "\"THUNDERBADGE\":" << boolToJsonBool(status.kantoBadges[THUNDERBADGE]) << ",";
	ss << "\"RAINBOWBADGE\":" << boolToJsonBool(status.kantoBadges[RAINBOWBADGE]) << ",";
	ss << "\"SOULBADGE\":" << boolToJsonBool(status.kantoBadges[SOULBADGE]) << ",";
	ss << "\"MARSHBADGE\":" << boolToJsonBool(status.kantoBadges[MARSHBADGE]) << ",";
	ss << "\"VOLCANOBADGE\":" << boolToJsonBool(status.kantoBadges[VOLCANOBADGE]) << ",";
	ss << "\"EARTHBADGE\":" << boolToJsonBool(status.kantoBadges[EARTHBADGE]) << ",";
	ss << "\"SQUIRTBOTTLE\":" << boolToJsonBool(std::find(status.itemsOwned.begin(), status.itemsOwned.end(), SQUIRTBOTTLE) != status.itemsOwned.end()) << ",";
	ss << "\"SECRETPOTION\":" << boolToJsonBool(std::find(status.itemsOwned.begin(), status.itemsOwned.end(), SECRETPOTION) != status.itemsOwned.end()) << ",";
	ss << "\"CARD_KEY\":" << boolToJsonBool(std::find(status.itemsOwned.begin(), status.itemsOwned.end(), CARD_KEY) != status.itemsOwned.end()) << ",";
	ss << "\"S_S_TICKET\":" << boolToJsonBool(std::find(status.itemsOwned.begin(), status.itemsOwned.end(), S_S_TICKET) != status.itemsOwned.end()) << ",";
	ss << "\"PASS\":" << boolToJsonBool(std::find(status.itemsOwned.begin(), status.itemsOwned.end(), PASS) != status.itemsOwned.end()) << ",";
	ss << "\"MACHINE_PART\":" << boolToJsonBool(std::find(status.itemsOwned.begin(), status.itemsOwned.end(), MACHINE_PART) != status.itemsOwned.end()) << ",";
	ss << "\"CLEAR_BELL\":" << boolToJsonBool(std::find(status.itemsOwned.begin(), status.itemsOwned.end(), CLEAR_BELL) != status.itemsOwned.end()) << ",";
	ss << "\"RAINBOW_WING\":" << boolToJsonBool(std::find(status.itemsOwned.begin(), status.itemsOwned.end(), RAINBOW_WING) != status.itemsOwned.end()) << ",";
	ss << "\"SILVER_WING\":" << boolToJsonBool(std::find(status.itemsOwned.begin(), status.itemsOwned.end(), SILVER_WING) != status.itemsOwned.end()) << ",";
	ss << "\"BASEMENT_KEY\":" << boolToJsonBool(std::find(status.itemsOwned.begin(), status.itemsOwned.end(), BASEMENT_KEY) != status.itemsOwned.end()) << ",";
	ss << "\"LOST_ITEM\":" << boolToJsonBool(std::find(status.itemsOwned.begin(), status.itemsOwned.end(), LOST_ITEM) != status.itemsOwned.end()) << ",";
	ss << "\"RED_SCALE\":" << boolToJsonBool(std::find(status.itemsOwned.begin(), status.itemsOwned.end(), RED_SCALE) != status.itemsOwned.end()) << ",";
	ss << "\"MYSTERY_EGG\":" << boolToJsonBool(std::find(status.itemsOwned.begin(), status.itemsOwned.end(), MYSTERY_EGG) != status.itemsOwned.end()) << ",";
	ss << "\"BICYCLE\":" << boolToJsonBool(std::find(status.itemsOwned.begin(), status.itemsOwned.end(), BICYCLE) != status.itemsOwned.end()) << ",";
	ss << "\"BLUE_CARD\":" << boolToJsonBool(std::find(status.itemsOwned.begin(), status.itemsOwned.end(), BLUE_CARD) != status.itemsOwned.end()) << ",";
	ss << "\"COIN_CASE\":" << boolToJsonBool(std::find(status.itemsOwned.begin(), status.itemsOwned.end(), COIN_CASE) != status.itemsOwned.end()) << ",";
	ss << "\"ITEMFINDER\":" << boolToJsonBool(std::find(status.itemsOwned.begin(), status.itemsOwned.end(), ITEMFINDER) != status.itemsOwned.end()) << ",";
	ss << "\"OLD_ROD\":" << boolToJsonBool(std::find(status.itemsOwned.begin(), status.itemsOwned.end(), OLD_ROD) != status.itemsOwned.end()) << ",";
	ss << "\"GOOD_ROD\":" << boolToJsonBool(std::find(status.itemsOwned.begin(), status.itemsOwned.end(), GOOD_ROD) != status.itemsOwned.end()) << ",";
	ss << "\"SUPER_ROD\":" << boolToJsonBool(std::find(status.itemsOwned.begin(), status.itemsOwned.end(), SUPER_ROD) != status.itemsOwned.end()) << ",";
	ss << "\"WATER_STONE\":" << boolToJsonBool(std::find(status.itemsOwned.begin(), status.itemsOwned.end(), WATER_STONE) != status.itemsOwned.end()) << ",";
	ss << "\"WARPS\":{";

	std::stringstream warps_ss;
	
	for (uint16_t i = 0; i < status.maps.size(); ++i) {
		Map map = status.maps[i];
		if (map.getWarps().size() != 0) {
			bool has_found_warps = false;
			for (uint8_t j = 0; j < map.getWarps().size(); ++j) {
				if (map.getWarps()[j].get_map_destination() != NULL) {
					has_found_warps = true;
					break;
				}
			}
			if (has_found_warps) {
				std::vector<std::string> connections;
				std::stringstream warp_ss;
				if (warps_ss.rdbuf()->in_avail() != 0) warps_ss << ",";
				warps_ss << "\"" << map.getName() << "\":{";
				for (uint8_t j = 0; j < map.getWarps().size(); ++j) {
					if (std::find(connections.begin(), connections.end(), map.getWarps()[j].get_vanilla_destination()) != connections.end()) continue;
					connections.push_back(map.getWarps()[j].get_vanilla_destination());
					if (warp_ss.rdbuf()->in_avail() != 0) warp_ss << ",";
					if (map.getWarps()[j].get_map_destination() != NULL)
						warp_ss << "\"" << map.getWarps()[j].get_vanilla_destination() << "\":\"" << map.getWarps()[j].get_map_destination()->getName() << "\"";
				}
				warps_ss << warp_ss.str();
				warps_ss << "}";
				connections.clear();
			}
		}
	}
	ss << warps_ss.str();
	ss << "}}";
	stream << ss.str();
	return stream;
}

// Map Class

Map::Map(uint8_t map_group, uint8_t map_const, std::string name) {
	this->map_group = map_group;
	this->map_const = map_const;
	this->name = name;
}

int8_t Map::getWarpId(uint8_t x, uint8_t y) {
	for (uint8_t i = 0; i < this->warps.size(); ++i) {
		if (this->warps[i].getX() == x && this->warps[i].getY() == y)
			return i;
	}
	return -1;
}

void Map::setWarp(uint8_t warp, Map* destination) {
	std::string map = this->warps[warp].get_vanilla_destination();

	for (uint8_t i = 0; i < this->warps.size(); ++i) {
		if(this->warps[i].get_vanilla_destination() == map)
			this->warps[i].add_map_destination(destination);
	}
}

// Warp Class

Warp::Warp(uint8_t y, uint8_t x, std::string vanilla_destination, uint8_t warp_destination) {
	this->y = y;
	this->x = x;
	this->vanilla_destination = vanilla_destination;
	this->warp_destination_id = warp_destination;
}


// Initialise all the maps and warps

void RandoState::initialiseMaps() {
	Map map = Map(8, 5, "AZALEA_GYM");
	map.addWarp(Warp(4, 15, "AZALEA_TOWN", 5));
	map.addWarp(Warp(5, 15, "AZALEA_TOWN", 5));
	maps.push_back(map);

	map = Map(8, 3, "AZALEA_MART");
	map.addWarp(Warp(2, 7, "AZALEA_TOWN", 3));
	map.addWarp(Warp(3, 7, "AZALEA_TOWN", 3));
	maps.push_back(map);

	map = Map(8, 1, "AZALEA_POKECENTER_1F");
	map.addWarp(Warp(3, 7, "AZALEA_TOWN", 1));
	map.addWarp(Warp(4, 7, "AZALEA_TOWN", 1));
	map.addWarp(Warp(0, 7, "POKECENTER_2F", 1));
	maps.push_back(map);

	map = Map(8, 7, "AZALEA_TOWN");
	map.addWarp(Warp(15, 9, "AZALEA_POKECENTER_1F", 1));
	map.addWarp(Warp(21, 13, "CHARCOAL_KILN", 1));
	map.addWarp(Warp(21, 5, "AZALEA_MART", 2));
	map.addWarp(Warp(9, 5, "KURTS_HOUSE", 1));
	map.addWarp(Warp(10, 15, "AZALEA_GYM", 1));
	map.addWarp(Warp(31, 7, "SLOWPOKE_WELL_B1F", 1));
	map.addWarp(Warp(2, 10, "ILEX_FOREST_AZALEA_GATE", 3));
	map.addWarp(Warp(2, 11, "ILEX_FOREST_AZALEA_GATE", 4));
	maps.push_back(map);

	map = Map(22, 11, "BATTLE_TOWER_1F");
	map.addWarp(Warp(7, 9, "BATTLE_TOWER_OUTSIDE", 3));
	map.addWarp(Warp(8, 9, "BATTLE_TOWER_OUTSIDE", 4));
	map.addWarp(Warp(7, 0, "BATTLE_TOWER_ELEVATOR", 1));
	maps.push_back(map);

	map = Map(22, 12, "BATTLE_TOWER_BATTLE_ROOM");
	map.addWarp(Warp(3, 7, "BATTLE_TOWER_HALLWAY", 4));
	map.addWarp(Warp(4, 7, "BATTLE_TOWER_HALLWAY", 4));
	maps.push_back(map);

	map = Map(22, 13, "BATTLE_TOWER_ELEVATOR");
	map.addWarp(Warp(1, 3, "BATTLE_TOWER_HALLWAY", 1));
	map.addWarp(Warp(2, 3, "BATTLE_TOWER_HALLWAY", 1));
	maps.push_back(map);

	map = Map(22, 14, "BATTLE_TOWER_HALLWAY");
	map.addWarp(Warp(11, 1, "BATTLE_TOWER_ELEVATOR", 1));
	map.addWarp(Warp(5, 0, "BATTLE_TOWER_BATTLE_ROOM", 1));
	map.addWarp(Warp(7, 0, "BATTLE_TOWER_BATTLE_ROOM", 1));
	map.addWarp(Warp(9, 0, "BATTLE_TOWER_BATTLE_ROOM", 1));
	map.addWarp(Warp(13, 0, "BATTLE_TOWER_BATTLE_ROOM", 1));
	map.addWarp(Warp(15, 0, "BATTLE_TOWER_BATTLE_ROOM", 1));
	maps.push_back(map);

	map = Map(22, 16, "BATTLE_TOWER_OUTSIDE");
	map.addWarp(Warp(8, 21, "ROUTE_40_BATTLE_TOWER_GATE", 3));
	map.addWarp(Warp(9, 21, "ROUTE_40_BATTLE_TOWER_GATE", 4));
	map.addWarp(Warp(8, 9, "BATTLE_TOWER_1F", 1));
	map.addWarp(Warp(9, 9, "BATTLE_TOWER_1F", 2));
	maps.push_back(map);

	map = Map(17, 9, "BILLS_BROTHERS_HOUSE");
	map.addWarp(Warp(2, 7, "FUCHSIA_CITY", 4));
	map.addWarp(Warp(3, 7, "FUCHSIA_CITY", 4));
	maps.push_back(map);

	map = Map(11, 6, "BILLS_FAMILYS_HOUSE");
	map.addWarp(Warp(2, 7, "GOLDENROD_CITY", 4));
	map.addWarp(Warp(3, 7, "GOLDENROD_CITY", 4));
	maps.push_back(map);

	map = Map(7, 11, "BILLS_HOUSE");
	map.addWarp(Warp(2, 7, "ROUTE_25", 1));
	map.addWarp(Warp(3, 7, "ROUTE_25", 1));
	maps.push_back(map);

	map = Map(5, 10, "BLACKTHORN_CITY");
	map.addWarp(Warp(18, 11, "BLACKTHORN_GYM_1F", 1));
	map.addWarp(Warp(13, 21, "BLACKTHORN_DRAGON_SPEECH_HOUSE", 1));
	map.addWarp(Warp(29, 23, "BLACKTHORN_EMYS_HOUSE", 1));
	map.addWarp(Warp(15, 29, "BLACKTHORN_MART", 2));
	map.addWarp(Warp(21, 29, "BLACKTHORN_POKECENTER_1F", 1));
	map.addWarp(Warp(9, 31, "MOVE_DELETERS_HOUSE", 1));
	map.addWarp(Warp(36, 9, "ICE_PATH_1F", 2));
	map.addWarp(Warp(20, 1, "DRAGONS_DEN_1F", 1));
	maps.push_back(map);

	map = Map(5, 3, "BLACKTHORN_DRAGON_SPEECH_HOUSE");
	map.addWarp(Warp(2, 7, "BLACKTHORN_CITY", 2));
	map.addWarp(Warp(3, 7, "BLACKTHORN_CITY", 2));
	maps.push_back(map);

	map = Map(5, 4, "BLACKTHORN_EMYS_HOUSE");
	map.addWarp(Warp(2, 7, "BLACKTHORN_CITY", 3));
	map.addWarp(Warp(3, 7, "BLACKTHORN_CITY", 3));
	maps.push_back(map);

	map = Map(5, 1, "BLACKTHORN_GYM_1F");
	map.addWarp(Warp(4, 17, "BLACKTHORN_CITY", 1));
	map.addWarp(Warp(5, 17, "BLACKTHORN_CITY", 1));
	map.addWarp(Warp(1, 7, "BLACKTHORN_GYM_2F", 1));
	map.addWarp(Warp(7, 9, "BLACKTHORN_GYM_2F", 2));
	map.addWarp(Warp(2, 6, "BLACKTHORN_GYM_2F", 3));
	map.addWarp(Warp(7, 7, "BLACKTHORN_GYM_2F", 4));
	map.addWarp(Warp(7, 6, "BLACKTHORN_GYM_2F", 5));
	maps.push_back(map);

	map = Map(5, 2, "BLACKTHORN_GYM_2F");
	map.addWarp(Warp(1, 7, "BLACKTHORN_GYM_1F", 3));
	map.addWarp(Warp(7, 9, "BLACKTHORN_GYM_1F", 4));
	map.addWarp(Warp(2, 5, "BLACKTHORN_GYM_1F", 5));
	map.addWarp(Warp(8, 7, "BLACKTHORN_GYM_1F", 6));
	map.addWarp(Warp(8, 3, "BLACKTHORN_GYM_1F", 7));
	maps.push_back(map);

	map = Map(5, 5, "BLACKTHORN_MART");
	map.addWarp(Warp(2, 7, "BLACKTHORN_CITY", 4));
	map.addWarp(Warp(3, 7, "BLACKTHORN_CITY", 4));
	maps.push_back(map);

	map = Map(5, 6, "BLACKTHORN_POKECENTER_1F");
	map.addWarp(Warp(3, 7, "BLACKTHORN_CITY", 5));
	map.addWarp(Warp(4, 7, "BLACKTHORN_CITY", 5));
	map.addWarp(Warp(0, 7, "POKECENTER_2F", 1));
	maps.push_back(map);

	map = Map(13, 5, "BLUES_HOUSE");
	map.addWarp(Warp(2, 7, "PALLET_TOWN", 2));
	map.addWarp(Warp(3, 7, "PALLET_TOWN", 2));
	maps.push_back(map);

	map = Map(16, 5, "BRUNOS_ROOM");
	map.addWarp(Warp(4, 17, "KOGAS_ROOM", 3));
	map.addWarp(Warp(5, 17, "KOGAS_ROOM", 4));
	map.addWarp(Warp(4, 2, "KARENS_ROOM", 1));
	map.addWarp(Warp(5, 2, "KARENS_ROOM", 2));
	maps.push_back(map);

	map = Map(3, 13, "BURNED_TOWER_1F");
	map.addWarp(Warp(9, 15, "ECRUTEAK_CITY", 13));
	map.addWarp(Warp(10, 15, "ECRUTEAK_CITY", 13));
	map.addWarp(Warp(10, 9, "BURNED_TOWER_B1F", 1));
	map.addWarp(Warp(5, 5, "BURNED_TOWER_B1F", 1));
	map.addWarp(Warp(5, 6, "BURNED_TOWER_B1F", 1));
	map.addWarp(Warp(4, 6, "BURNED_TOWER_B1F", 1));
	map.addWarp(Warp(15, 4, "BURNED_TOWER_B1F", 2));
	map.addWarp(Warp(15, 5, "BURNED_TOWER_B1F", 2));
	map.addWarp(Warp(10, 7, "BURNED_TOWER_B1F", 3));
	map.addWarp(Warp(5, 14, "BURNED_TOWER_B1F", 4));
	map.addWarp(Warp(4, 14, "BURNED_TOWER_B1F", 4));
	map.addWarp(Warp(14, 14, "BURNED_TOWER_B1F", 5));
	map.addWarp(Warp(15, 14, "BURNED_TOWER_B1F", 5));
	map.addWarp(Warp(7, 15, "BURNED_TOWER_B1F", 6));
	maps.push_back(map);

	map = Map(3, 14, "BURNED_TOWER_B1F");
	map.addWarp(Warp(10, 9, "BURNED_TOWER_1F", 3));
	map.addWarp(Warp(17, 7, "BURNED_TOWER_1F", 7));
	map.addWarp(Warp(10, 8, "BURNED_TOWER_1F", 9));
	map.addWarp(Warp(3, 13, "BURNED_TOWER_1F", 10));
	map.addWarp(Warp(17, 14, "BURNED_TOWER_1F", 12));
	map.addWarp(Warp(7, 15, "BURNED_TOWER_1F", 14));
	maps.push_back(map);

	map = Map(21, 22, "CELADON_CAFE");
	map.addWarp(Warp(6, 7, "CELADON_CITY", 9));
	map.addWarp(Warp(7, 7, "CELADON_CITY", 9));
	maps.push_back(map);

	map = Map(21, 4, "CELADON_CITY");
	map.addWarp(Warp(4, 9, "CELADON_DEPT_STORE_1F", 1));
	map.addWarp(Warp(16, 9, "CELADON_MANSION_1F", 1));
	map.addWarp(Warp(16, 3, "CELADON_MANSION_1F", 3));
	map.addWarp(Warp(17, 3, "CELADON_MANSION_1F", 3));
	map.addWarp(Warp(29, 9, "CELADON_POKECENTER_1F", 1));
	map.addWarp(Warp(18, 19, "CELADON_GAME_CORNER", 1));
	map.addWarp(Warp(23, 19, "CELADON_GAME_CORNER_PRIZE_ROOM", 1));
	map.addWarp(Warp(10, 29, "CELADON_GYM", 1));
	map.addWarp(Warp(25, 29, "CELADON_CAFE", 1));
	maps.push_back(map);

	map = Map(21, 5, "CELADON_DEPT_STORE_1F");
	map.addWarp(Warp(7, 7, "CELADON_CITY", 1));
	map.addWarp(Warp(8, 7, "CELADON_CITY", 1));
	map.addWarp(Warp(15, 0, "CELADON_DEPT_STORE_2F", 2));
	map.addWarp(Warp(2, 0, "CELADON_DEPT_STORE_ELEVATOR", 1));
	maps.push_back(map);

	map = Map(21, 6, "CELADON_DEPT_STORE_2F");
	map.addWarp(Warp(12, 0, "CELADON_DEPT_STORE_3F", 1));
	map.addWarp(Warp(15, 0, "CELADON_DEPT_STORE_1F", 3));
	map.addWarp(Warp(2, 0, "CELADON_DEPT_STORE_ELEVATOR", 1));
	maps.push_back(map);

	map = Map(21, 7, "CELADON_DEPT_STORE_3F");
	map.addWarp(Warp(12, 0, "CELADON_DEPT_STORE_2F", 1));
	map.addWarp(Warp(15, 0, "CELADON_DEPT_STORE_4F", 2));
	map.addWarp(Warp(2, 0, "CELADON_DEPT_STORE_ELEVATOR", 1));
	maps.push_back(map);

	map = Map(21, 8, "CELADON_DEPT_STORE_4F");
	map.addWarp(Warp(12, 0, "CELADON_DEPT_STORE_5F", 1));
	map.addWarp(Warp(15, 0, "CELADON_DEPT_STORE_3F", 2));
	map.addWarp(Warp(2, 0, "CELADON_DEPT_STORE_ELEVATOR", 1));
	maps.push_back(map);

	map = Map(21, 9, "CELADON_DEPT_STORE_5F");
	map.addWarp(Warp(12, 0, "CELADON_DEPT_STORE_4F", 1));
	map.addWarp(Warp(15, 0, "CELADON_DEPT_STORE_6F", 1));
	map.addWarp(Warp(2, 0, "CELADON_DEPT_STORE_ELEVATOR", 1));
	maps.push_back(map);

	map = Map(21, 10, "CELADON_DEPT_STORE_6F");
	map.addWarp(Warp(15, 0, "CELADON_DEPT_STORE_5F", 2));
	map.addWarp(Warp(2, 0, "CELADON_DEPT_STORE_ELEVATOR", 1));
	maps.push_back(map);

	map = Map(21, 11, "CELADON_DEPT_STORE_ELEVATOR");
	map.addWarp(Warp(1, 3, "CELADON_DEPT_STORE_1F", -1));
	map.addWarp(Warp(2, 3, "CELADON_DEPT_STORE_1F", -1));
	maps.push_back(map);

	map = Map(21, 19, "CELADON_GAME_CORNER");
	map.addWarp(Warp(14, 13, "CELADON_CITY", 6));
	map.addWarp(Warp(15, 13, "CELADON_CITY", 6));
	maps.push_back(map);

	map = Map(21, 20, "CELADON_GAME_CORNER_PRIZE_ROOM");
	map.addWarp(Warp(2, 5, "CELADON_CITY", 7));
	map.addWarp(Warp(3, 5, "CELADON_CITY", 7));
	maps.push_back(map);

	map = Map(21, 21, "CELADON_GYM");
	map.addWarp(Warp(4, 17, "CELADON_CITY", 8));
	map.addWarp(Warp(5, 17, "CELADON_CITY", 8));
	maps.push_back(map);

	map = Map(21, 12, "CELADON_MANSION_1F");
	map.addWarp(Warp(6, 9, "CELADON_CITY", 2));
	map.addWarp(Warp(7, 9, "CELADON_CITY", 2));
	map.addWarp(Warp(3, 0, "CELADON_CITY", 3));
	map.addWarp(Warp(0, 0, "CELADON_MANSION_2F", 1));
	map.addWarp(Warp(7, 0, "CELADON_MANSION_2F", 4));
	maps.push_back(map);

	map = Map(21, 13, "CELADON_MANSION_2F");
	map.addWarp(Warp(0, 0, "CELADON_MANSION_1F", 4));
	map.addWarp(Warp(1, 0, "CELADON_MANSION_3F", 2));
	map.addWarp(Warp(6, 0, "CELADON_MANSION_3F", 3));
	map.addWarp(Warp(7, 0, "CELADON_MANSION_1F", 5));
	maps.push_back(map);

	map = Map(21, 14, "CELADON_MANSION_3F");
	map.addWarp(Warp(0, 0, "CELADON_MANSION_ROOF", 1));
	map.addWarp(Warp(1, 0, "CELADON_MANSION_2F", 2));
	map.addWarp(Warp(6, 0, "CELADON_MANSION_2F", 3));
	map.addWarp(Warp(7, 0, "CELADON_MANSION_ROOF", 2));
	maps.push_back(map);

	map = Map(21, 15, "CELADON_MANSION_ROOF");
	map.addWarp(Warp(1, 1, "CELADON_MANSION_3F", 1));
	map.addWarp(Warp(6, 1, "CELADON_MANSION_3F", 4));
	map.addWarp(Warp(2, 5, "CELADON_MANSION_ROOF_HOUSE", 1));
	maps.push_back(map);

	map = Map(21, 16, "CELADON_MANSION_ROOF_HOUSE");
	map.addWarp(Warp(2, 7, "CELADON_MANSION_ROOF", 3));
	map.addWarp(Warp(3, 7, "CELADON_MANSION_ROOF", 3));
	maps.push_back(map);

	map = Map(21, 17, "CELADON_POKECENTER_1F");
	map.addWarp(Warp(3, 7, "CELADON_CITY", 5));
	map.addWarp(Warp(4, 7, "CELADON_CITY", 5));
	map.addWarp(Warp(0, 7, "POKECENTER_2F", 1));
	maps.push_back(map);

	map = Map(21, 18, "CELADON_POKECENTER_2F_BETA");
	map.addWarp(Warp(0, 7, "CELADON_POKECENTER_1F", 3));
	maps.push_back(map);

	map = Map(7, 17, "CERULEAN_CITY");
	map.addWarp(Warp(7, 15, "CERULEAN_GYM_BADGE_SPEECH_HOUSE", 1));
	map.addWarp(Warp(28, 17, "CERULEAN_POLICE_STATION", 1));
	map.addWarp(Warp(13, 19, "CERULEAN_TRADE_SPEECH_HOUSE", 1));
	map.addWarp(Warp(19, 21, "CERULEAN_POKECENTER_1F", 1));
	map.addWarp(Warp(30, 23, "CERULEAN_GYM", 1));
	map.addWarp(Warp(25, 29, "CERULEAN_MART", 2));
	maps.push_back(map);

	map = Map(7, 6, "CERULEAN_GYM");
	map.addWarp(Warp(4, 15, "CERULEAN_CITY", 5));
	map.addWarp(Warp(5, 15, "CERULEAN_CITY", 5));
	maps.push_back(map);

	map = Map(7, 1, "CERULEAN_GYM_BADGE_SPEECH_HOUSE");
	map.addWarp(Warp(2, 7, "CERULEAN_CITY", 1));
	map.addWarp(Warp(3, 7, "CERULEAN_CITY", 1));
	maps.push_back(map);

	map = Map(7, 7, "CERULEAN_MART");
	map.addWarp(Warp(2, 7, "CERULEAN_CITY", 6));
	map.addWarp(Warp(3, 7, "CERULEAN_CITY", 6));
	maps.push_back(map);

	map = Map(7, 4, "CERULEAN_POKECENTER_1F");
	map.addWarp(Warp(3, 7, "CERULEAN_CITY", 4));
	map.addWarp(Warp(4, 7, "CERULEAN_CITY", 4));
	map.addWarp(Warp(0, 7, "POKECENTER_2F", 1));
	maps.push_back(map);

	map = Map(7, 5, "CERULEAN_POKECENTER_2F_BETA");
	map.addWarp(Warp(0, 7, "CERULEAN_POKECENTER_1F", 1));
	maps.push_back(map);

	map = Map(7, 2, "CERULEAN_POLICE_STATION");
	map.addWarp(Warp(2, 7, "CERULEAN_CITY", 2));
	map.addWarp(Warp(3, 7, "CERULEAN_CITY", 2));
	maps.push_back(map);

	map = Map(7, 3, "CERULEAN_TRADE_SPEECH_HOUSE");
	map.addWarp(Warp(2, 7, "CERULEAN_CITY", 3));
	map.addWarp(Warp(3, 7, "CERULEAN_CITY", 3));
	maps.push_back(map);

	map = Map(8, 2, "CHARCOAL_KILN");
	map.addWarp(Warp(2, 7, "AZALEA_TOWN", 2));
	map.addWarp(Warp(3, 7, "AZALEA_TOWN", 2));
	maps.push_back(map);

	map = Map(26, 3, "CHERRYGROVE_CITY");
	map.addWarp(Warp(23, 3, "CHERRYGROVE_MART", 2));
	map.addWarp(Warp(29, 3, "CHERRYGROVE_POKECENTER_1F", 1));
	map.addWarp(Warp(17, 7, "CHERRYGROVE_GYM_SPEECH_HOUSE", 1));
	map.addWarp(Warp(25, 9, "GUIDE_GENTS_HOUSE", 1));
	map.addWarp(Warp(31, 11, "CHERRYGROVE_EVOLUTION_SPEECH_HOUSE", 1));
	maps.push_back(map);

	map = Map(26, 8, "CHERRYGROVE_EVOLUTION_SPEECH_HOUSE");
	map.addWarp(Warp(2, 7, "CHERRYGROVE_CITY", 5));
	map.addWarp(Warp(3, 7, "CHERRYGROVE_CITY", 5));
	maps.push_back(map);

	map = Map(26, 6, "CHERRYGROVE_GYM_SPEECH_HOUSE");
	map.addWarp(Warp(2, 7, "CHERRYGROVE_CITY", 3));
	map.addWarp(Warp(3, 7, "CHERRYGROVE_CITY", 3));
	maps.push_back(map);

	map = Map(26, 4, "CHERRYGROVE_MART");
	map.addWarp(Warp(2, 7, "CHERRYGROVE_CITY", 1));
	map.addWarp(Warp(3, 7, "CHERRYGROVE_CITY", 1));
	maps.push_back(map);

	map = Map(26, 5, "CHERRYGROVE_POKECENTER_1F");
	map.addWarp(Warp(3, 7, "CHERRYGROVE_CITY", 2));
	map.addWarp(Warp(4, 7, "CHERRYGROVE_CITY", 2));
	map.addWarp(Warp(0, 7, "POKECENTER_2F", 1));
	maps.push_back(map);

	map = Map(22, 3, "CIANWOOD_CITY");
	map.addWarp(Warp(17, 41, "MANIAS_HOUSE", 1));
	map.addWarp(Warp(8, 43, "CIANWOOD_GYM", 1));
	map.addWarp(Warp(23, 43, "CIANWOOD_POKECENTER_1F", 1));
	map.addWarp(Warp(15, 47, "CIANWOOD_PHARMACY", 1));
	map.addWarp(Warp(9, 31, "CIANWOOD_PHOTO_STUDIO", 1));
	map.addWarp(Warp(15, 37, "CIANWOOD_LUGIA_SPEECH_HOUSE", 1));
	map.addWarp(Warp(5, 17, "POKE_SEERS_HOUSE", 1));
	maps.push_back(map);

	map = Map(22, 5, "CIANWOOD_GYM");
	map.addWarp(Warp(4, 17, "CIANWOOD_CITY", 2));
	map.addWarp(Warp(5, 17, "CIANWOOD_CITY", 2));
	maps.push_back(map);

	map = Map(22, 9, "CIANWOOD_LUGIA_SPEECH_HOUSE");
	map.addWarp(Warp(2, 7, "CIANWOOD_CITY", 6));
	map.addWarp(Warp(3, 7, "CIANWOOD_CITY", 6));
	maps.push_back(map);

	map = Map(22, 7, "CIANWOOD_PHARMACY");
	map.addWarp(Warp(2, 7, "CIANWOOD_CITY", 4));
	map.addWarp(Warp(3, 7, "CIANWOOD_CITY", 4));
	maps.push_back(map);

	map = Map(22, 8, "CIANWOOD_PHOTO_STUDIO");
	map.addWarp(Warp(2, 7, "CIANWOOD_CITY", 5));
	map.addWarp(Warp(3, 7, "CIANWOOD_CITY", 5));
	maps.push_back(map);

	map = Map(22, 6, "CIANWOOD_POKECENTER_1F");
	map.addWarp(Warp(3, 7, "CIANWOOD_CITY", 3));
	map.addWarp(Warp(4, 7, "CIANWOOD_CITY", 3));
	map.addWarp(Warp(0, 7, "POKECENTER_2F", 1));
	maps.push_back(map);

	map = Map(6, 8, "CINNABAR_ISLAND");
	map.addWarp(Warp(11, 11, "CINNABAR_POKECENTER_1F", 1));
	maps.push_back(map);

	map = Map(6, 1, "CINNABAR_POKECENTER_1F");
	map.addWarp(Warp(3, 7, "CINNABAR_ISLAND", 1));
	map.addWarp(Warp(4, 7, "CINNABAR_ISLAND", 1));
	map.addWarp(Warp(0, 7, "POKECENTER_2F", 1));
	maps.push_back(map);

	map = Map(6, 2, "CINNABAR_POKECENTER_2F_BETA");
	map.addWarp(Warp(0, 7, "CINNABAR_POKECENTER_1F", 3));
	maps.push_back(map);

	map = Map(20, 3, "COLOSSEUM");
	map.addWarp(Warp(4, 7, "POKECENTER_2F", 3));
	map.addWarp(Warp(5, 7, "POKECENTER_2F", 3));
	maps.push_back(map);

	map = Map(25, 11, "COPYCATS_HOUSE_1F");
	map.addWarp(Warp(2, 7, "SAFFRON_CITY", 8));
	map.addWarp(Warp(3, 7, "SAFFRON_CITY", 8));
	map.addWarp(Warp(2, 0, "COPYCATS_HOUSE_2F", 1));
	maps.push_back(map);

	map = Map(25, 12, "COPYCATS_HOUSE_2F");
	map.addWarp(Warp(3, 0, "COPYCATS_HOUSE_1F", 3));
	maps.push_back(map);

	map = Map(4, 5, "DANCE_THEATRE");
	map.addWarp(Warp(5, 13, "ECRUTEAK_CITY", 8));
	map.addWarp(Warp(6, 13, "ECRUTEAK_CITY", 8));
	maps.push_back(map);

	map = Map(3, 79, "DARK_CAVE_BLACKTHORN_ENTRANCE");
	map.addWarp(Warp(23, 3, "ROUTE_45", 1));
	map.addWarp(Warp(3, 25, "DARK_CAVE_VIOLET_ENTRANCE", 2));
	maps.push_back(map);

	map = Map(3, 78, "DARK_CAVE_VIOLET_ENTRANCE");
	map.addWarp(Warp(3, 15, "ROUTE_31", 3));
	map.addWarp(Warp(17, 1, "DARK_CAVE_BLACKTHORN_ENTRANCE", 2));
	map.addWarp(Warp(35, 33, "ROUTE_46", 3));
	maps.push_back(map);

	map = Map(11, 24, "DAY_CARE");
	map.addWarp(Warp(0, 5, "ROUTE_34", 3));
	map.addWarp(Warp(0, 6, "ROUTE_34", 4));
	map.addWarp(Warp(2, 7, "ROUTE_34", 5));
	map.addWarp(Warp(3, 7, "ROUTE_34", 5));
	maps.push_back(map);

	map = Map(24, 11, "DAY_OF_WEEK_SIBLINGS_HOUSE");
	map.addWarp(Warp(2, 7, "ROUTE_26", 3));
	map.addWarp(Warp(3, 7, "ROUTE_26", 3));
	maps.push_back(map);

	map = Map(3, 84, "DIGLETTS_CAVE");
	map.addWarp(Warp(3, 33, "VERMILION_CITY", 10));
	map.addWarp(Warp(5, 31, "DIGLETTS_CAVE", 5));
	map.addWarp(Warp(15, 5, "ROUTE_2", 5));
	map.addWarp(Warp(17, 3, "DIGLETTS_CAVE", 6));
	map.addWarp(Warp(17, 33, "DIGLETTS_CAVE", 2));
	map.addWarp(Warp(3, 3, "DIGLETTS_CAVE", 4));
	maps.push_back(map);

	map = Map(3, 80, "DRAGONS_DEN_1F");
	map.addWarp(Warp(3, 5, "BLACKTHORN_CITY", 8));
	map.addWarp(Warp(3, 3, "DRAGONS_DEN_1F", 4));
	map.addWarp(Warp(5, 15, "DRAGONS_DEN_B1F", 1));
	map.addWarp(Warp(5, 13, "DRAGONS_DEN_1F", 2));
	maps.push_back(map);

	map = Map(3, 81, "DRAGONS_DEN_B1F");
	map.addWarp(Warp(20, 3, "DRAGONS_DEN_1F", 3));
	map.addWarp(Warp(19, 29, "DRAGON_SHRINE", 1));
	maps.push_back(map);

	map = Map(3, 82, "DRAGON_SHRINE");
	map.addWarp(Warp(4, 9, "DRAGONS_DEN_B1F", 2));
	map.addWarp(Warp(5, 9, "DRAGONS_DEN_B1F", 2));
	maps.push_back(map);

	map = Map(10, 8, "EARLS_POKEMON_ACADEMY");
	map.addWarp(Warp(3, 15, "VIOLET_CITY", 3));
	map.addWarp(Warp(4, 15, "VIOLET_CITY", 3));
	maps.push_back(map);

	map = Map(4, 9, "ECRUTEAK_CITY");
	map.addWarp(Warp(35, 26, "ROUTE_42_ECRUTEAK_GATE", 1));
	map.addWarp(Warp(35, 27, "ROUTE_42_ECRUTEAK_GATE", 2));
	map.addWarp(Warp(18, 11, "ECRUTEAK_TIN_TOWER_ENTRANCE", 1));
	map.addWarp(Warp(20, 2, "WISE_TRIOS_ROOM", 1));
	map.addWarp(Warp(20, 3, "WISE_TRIOS_ROOM", 2));
	map.addWarp(Warp(23, 27, "ECRUTEAK_POKECENTER_1F", 1));
	map.addWarp(Warp(5, 21, "ECRUTEAK_LUGIA_SPEECH_HOUSE", 1));
	map.addWarp(Warp(23, 21, "DANCE_THEATRE", 1));
	map.addWarp(Warp(29, 21, "ECRUTEAK_MART", 2));
	map.addWarp(Warp(6, 27, "ECRUTEAK_GYM", 1));
	map.addWarp(Warp(13, 27, "ECRUTEAK_ITEMFINDER_HOUSE", 1));
	map.addWarp(Warp(37, 7, "TIN_TOWER_1F", 1));
	map.addWarp(Warp(5, 5, "BURNED_TOWER_1F", 1));
	map.addWarp(Warp(0, 18, "ROUTE_38_ECRUTEAK_GATE", 3));
	map.addWarp(Warp(0, 19, "ROUTE_38_ECRUTEAK_GATE", 4));
	maps.push_back(map);

	map = Map(4, 7, "ECRUTEAK_GYM");
	map.addWarp(Warp(4, 17, "ECRUTEAK_CITY", 10));
	map.addWarp(Warp(5, 17, "ECRUTEAK_CITY", 10));
	map.addWarp(Warp(4, 14, "ECRUTEAK_GYM", 4));
	map.addWarp(Warp(2, 4, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(3, 4, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(4, 4, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(4, 5, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(6, 7, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(7, 4, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(2, 6, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(3, 6, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(4, 6, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(5, 6, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(7, 6, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(7, 7, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(4, 8, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(5, 8, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(6, 8, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(7, 8, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(2, 8, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(2, 9, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(2, 10, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(2, 11, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(4, 10, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(5, 10, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(2, 12, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(3, 12, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(4, 12, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(5, 12, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(7, 10, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(7, 11, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(7, 12, "ECRUTEAK_GYM", 3));
	map.addWarp(Warp(7, 13, "ECRUTEAK_GYM", 3));
	maps.push_back(map);

	map = Map(4, 8, "ECRUTEAK_ITEMFINDER_HOUSE");
	map.addWarp(Warp(3, 7, "ECRUTEAK_CITY", 11));
	map.addWarp(Warp(4, 7, "ECRUTEAK_CITY", 11));
	maps.push_back(map);

	map = Map(4, 4, "ECRUTEAK_LUGIA_SPEECH_HOUSE");
	map.addWarp(Warp(3, 7, "ECRUTEAK_CITY", 7));
	map.addWarp(Warp(4, 7, "ECRUTEAK_CITY", 7));
	maps.push_back(map);

	map = Map(4, 6, "ECRUTEAK_MART");
	map.addWarp(Warp(2, 7, "ECRUTEAK_CITY", 9));
	map.addWarp(Warp(3, 7, "ECRUTEAK_CITY", 9));
	maps.push_back(map);

	map = Map(4, 3, "ECRUTEAK_POKECENTER_1F");
	map.addWarp(Warp(3, 7, "ECRUTEAK_CITY", 6));
	map.addWarp(Warp(4, 7, "ECRUTEAK_CITY", 6));
	map.addWarp(Warp(0, 7, "POKECENTER_2F", 1));
	maps.push_back(map);

	map = Map(4, 1, "ECRUTEAK_TIN_TOWER_ENTRANCE");
	map.addWarp(Warp(4, 17, "ECRUTEAK_CITY", 3));
	map.addWarp(Warp(5, 17, "ECRUTEAK_CITY", 3));
	map.addWarp(Warp(5, 3, "ECRUTEAK_TIN_TOWER_ENTRANCE", 4));
	map.addWarp(Warp(17, 15, "ECRUTEAK_TIN_TOWER_ENTRANCE", 3));
	map.addWarp(Warp(17, 3, "WISE_TRIOS_ROOM", 3));
	maps.push_back(map);

	map = Map(24, 9, "ELMS_HOUSE");
	map.addWarp(Warp(2, 7, "NEW_BARK_TOWN", 4));
	map.addWarp(Warp(3, 7, "NEW_BARK_TOWN", 4));
	maps.push_back(map);

	map = Map(24, 5, "ELMS_LAB");
	map.addWarp(Warp(4, 11, "NEW_BARK_TOWN", 1));
	map.addWarp(Warp(5, 11, "NEW_BARK_TOWN", 1));
	maps.push_back(map);

	map = Map(15, 3, "FAST_SHIP_1F");
	map.addWarp(Warp(25, 1, "FAST_SHIP_1F", -1));
	map.addWarp(Warp(27, 8, "FAST_SHIP_CABINS_NNW_NNE_NE", 1));
	map.addWarp(Warp(23, 8, "FAST_SHIP_CABINS_NNW_NNE_NE", 2));
	map.addWarp(Warp(19, 8, "FAST_SHIP_CABINS_NNW_NNE_NE", 3));
	map.addWarp(Warp(15, 8, "FAST_SHIP_CABINS_SW_SSW_NW", 1));
	map.addWarp(Warp(15, 15, "FAST_SHIP_CABINS_SW_SSW_NW", 2));
	map.addWarp(Warp(19, 15, "FAST_SHIP_CABINS_SW_SSW_NW", 4));
	map.addWarp(Warp(23, 15, "FAST_SHIP_CABINS_SE_SSE_CAPTAINS_CABIN", 1));
	map.addWarp(Warp(27, 15, "FAST_SHIP_CABINS_SE_SSE_CAPTAINS_CABIN", 3));
	map.addWarp(Warp(3, 13, "FAST_SHIP_CABINS_SE_SSE_CAPTAINS_CABIN", 5));
	map.addWarp(Warp(6, 12, "FAST_SHIP_B1F", 1));
	map.addWarp(Warp(30, 14, "FAST_SHIP_B1F", 2));
	maps.push_back(map);

	map = Map(15, 7, "FAST_SHIP_B1F");
	map.addWarp(Warp(5, 11, "FAST_SHIP_1F", 11));
	map.addWarp(Warp(31, 13, "FAST_SHIP_1F", 12));
	maps.push_back(map);

	map = Map(15, 4, "FAST_SHIP_CABINS_NNW_NNE_NE");
	map.addWarp(Warp(2, 0, "FAST_SHIP_1F", 2));
	map.addWarp(Warp(2, 12, "FAST_SHIP_1F", 3));
	map.addWarp(Warp(2, 24, "FAST_SHIP_1F", 4));
	maps.push_back(map);

	map = Map(15, 6, "FAST_SHIP_CABINS_SE_SSE_CAPTAINS_CABIN");
	map.addWarp(Warp(2, 7, "FAST_SHIP_1F", 8));
	map.addWarp(Warp(3, 7, "FAST_SHIP_1F", 8));
	map.addWarp(Warp(2, 19, "FAST_SHIP_1F", 9));
	map.addWarp(Warp(3, 19, "FAST_SHIP_1F", 9));
	map.addWarp(Warp(2, 33, "FAST_SHIP_1F", 10));
	map.addWarp(Warp(3, 33, "FAST_SHIP_1F", 10));
	maps.push_back(map);

	map = Map(15, 5, "FAST_SHIP_CABINS_SW_SSW_NW");
	map.addWarp(Warp(2, 0, "FAST_SHIP_1F", 5));
	map.addWarp(Warp(2, 19, "FAST_SHIP_1F", 6));
	map.addWarp(Warp(3, 19, "FAST_SHIP_1F", 6));
	map.addWarp(Warp(2, 31, "FAST_SHIP_1F", 7));
	map.addWarp(Warp(3, 31, "FAST_SHIP_1F", 7));
	maps.push_back(map);

	map = Map(25, 3, "FIGHTING_DOJO");
	map.addWarp(Warp(4, 11, "SAFFRON_CITY", 1));
	map.addWarp(Warp(5, 11, "SAFFRON_CITY", 1));
	maps.push_back(map);

	map = Map(17, 5, "FUCHSIA_CITY");
	map.addWarp(Warp(5, 13, "FUCHSIA_MART", 2));
	map.addWarp(Warp(22, 13, "SAFARI_ZONE_MAIN_OFFICE", 1));
	map.addWarp(Warp(8, 27, "FUCHSIA_GYM", 1));
	map.addWarp(Warp(11, 27, "BILLS_BROTHERS_HOUSE", 1));
	map.addWarp(Warp(19, 27, "FUCHSIA_POKECENTER_1F", 1));
	map.addWarp(Warp(27, 27, "SAFARI_ZONE_WARDENS_HOME", 1));
	map.addWarp(Warp(18, 3, "SAFARI_ZONE_FUCHSIA_GATE_BETA", 3));
	map.addWarp(Warp(37, 22, "ROUTE_15_FUCHSIA_GATE", 1));
	map.addWarp(Warp(37, 23, "ROUTE_15_FUCHSIA_GATE", 2));
	map.addWarp(Warp(7, 35, "ROUTE_19_FUCHSIA_GATE", 1));
	map.addWarp(Warp(8, 35, "ROUTE_19_FUCHSIA_GATE", 2));
	maps.push_back(map);

	map = Map(17, 8, "FUCHSIA_GYM");
	map.addWarp(Warp(4, 17, "FUCHSIA_CITY", 3));
	map.addWarp(Warp(5, 17, "FUCHSIA_CITY", 3));
	maps.push_back(map);

	map = Map(17, 6, "FUCHSIA_MART");
	map.addWarp(Warp(2, 7, "FUCHSIA_CITY", 1));
	map.addWarp(Warp(3, 7, "FUCHSIA_CITY", 1));
	maps.push_back(map);

	map = Map(17, 10, "FUCHSIA_POKECENTER_1F");
	map.addWarp(Warp(3, 7, "FUCHSIA_CITY", 5));
	map.addWarp(Warp(4, 7, "FUCHSIA_CITY", 5));
	map.addWarp(Warp(0, 7, "POKECENTER_2F", 1));
	maps.push_back(map);

	map = Map(17, 11, "FUCHSIA_POKECENTER_2F_BETA");
	map.addWarp(Warp(0, 7, "FUCHSIA_POKECENTER_1F", 3));
	maps.push_back(map);

	map = Map(11, 4, "GOLDENROD_BIKE_SHOP");
	map.addWarp(Warp(2, 7, "GOLDENROD_CITY", 2));
	map.addWarp(Warp(3, 7, "GOLDENROD_CITY", 2));
	maps.push_back(map);

	map = Map(11, 2, "GOLDENROD_CITY");
	map.addWarp(Warp(24, 7, "GOLDENROD_GYM", 1));
	map.addWarp(Warp(29, 29, "GOLDENROD_BIKE_SHOP", 1));
	map.addWarp(Warp(31, 21, "GOLDENROD_HAPPINESS_RATER", 1));
	map.addWarp(Warp(5, 25, "BILLS_FAMILYS_HOUSE", 1));
	map.addWarp(Warp(9, 13, "GOLDENROD_MAGNET_TRAIN_STATION", 2));
	map.addWarp(Warp(29, 5, "GOLDENROD_FLOWER_SHOP", 1));
	map.addWarp(Warp(33, 9, "GOLDENROD_PP_SPEECH_HOUSE", 1));
	map.addWarp(Warp(15, 7, "GOLDENROD_NAME_RATER", 1));
	map.addWarp(Warp(24, 27, "GOLDENROD_DEPT_STORE_1F", 1));
	map.addWarp(Warp(14, 21, "GOLDENROD_GAME_CORNER", 1));
	map.addWarp(Warp(5, 15, "RADIO_TOWER_1F", 1));
	map.addWarp(Warp(19, 1, "ROUTE_35_GOLDENROD_GATE", 3));
	map.addWarp(Warp(9, 5, "GOLDENROD_UNDERGROUND_SWITCH_ROOM_ENTRANCES", 8));
	map.addWarp(Warp(11, 29, "GOLDENROD_UNDERGROUND_SWITCH_ROOM_ENTRANCES", 5));
	map.addWarp(Warp(15, 27, "GOLDENROD_POKECENTER_1F", 1));
	maps.push_back(map);

	map = Map(11, 11, "GOLDENROD_DEPT_STORE_1F");
	map.addWarp(Warp(7, 7, "GOLDENROD_CITY", 9));
	map.addWarp(Warp(8, 7, "GOLDENROD_CITY", 9));
	map.addWarp(Warp(15, 0, "GOLDENROD_DEPT_STORE_2F", 2));
	map.addWarp(Warp(2, 0, "GOLDENROD_DEPT_STORE_ELEVATOR", 1));
	maps.push_back(map);

	map = Map(11, 12, "GOLDENROD_DEPT_STORE_2F");
	map.addWarp(Warp(12, 0, "GOLDENROD_DEPT_STORE_3F", 1));
	map.addWarp(Warp(15, 0, "GOLDENROD_DEPT_STORE_1F", 3));
	map.addWarp(Warp(2, 0, "GOLDENROD_DEPT_STORE_ELEVATOR", 1));
	maps.push_back(map);

	map = Map(11, 13, "GOLDENROD_DEPT_STORE_3F");
	map.addWarp(Warp(12, 0, "GOLDENROD_DEPT_STORE_2F", 1));
	map.addWarp(Warp(15, 0, "GOLDENROD_DEPT_STORE_4F", 2));
	map.addWarp(Warp(2, 0, "GOLDENROD_DEPT_STORE_ELEVATOR", 1));
	maps.push_back(map);

	map = Map(11, 14, "GOLDENROD_DEPT_STORE_4F");
	map.addWarp(Warp(12, 0, "GOLDENROD_DEPT_STORE_5F", 1));
	map.addWarp(Warp(15, 0, "GOLDENROD_DEPT_STORE_3F", 2));
	map.addWarp(Warp(2, 0, "GOLDENROD_DEPT_STORE_ELEVATOR", 1));
	maps.push_back(map);

	map = Map(11, 15, "GOLDENROD_DEPT_STORE_5F");
	map.addWarp(Warp(12, 0, "GOLDENROD_DEPT_STORE_4F", 1));
	map.addWarp(Warp(15, 0, "GOLDENROD_DEPT_STORE_6F", 1));
	map.addWarp(Warp(2, 0, "GOLDENROD_DEPT_STORE_ELEVATOR", 1));
	maps.push_back(map);

	map = Map(11, 16, "GOLDENROD_DEPT_STORE_6F");
	map.addWarp(Warp(15, 0, "GOLDENROD_DEPT_STORE_5F", 2));
	map.addWarp(Warp(2, 0, "GOLDENROD_DEPT_STORE_ELEVATOR", 1));
	map.addWarp(Warp(13, 0, "GOLDENROD_DEPT_STORE_ROOF", 1));
	maps.push_back(map);

	map = Map(3, 55, "GOLDENROD_DEPT_STORE_B1F");
	map.addWarp(Warp(17, 2, "GOLDENROD_UNDERGROUND_WAREHOUSE", 3));
	map.addWarp(Warp(9, 4, "GOLDENROD_DEPT_STORE_ELEVATOR", 1));
	map.addWarp(Warp(10, 4, "GOLDENROD_DEPT_STORE_ELEVATOR", 2));
	maps.push_back(map);

	map = Map(11, 17, "GOLDENROD_DEPT_STORE_ELEVATOR");
	map.addWarp(Warp(1, 3, "GOLDENROD_DEPT_STORE_1F", -1));
	map.addWarp(Warp(2, 3, "GOLDENROD_DEPT_STORE_1F", -1));
	maps.push_back(map);

	map = Map(11, 18, "GOLDENROD_DEPT_STORE_ROOF");
	map.addWarp(Warp(13, 1, "GOLDENROD_DEPT_STORE_6F", 3));
	maps.push_back(map);

	map = Map(11, 8, "GOLDENROD_FLOWER_SHOP");
	map.addWarp(Warp(2, 7, "GOLDENROD_CITY", 6));
	map.addWarp(Warp(3, 7, "GOLDENROD_CITY", 6));
	maps.push_back(map);

	map = Map(11, 19, "GOLDENROD_GAME_CORNER");
	map.addWarp(Warp(2, 13, "GOLDENROD_CITY", 10));
	map.addWarp(Warp(3, 13, "GOLDENROD_CITY", 10));
	maps.push_back(map);

	map = Map(11, 3, "GOLDENROD_GYM");
	map.addWarp(Warp(2, 17, "GOLDENROD_CITY", 1));
	map.addWarp(Warp(3, 17, "GOLDENROD_CITY", 1));
	maps.push_back(map);

	map = Map(11, 5, "GOLDENROD_HAPPINESS_RATER");
	map.addWarp(Warp(2, 7, "GOLDENROD_CITY", 3));
	map.addWarp(Warp(3, 7, "GOLDENROD_CITY", 3));
	maps.push_back(map);

	map = Map(11, 7, "GOLDENROD_MAGNET_TRAIN_STATION");
	map.addWarp(Warp(8, 17, "GOLDENROD_CITY", 5));
	map.addWarp(Warp(9, 17, "GOLDENROD_CITY", 5));
	map.addWarp(Warp(6, 5, "SAFFRON_MAGNET_TRAIN_STATION", 4));
	map.addWarp(Warp(11, 5, "SAFFRON_MAGNET_TRAIN_STATION", 3));
	maps.push_back(map);

	map = Map(11, 10, "GOLDENROD_NAME_RATER");
	map.addWarp(Warp(2, 7, "GOLDENROD_CITY", 8));
	map.addWarp(Warp(3, 7, "GOLDENROD_CITY", 8));
	maps.push_back(map);

	map = Map(11, 20, "GOLDENROD_POKECENTER_1F");
	map.addWarp(Warp(3, 7, "GOLDENROD_CITY", 15));
	map.addWarp(Warp(4, 7, "GOLDENROD_CITY", 15));
	map.addWarp(Warp(0, 6, "POKECOM_CENTER_ADMIN_OFFICE_MOBILE", 1));
	map.addWarp(Warp(0, 7, "POKECENTER_2F", 1));
	maps.push_back(map);

	map = Map(11, 9, "GOLDENROD_PP_SPEECH_HOUSE");
	map.addWarp(Warp(2, 7, "GOLDENROD_CITY", 7));
	map.addWarp(Warp(3, 7, "GOLDENROD_CITY", 7));
	maps.push_back(map);

	map = Map(3, 53, "GOLDENROD_UNDERGROUND");
	map.addWarp(Warp(3, 2, "GOLDENROD_UNDERGROUND_SWITCH_ROOM_ENTRANCES", 7));
	map.addWarp(Warp(3, 34, "GOLDENROD_UNDERGROUND_SWITCH_ROOM_ENTRANCES", 4));
	map.addWarp(Warp(18, 6, "GOLDENROD_UNDERGROUND", 4));
	map.addWarp(Warp(21, 31, "GOLDENROD_UNDERGROUND", 3));
	map.addWarp(Warp(22, 31, "GOLDENROD_UNDERGROUND", 3));
	map.addWarp(Warp(22, 27, "GOLDENROD_UNDERGROUND_SWITCH_ROOM_ENTRANCES", 1));
	maps.push_back(map);

	map = Map(3, 54, "GOLDENROD_UNDERGROUND_SWITCH_ROOM_ENTRANCES");
	map.addWarp(Warp(23, 3, "GOLDENROD_UNDERGROUND", 6));
	map.addWarp(Warp(22, 10, "GOLDENROD_UNDERGROUND_WAREHOUSE", 1));
	map.addWarp(Warp(23, 10, "GOLDENROD_UNDERGROUND_WAREHOUSE", 2));
	map.addWarp(Warp(5, 25, "GOLDENROD_UNDERGROUND", 2));
	map.addWarp(Warp(4, 29, "GOLDENROD_CITY", 14));
	map.addWarp(Warp(5, 29, "GOLDENROD_CITY", 14));
	map.addWarp(Warp(21, 25, "GOLDENROD_UNDERGROUND", 1));
	map.addWarp(Warp(20, 29, "GOLDENROD_CITY", 13));
	map.addWarp(Warp(21, 29, "GOLDENROD_CITY", 13));
	maps.push_back(map);

	map = Map(3, 56, "GOLDENROD_UNDERGROUND_WAREHOUSE");
	map.addWarp(Warp(2, 12, "GOLDENROD_UNDERGROUND_SWITCH_ROOM_ENTRANCES", 2));
	map.addWarp(Warp(3, 12, "GOLDENROD_UNDERGROUND_SWITCH_ROOM_ENTRANCES", 3));
	map.addWarp(Warp(17, 2, "GOLDENROD_DEPT_STORE_B1F", 1));
	maps.push_back(map);

	map = Map(26, 7, "GUIDE_GENTS_HOUSE");
	map.addWarp(Warp(2, 7, "CHERRYGROVE_CITY", 4));
	map.addWarp(Warp(3, 7, "CHERRYGROVE_CITY", 4));
	maps.push_back(map);

	map = Map(16, 8, "HALL_OF_FAME");
	map.addWarp(Warp(4, 13, "LANCES_ROOM", 3));
	map.addWarp(Warp(5, 13, "LANCES_ROOM", 4));
	maps.push_back(map);

	map = Map(3, 61, "ICE_PATH_1F");
	map.addWarp(Warp(4, 19, "ROUTE_44", 1));
	map.addWarp(Warp(36, 27, "BLACKTHORN_CITY", 7));
	map.addWarp(Warp(37, 5, "ICE_PATH_B1F", 1));
	map.addWarp(Warp(37, 13, "ICE_PATH_B1F", 7));
	maps.push_back(map);

	map = Map(3, 62, "ICE_PATH_B1F");
	map.addWarp(Warp(3, 15, "ICE_PATH_1F", 3));
	map.addWarp(Warp(17, 3, "ICE_PATH_B2F_MAHOGANY_SIDE", 1));
	map.addWarp(Warp(11, 2, "ICE_PATH_B2F_MAHOGANY_SIDE", 3));
	map.addWarp(Warp(4, 7, "ICE_PATH_B2F_MAHOGANY_SIDE", 4));
	map.addWarp(Warp(5, 12, "ICE_PATH_B2F_MAHOGANY_SIDE", 5));
	map.addWarp(Warp(12, 13, "ICE_PATH_B2F_MAHOGANY_SIDE", 6));
	map.addWarp(Warp(5, 25, "ICE_PATH_1F", 4));
	map.addWarp(Warp(11, 27, "ICE_PATH_B2F_BLACKTHORN_SIDE", 1));
	maps.push_back(map);

	map = Map(3, 64, "ICE_PATH_B2F_BLACKTHORN_SIDE");
	map.addWarp(Warp(3, 15, "ICE_PATH_B1F", 8));
	map.addWarp(Warp(3, 3, "ICE_PATH_B3F", 2));
	maps.push_back(map);

	map = Map(3, 63, "ICE_PATH_B2F_MAHOGANY_SIDE");
	map.addWarp(Warp(17, 1, "ICE_PATH_B1F", 2));
	map.addWarp(Warp(9, 11, "ICE_PATH_B3F", 1));
	map.addWarp(Warp(11, 4, "ICE_PATH_B1F", 3));
	map.addWarp(Warp(4, 6, "ICE_PATH_B1F", 4));
	map.addWarp(Warp(4, 12, "ICE_PATH_B1F", 5));
	map.addWarp(Warp(12, 12, "ICE_PATH_B1F", 6));
	maps.push_back(map);

	map = Map(3, 65, "ICE_PATH_B3F");
	map.addWarp(Warp(3, 5, "ICE_PATH_B2F_MAHOGANY_SIDE", 2));
	map.addWarp(Warp(15, 5, "ICE_PATH_B2F_BLACKTHORN_SIDE", 2));
	maps.push_back(map);

	map = Map(3, 52, "ILEX_FOREST");
	map.addWarp(Warp(1, 5, "ROUTE_34_ILEX_FOREST_GATE", 3));
	map.addWarp(Warp(3, 42, "ILEX_FOREST_AZALEA_GATE", 1));
	map.addWarp(Warp(3, 43, "ILEX_FOREST_AZALEA_GATE", 2));
	maps.push_back(map);

	map = Map(11, 22, "ILEX_FOREST_AZALEA_GATE");
	map.addWarp(Warp(0, 4, "ILEX_FOREST", 2));
	map.addWarp(Warp(0, 5, "ILEX_FOREST", 3));
	map.addWarp(Warp(9, 4, "AZALEA_TOWN", 7));
	map.addWarp(Warp(9, 5, "AZALEA_TOWN", 8));
	maps.push_back(map);

	map = Map(16, 2, "INDIGO_PLATEAU_POKECENTER_1F");
	map.addWarp(Warp(5, 13, "ROUTE_23", 1));
	map.addWarp(Warp(6, 13, "ROUTE_23", 2));
	map.addWarp(Warp(0, 13, "POKECENTER_2F", 1));
	map.addWarp(Warp(14, 3, "WILLS_ROOM", 1));
	maps.push_back(map);

	map = Map(16, 6, "KARENS_ROOM");
	map.addWarp(Warp(4, 17, "BRUNOS_ROOM", 3));
	map.addWarp(Warp(5, 17, "BRUNOS_ROOM", 4));
	map.addWarp(Warp(4, 2, "LANCES_ROOM", 1));
	map.addWarp(Warp(5, 2, "LANCES_ROOM", 2));
	maps.push_back(map);

	map = Map(16, 4, "KOGAS_ROOM");
	map.addWarp(Warp(4, 17, "WILLS_ROOM", 2));
	map.addWarp(Warp(5, 17, "WILLS_ROOM", 3));
	map.addWarp(Warp(4, 2, "BRUNOS_ROOM", 1));
	map.addWarp(Warp(5, 2, "BRUNOS_ROOM", 2));
	maps.push_back(map);

	map = Map(8, 4, "KURTS_HOUSE");
	map.addWarp(Warp(3, 7, "AZALEA_TOWN", 4));
	map.addWarp(Warp(4, 7, "AZALEA_TOWN", 4));
	maps.push_back(map);

	map = Map(9, 6, "LAKE_OF_RAGE");
	map.addWarp(Warp(7, 3, "LAKE_OF_RAGE_HIDDEN_POWER_HOUSE", 1));
	map.addWarp(Warp(27, 31, "LAKE_OF_RAGE_MAGIKARP_HOUSE", 1));
	maps.push_back(map);

	map = Map(9, 1, "LAKE_OF_RAGE_HIDDEN_POWER_HOUSE");
	map.addWarp(Warp(2, 7, "LAKE_OF_RAGE", 1));
	map.addWarp(Warp(3, 7, "LAKE_OF_RAGE", 1));
	maps.push_back(map);

	map = Map(9, 2, "LAKE_OF_RAGE_MAGIKARP_HOUSE");
	map.addWarp(Warp(2, 7, "LAKE_OF_RAGE", 2));
	map.addWarp(Warp(3, 7, "LAKE_OF_RAGE", 2));
	maps.push_back(map);

	map = Map(16, 7, "LANCES_ROOM");
	map.addWarp(Warp(4, 23, "KARENS_ROOM", 3));
	map.addWarp(Warp(5, 23, "KARENS_ROOM", 4));
	map.addWarp(Warp(4, 1, "HALL_OF_FAME", 1));
	map.addWarp(Warp(5, 1, "HALL_OF_FAME", 2));
	maps.push_back(map);

	map = Map(18, 10, "LAVENDER_MART");
	map.addWarp(Warp(2, 7, "LAVENDER_TOWN", 5));
	map.addWarp(Warp(3, 7, "LAVENDER_TOWN", 5));
	maps.push_back(map);

	map = Map(18, 9, "LAVENDER_NAME_RATER");
	map.addWarp(Warp(2, 7, "LAVENDER_TOWN", 4));
	map.addWarp(Warp(3, 7, "LAVENDER_TOWN", 4));
	maps.push_back(map);

	map = Map(18, 5, "LAVENDER_POKECENTER_1F");
	map.addWarp(Warp(3, 7, "LAVENDER_TOWN", 1));
	map.addWarp(Warp(4, 7, "LAVENDER_TOWN", 1));
	map.addWarp(Warp(0, 7, "POKECENTER_2F", 1));
	maps.push_back(map);

	map = Map(18, 6, "LAVENDER_POKECENTER_2F_BETA");
	map.addWarp(Warp(0, 7, "LAVENDER_POKECENTER_1F", 3));
	maps.push_back(map);

	map = Map(18, 8, "LAVENDER_SPEECH_HOUSE");
	map.addWarp(Warp(2, 7, "LAVENDER_TOWN", 3));
	map.addWarp(Warp(3, 7, "LAVENDER_TOWN", 3));
	maps.push_back(map);

	map = Map(18, 4, "LAVENDER_TOWN");
	map.addWarp(Warp(5, 5, "LAVENDER_POKECENTER_1F", 1));
	map.addWarp(Warp(5, 9, "MR_FUJIS_HOUSE", 1));
	map.addWarp(Warp(3, 13, "LAVENDER_SPEECH_HOUSE", 1));
	map.addWarp(Warp(7, 13, "LAVENDER_NAME_RATER", 1));
	map.addWarp(Warp(1, 5, "LAVENDER_MART", 2));
	map.addWarp(Warp(13, 11, "SOUL_HOUSE", 1));
	map.addWarp(Warp(14, 5, "LAV_RADIO_TOWER_1F", 1));
	maps.push_back(map);

	map = Map(18, 12, "LAV_RADIO_TOWER_1F");
	map.addWarp(Warp(2, 7, "LAVENDER_TOWN", 7));
	map.addWarp(Warp(3, 7, "LAVENDER_TOWN", 7));
	maps.push_back(map);

	map = Map(2, 2, "MAHOGANY_GYM");
	map.addWarp(Warp(4, 17, "MAHOGANY_TOWN", 3));
	map.addWarp(Warp(5, 17, "MAHOGANY_TOWN", 3));
	maps.push_back(map);

	map = Map(3, 48, "MAHOGANY_MART_1F");
	map.addWarp(Warp(3, 7, "MAHOGANY_TOWN", 1));
	map.addWarp(Warp(4, 7, "MAHOGANY_TOWN", 1));
	map.addWarp(Warp(7, 3, "TEAM_ROCKET_BASE_B1F", 1));
	maps.push_back(map);

	map = Map(2, 3, "MAHOGANY_POKECENTER_1F");
	map.addWarp(Warp(3, 7, "MAHOGANY_TOWN", 4));
	map.addWarp(Warp(4, 7, "MAHOGANY_TOWN", 4));
	map.addWarp(Warp(0, 7, "POKECENTER_2F", 1));
	maps.push_back(map);

	map = Map(2, 1, "MAHOGANY_RED_GYARADOS_SPEECH_HOUSE");
	map.addWarp(Warp(2, 7, "MAHOGANY_TOWN", 2));
	map.addWarp(Warp(3, 7, "MAHOGANY_TOWN", 2));
	maps.push_back(map);

	map = Map(2, 7, "MAHOGANY_TOWN");
	map.addWarp(Warp(11, 7, "MAHOGANY_MART_1F", 1));
	map.addWarp(Warp(17, 7, "MAHOGANY_RED_GYARADOS_SPEECH_HOUSE", 1));
	map.addWarp(Warp(6, 13, "MAHOGANY_GYM", 1));
	map.addWarp(Warp(15, 13, "MAHOGANY_POKECENTER_1F", 1));
	map.addWarp(Warp(9, 1, "ROUTE_43_MAHOGANY_GATE", 3));
	maps.push_back(map);

	map = Map(22, 4, "MANIAS_HOUSE");
	map.addWarp(Warp(2, 7, "CIANWOOD_CITY", 1));
	map.addWarp(Warp(3, 7, "CIANWOOD_CITY", 1));
	maps.push_back(map);

	map = Map(20, 6, "MOBILE_BATTLE_ROOM");
	map.addWarp(Warp(4, 7, "POKECENTER_2F", 6));
	map.addWarp(Warp(5, 7, "POKECENTER_2F", 6));
	maps.push_back(map);

	map = Map(20, 5, "MOBILE_TRADE_ROOM");
	map.addWarp(Warp(4, 7, "POKECENTER_2F", 5));
	map.addWarp(Warp(5, 7, "POKECENTER_2F", 5));
	maps.push_back(map);

	map = Map(3, 85, "MOUNT_MOON");
	map.addWarp(Warp(3, 3, "ROUTE_3", 1));
	map.addWarp(Warp(15, 15, "ROUTE_4", 1));
	map.addWarp(Warp(13, 3, "MOUNT_MOON", 7));
	map.addWarp(Warp(15, 11, "MOUNT_MOON", 8));
	map.addWarp(Warp(25, 5, "MOUNT_MOON_SQUARE", 1));
	map.addWarp(Warp(25, 15, "MOUNT_MOON_SQUARE", 2));
	map.addWarp(Warp(25, 3, "MOUNT_MOON", 3));
	map.addWarp(Warp(25, 13, "MOUNT_MOON", 4));
	maps.push_back(map);

	map = Map(15, 11, "MOUNT_MOON_GIFT_SHOP");
	map.addWarp(Warp(3, 7, "MOUNT_MOON_SQUARE", 3));
	map.addWarp(Warp(4, 7, "MOUNT_MOON_SQUARE", 3));
	maps.push_back(map);

	map = Map(15, 10, "MOUNT_MOON_SQUARE");
	map.addWarp(Warp(20, 5, "MOUNT_MOON", 5));
	map.addWarp(Warp(22, 11, "MOUNT_MOON", 6));
	map.addWarp(Warp(13, 7, "MOUNT_MOON_GIFT_SHOP", 1));
	maps.push_back(map);

	map = Map(3, 58, "MOUNT_MORTAR_1F_INSIDE");
	map.addWarp(Warp(11, 47, "MOUNT_MORTAR_1F_OUTSIDE", 5));
	map.addWarp(Warp(29, 47, "MOUNT_MORTAR_1F_OUTSIDE", 6));
	map.addWarp(Warp(5, 39, "MOUNT_MORTAR_1F_OUTSIDE", 8));
	map.addWarp(Warp(33, 41, "MOUNT_MORTAR_1F_OUTSIDE", 9));
	map.addWarp(Warp(3, 19, "MOUNT_MORTAR_B1F", 1));
	map.addWarp(Warp(9, 9, "MOUNT_MORTAR_2F_INSIDE", 2));
	maps.push_back(map);

	map = Map(3, 57, "MOUNT_MORTAR_1F_OUTSIDE");
	map.addWarp(Warp(3, 33, "ROUTE_42", 3));
	map.addWarp(Warp(17, 33, "ROUTE_42", 4));
	map.addWarp(Warp(37, 33, "ROUTE_42", 5));
	map.addWarp(Warp(17, 5, "MOUNT_MORTAR_2F_INSIDE", 1));
	map.addWarp(Warp(11, 21, "MOUNT_MORTAR_1F_INSIDE", 1));
	map.addWarp(Warp(29, 21, "MOUNT_MORTAR_1F_INSIDE", 2));
	map.addWarp(Warp(17, 29, "MOUNT_MORTAR_B1F", 2));
	map.addWarp(Warp(7, 13, "MOUNT_MORTAR_1F_INSIDE", 3));
	map.addWarp(Warp(33, 13, "MOUNT_MORTAR_1F_INSIDE", 4));
	maps.push_back(map);

	map = Map(3, 59, "MOUNT_MORTAR_2F_INSIDE");
	map.addWarp(Warp(17, 33, "MOUNT_MORTAR_1F_OUTSIDE", 4));
	map.addWarp(Warp(3, 5, "MOUNT_MORTAR_1F_INSIDE", 6));
	maps.push_back(map);

	map = Map(3, 60, "MOUNT_MORTAR_B1F");
	map.addWarp(Warp(3, 3, "MOUNT_MORTAR_1F_INSIDE", 5));
	map.addWarp(Warp(19, 29, "MOUNT_MORTAR_1F_OUTSIDE", 7));
	maps.push_back(map);

	map = Map(5, 7, "MOVE_DELETERS_HOUSE");
	map.addWarp(Warp(2, 7, "BLACKTHORN_CITY", 6));
	map.addWarp(Warp(3, 7, "BLACKTHORN_CITY", 6));
	maps.push_back(map);

	map = Map(18, 7, "MR_FUJIS_HOUSE");
	map.addWarp(Warp(2, 7, "LAVENDER_TOWN", 2));
	map.addWarp(Warp(3, 7, "LAVENDER_TOWN", 2));
	maps.push_back(map);

	map = Map(26, 10, "MR_POKEMONS_HOUSE");
	map.addWarp(Warp(2, 7, "ROUTE_30", 2));
	map.addWarp(Warp(3, 7, "ROUTE_30", 2));
	maps.push_back(map);

	map = Map(25, 8, "MR_PSYCHICS_HOUSE");
	map.addWarp(Warp(2, 7, "SAFFRON_CITY", 5));
	map.addWarp(Warp(3, 7, "SAFFRON_CITY", 5));
	maps.push_back(map);

	map = Map(3, 15, "NATIONAL_PARK");
	map.addWarp(Warp(33, 18, "ROUTE_36_NATIONAL_PARK_GATE", 1));
	map.addWarp(Warp(33, 19, "ROUTE_36_NATIONAL_PARK_GATE", 2));
	map.addWarp(Warp(10, 47, "ROUTE_35_NATIONAL_PARK_GATE", 1));
	map.addWarp(Warp(11, 47, "ROUTE_35_NATIONAL_PARK_GATE", 2));
	maps.push_back(map);

	map = Map(3, 16, "NATIONAL_PARK_BUG_CONTEST");
	map.addWarp(Warp(33, 18, "ROUTE_36_NATIONAL_PARK_GATE", 1));
	map.addWarp(Warp(33, 19, "ROUTE_36_NATIONAL_PARK_GATE", 1));
	map.addWarp(Warp(10, 47, "ROUTE_35_NATIONAL_PARK_GATE", 1));
	map.addWarp(Warp(11, 47, "ROUTE_35_NATIONAL_PARK_GATE", 1));
	maps.push_back(map);

	map = Map(24, 4, "NEW_BARK_TOWN");
	map.addWarp(Warp(6, 3, "ELMS_LAB", 1));
	map.addWarp(Warp(13, 5, "PLAYERS_HOUSE_1F", 1));
	map.addWarp(Warp(3, 11, "PLAYERS_NEIGHBORS_HOUSE", 1));
	map.addWarp(Warp(11, 13, "ELMS_HOUSE", 1));
	maps.push_back(map);

	map = Map(13, 6, "OAKS_LAB");
	map.addWarp(Warp(4, 11, "PALLET_TOWN", 3));
	map.addWarp(Warp(5, 11, "PALLET_TOWN", 3));
	maps.push_back(map);

	map = Map(1, 7, "OLIVINE_CAFE");
	map.addWarp(Warp(2, 7, "OLIVINE_CITY", 7));
	map.addWarp(Warp(3, 7, "OLIVINE_CITY", 7));
	maps.push_back(map);

	map = Map(1, 14, "OLIVINE_CITY");
	map.addWarp(Warp(13, 21, "OLIVINE_POKECENTER_1F", 1));
	map.addWarp(Warp(10, 11, "OLIVINE_GYM", 1));
	map.addWarp(Warp(25, 11, "OLIVINE_TIMS_HOUSE", 1));
	map.addWarp(Warp(0, 0, "OLIVINE_HOUSE_BETA", 1));
	map.addWarp(Warp(29, 11, "OLIVINE_PUNISHMENT_SPEECH_HOUSE", 1));
	map.addWarp(Warp(13, 15, "OLIVINE_GOOD_ROD_HOUSE", 1));
	map.addWarp(Warp(7, 21, "OLIVINE_CAFE", 1));
	map.addWarp(Warp(19, 17, "OLIVINE_MART", 2));
	map.addWarp(Warp(29, 27, "OLIVINE_LIGHTHOUSE_1F", 1));
	map.addWarp(Warp(19, 27, "OLIVINE_PORT_PASSAGE", 1));
	map.addWarp(Warp(20, 27, "OLIVINE_PORT_PASSAGE", 2));
	maps.push_back(map);

	map = Map(1, 6, "OLIVINE_GOOD_ROD_HOUSE");
	map.addWarp(Warp(2, 7, "OLIVINE_CITY", 6));
	map.addWarp(Warp(3, 7, "OLIVINE_CITY", 6));
	maps.push_back(map);

	map = Map(1, 2, "OLIVINE_GYM");
	map.addWarp(Warp(4, 15, "OLIVINE_CITY", 2));
	map.addWarp(Warp(5, 15, "OLIVINE_CITY", 2));
	maps.push_back(map);

	map = Map(1, 4, "OLIVINE_HOUSE_BETA");
	map.addWarp(Warp(2, 7, "OLIVINE_CITY", 4));
	map.addWarp(Warp(3, 7, "OLIVINE_CITY", 4));
	maps.push_back(map);

	map = Map(3, 42, "OLIVINE_LIGHTHOUSE_1F");
	map.addWarp(Warp(10, 17, "OLIVINE_CITY", 9));
	map.addWarp(Warp(11, 17, "OLIVINE_CITY", 9));
	map.addWarp(Warp(3, 11, "OLIVINE_LIGHTHOUSE_2F", 1));
	map.addWarp(Warp(16, 13, "OLIVINE_LIGHTHOUSE_2F", 3));
	map.addWarp(Warp(17, 13, "OLIVINE_LIGHTHOUSE_2F", 4));
	maps.push_back(map);

	map = Map(3, 43, "OLIVINE_LIGHTHOUSE_2F");
	map.addWarp(Warp(3, 11, "OLIVINE_LIGHTHOUSE_1F", 3));
	map.addWarp(Warp(5, 3, "OLIVINE_LIGHTHOUSE_3F", 2));
	map.addWarp(Warp(16, 13, "OLIVINE_LIGHTHOUSE_1F", 4));
	map.addWarp(Warp(17, 13, "OLIVINE_LIGHTHOUSE_1F", 5));
	map.addWarp(Warp(16, 11, "OLIVINE_LIGHTHOUSE_3F", 4));
	map.addWarp(Warp(17, 11, "OLIVINE_LIGHTHOUSE_3F", 5));
	maps.push_back(map);

	map = Map(3, 44, "OLIVINE_LIGHTHOUSE_3F");
	map.addWarp(Warp(13, 3, "OLIVINE_LIGHTHOUSE_4F", 1));
	map.addWarp(Warp(5, 3, "OLIVINE_LIGHTHOUSE_2F", 2));
	map.addWarp(Warp(9, 5, "OLIVINE_LIGHTHOUSE_4F", 4));
	map.addWarp(Warp(16, 11, "OLIVINE_LIGHTHOUSE_2F", 5));
	map.addWarp(Warp(17, 11, "OLIVINE_LIGHTHOUSE_2F", 6));
	map.addWarp(Warp(16, 9, "OLIVINE_LIGHTHOUSE_4F", 5));
	map.addWarp(Warp(17, 9, "OLIVINE_LIGHTHOUSE_4F", 6));
	map.addWarp(Warp(8, 3, "OLIVINE_LIGHTHOUSE_4F", 7));
	map.addWarp(Warp(9, 3, "OLIVINE_LIGHTHOUSE_4F", 8));
	maps.push_back(map);

	map = Map(3, 45, "OLIVINE_LIGHTHOUSE_4F");
	map.addWarp(Warp(13, 3, "OLIVINE_LIGHTHOUSE_3F", 1));
	map.addWarp(Warp(3, 5, "OLIVINE_LIGHTHOUSE_5F", 2));
	map.addWarp(Warp(9, 7, "OLIVINE_LIGHTHOUSE_5F", 3));
	map.addWarp(Warp(9, 5, "OLIVINE_LIGHTHOUSE_3F", 3));
	map.addWarp(Warp(16, 9, "OLIVINE_LIGHTHOUSE_3F", 6));
	map.addWarp(Warp(17, 9, "OLIVINE_LIGHTHOUSE_3F", 7));
	map.addWarp(Warp(8, 3, "OLIVINE_LIGHTHOUSE_3F", 8));
	map.addWarp(Warp(9, 3, "OLIVINE_LIGHTHOUSE_3F", 9));
	map.addWarp(Warp(16, 7, "OLIVINE_LIGHTHOUSE_5F", 4));
	map.addWarp(Warp(17, 7, "OLIVINE_LIGHTHOUSE_5F", 5));
	maps.push_back(map);

	map = Map(3, 46, "OLIVINE_LIGHTHOUSE_5F");
	map.addWarp(Warp(9, 15, "OLIVINE_LIGHTHOUSE_6F", 1));
	map.addWarp(Warp(3, 5, "OLIVINE_LIGHTHOUSE_4F", 2));
	map.addWarp(Warp(9, 7, "OLIVINE_LIGHTHOUSE_4F", 3));
	map.addWarp(Warp(16, 7, "OLIVINE_LIGHTHOUSE_4F", 9));
	map.addWarp(Warp(17, 7, "OLIVINE_LIGHTHOUSE_4F", 10));
	map.addWarp(Warp(16, 5, "OLIVINE_LIGHTHOUSE_6F", 2));
	map.addWarp(Warp(17, 5, "OLIVINE_LIGHTHOUSE_6F", 3));
	maps.push_back(map);

	map = Map(3, 47, "OLIVINE_LIGHTHOUSE_6F");
	map.addWarp(Warp(9, 15, "OLIVINE_LIGHTHOUSE_5F", 1));
	map.addWarp(Warp(16, 5, "OLIVINE_LIGHTHOUSE_5F", 6));
	map.addWarp(Warp(17, 5, "OLIVINE_LIGHTHOUSE_5F", 7));
	maps.push_back(map);

	map = Map(1, 8, "OLIVINE_MART");
	map.addWarp(Warp(2, 7, "OLIVINE_CITY", 8));
	map.addWarp(Warp(3, 7, "OLIVINE_CITY", 8));
	maps.push_back(map);

	map = Map(1, 1, "OLIVINE_POKECENTER_1F");
	map.addWarp(Warp(3, 7, "OLIVINE_CITY", 1));
	map.addWarp(Warp(4, 7, "OLIVINE_CITY", 1));
	map.addWarp(Warp(0, 7, "POKECENTER_2F", 1));
	maps.push_back(map);

	map = Map(15, 1, "OLIVINE_PORT");
	map.addWarp(Warp(11, 7, "OLIVINE_PORT_PASSAGE", 5));
	map.addWarp(Warp(7, 23, "FAST_SHIP_1F", 1));
	maps.push_back(map);

	map = Map(15, 8, "OLIVINE_PORT_PASSAGE");
	map.addWarp(Warp(15, 0, "OLIVINE_CITY", 10));
	map.addWarp(Warp(16, 0, "OLIVINE_CITY", 11));
	map.addWarp(Warp(15, 4, "OLIVINE_PORT_PASSAGE", 4));
	map.addWarp(Warp(3, 2, "OLIVINE_PORT_PASSAGE", 3));
	map.addWarp(Warp(3, 14, "OLIVINE_PORT", 1));
	maps.push_back(map);

	map = Map(1, 5, "OLIVINE_PUNISHMENT_SPEECH_HOUSE");
	map.addWarp(Warp(2, 7, "OLIVINE_CITY", 5));
	map.addWarp(Warp(3, 7, "OLIVINE_CITY", 5));
	maps.push_back(map);

	map = Map(1, 3, "OLIVINE_TIMS_HOUSE");
	map.addWarp(Warp(2, 7, "OLIVINE_CITY", 3));
	map.addWarp(Warp(3, 7, "OLIVINE_CITY", 3));
	maps.push_back(map);

	map = Map(13, 2, "PALLET_TOWN");
	map.addWarp(Warp(5, 5, "REDS_HOUSE_1F", 1));
	map.addWarp(Warp(13, 5, "BLUES_HOUSE", 1));
	map.addWarp(Warp(12, 11, "OAKS_LAB", 1));
	maps.push_back(map);

	map = Map(14, 2, "PEWTER_CITY");
	map.addWarp(Warp(29, 13, "PEWTER_NIDORAN_SPEECH_HOUSE", 1));
	map.addWarp(Warp(16, 17, "PEWTER_GYM", 1));
	map.addWarp(Warp(23, 17, "PEWTER_MART", 2));
	map.addWarp(Warp(13, 25, "PEWTER_POKECENTER_1F", 1));
	map.addWarp(Warp(7, 29, "PEWTER_SNOOZE_SPEECH_HOUSE", 1));
	maps.push_back(map);

	map = Map(14, 4, "PEWTER_GYM");
	map.addWarp(Warp(4, 13, "PEWTER_CITY", 2));
	map.addWarp(Warp(5, 13, "PEWTER_CITY", 2));
	maps.push_back(map);

	map = Map(14, 5, "PEWTER_MART");
	map.addWarp(Warp(2, 7, "PEWTER_CITY", 3));
	map.addWarp(Warp(3, 7, "PEWTER_CITY", 3));
	maps.push_back(map);

	map = Map(14, 3, "PEWTER_NIDORAN_SPEECH_HOUSE");
	map.addWarp(Warp(2, 7, "PEWTER_CITY", 1));
	map.addWarp(Warp(3, 7, "PEWTER_CITY", 1));
	maps.push_back(map);

	map = Map(14, 6, "PEWTER_POKECENTER_1F");
	map.addWarp(Warp(3, 7, "PEWTER_CITY", 4));
	map.addWarp(Warp(4, 7, "PEWTER_CITY", 4));
	map.addWarp(Warp(0, 7, "POKECENTER_2F", 1));
	maps.push_back(map);

	map = Map(14, 7, "PEWTER_POKECENTER_2F_BETA");
	map.addWarp(Warp(0, 7, "PEWTER_POKECENTER_1F", 3));
	maps.push_back(map);

	map = Map(14, 8, "PEWTER_SNOOZE_SPEECH_HOUSE");
	map.addWarp(Warp(2, 7, "PEWTER_CITY", 5));
	map.addWarp(Warp(3, 7, "PEWTER_CITY", 5));
	maps.push_back(map);

	map = Map(24, 6, "PLAYERS_HOUSE_1F");
	map.addWarp(Warp(6, 7, "NEW_BARK_TOWN", 2));
	map.addWarp(Warp(7, 7, "NEW_BARK_TOWN", 2));
	map.addWarp(Warp(9, 0, "PLAYERS_HOUSE_2F", 1));
	maps.push_back(map);

	map = Map(24, 7, "PLAYERS_HOUSE_2F");
	map.addWarp(Warp(7, 0, "PLAYERS_HOUSE_1F", 3));
	maps.push_back(map);

	map = Map(24, 8, "PLAYERS_NEIGHBORS_HOUSE");
	map.addWarp(Warp(2, 7, "NEW_BARK_TOWN", 3));
	map.addWarp(Warp(3, 7, "NEW_BARK_TOWN", 3));
	maps.push_back(map);

	map = Map(20, 1, "POKECENTER_2F");
	map.addWarp(Warp(0, 7, "POKECENTER_2F", -1));
	map.addWarp(Warp(5, 0, "TRADE_CENTER", 1));
	map.addWarp(Warp(9, 0, "COLOSSEUM", 1));
	map.addWarp(Warp(13, 2, "TIME_CAPSULE", 1));
	map.addWarp(Warp(6, 0, "MOBILE_TRADE_ROOM", 1));
	map.addWarp(Warp(10, 0, "MOBILE_BATTLE_ROOM", 1));
	maps.push_back(map);

	map = Map(11, 21, "POKECOM_CENTER_ADMIN_OFFICE_MOBILE");
	map.addWarp(Warp(0, 31, "GOLDENROD_POKECENTER_1F", 3));
	map.addWarp(Warp(1, 31, "GOLDENROD_POKECENTER_1F", 3));
	maps.push_back(map);

	map = Map(12, 1, "POKEMON_FAN_CLUB");
	map.addWarp(Warp(2, 7, "VERMILION_CITY", 3));
	map.addWarp(Warp(3, 7, "VERMILION_CITY", 3));
	maps.push_back(map);

	map = Map(22, 10, "POKE_SEERS_HOUSE");
	map.addWarp(Warp(2, 7, "CIANWOOD_CITY", 7));
	map.addWarp(Warp(3, 7, "CIANWOOD_CITY", 7));
	maps.push_back(map);

	map = Map(7, 10, "POWER_PLANT");
	map.addWarp(Warp(2, 17, "ROUTE_10_NORTH", 2));
	map.addWarp(Warp(3, 17, "ROUTE_10_NORTH", 2));
	maps.push_back(map);

	map = Map(3, 17, "RADIO_TOWER_1F");
	map.addWarp(Warp(2, 7, "GOLDENROD_CITY", 11));
	map.addWarp(Warp(3, 7, "GOLDENROD_CITY", 11));
	map.addWarp(Warp(15, 0, "RADIO_TOWER_2F", 2));
	maps.push_back(map);

	map = Map(3, 18, "RADIO_TOWER_2F");
	map.addWarp(Warp(0, 0, "RADIO_TOWER_3F", 1));
	map.addWarp(Warp(15, 0, "RADIO_TOWER_1F", 3));
	maps.push_back(map);

	map = Map(3, 19, "RADIO_TOWER_3F");
	map.addWarp(Warp(0, 0, "RADIO_TOWER_2F", 1));
	map.addWarp(Warp(7, 0, "RADIO_TOWER_4F", 2));
	map.addWarp(Warp(17, 0, "RADIO_TOWER_4F", 4));
	maps.push_back(map);

	map = Map(3, 20, "RADIO_TOWER_4F");
	map.addWarp(Warp(0, 0, "RADIO_TOWER_5F", 1));
	map.addWarp(Warp(9, 0, "RADIO_TOWER_3F", 2));
	map.addWarp(Warp(12, 0, "RADIO_TOWER_5F", 2));
	map.addWarp(Warp(17, 0, "RADIO_TOWER_3F", 3));
	maps.push_back(map);

	map = Map(3, 21, "RADIO_TOWER_5F");
	map.addWarp(Warp(0, 0, "RADIO_TOWER_4F", 1));
	map.addWarp(Warp(12, 0, "RADIO_TOWER_4F", 3));
	maps.push_back(map);

	map = Map(13, 3, "REDS_HOUSE_1F");
	map.addWarp(Warp(2, 7, "PALLET_TOWN", 1));
	map.addWarp(Warp(3, 7, "PALLET_TOWN", 1));
	map.addWarp(Warp(7, 0, "REDS_HOUSE_2F", 1));
	maps.push_back(map);

	map = Map(13, 4, "REDS_HOUSE_2F");
	map.addWarp(Warp(7, 0, "REDS_HOUSE_1F", 3));
	maps.push_back(map);

	map = Map(3, 87, "ROCK_TUNNEL_1F");
	map.addWarp(Warp(15, 3, "ROUTE_9", 1));
	map.addWarp(Warp(11, 25, "ROUTE_10_SOUTH", 1));
	map.addWarp(Warp(5, 3, "ROCK_TUNNEL_B1F", 3));
	map.addWarp(Warp(15, 9, "ROCK_TUNNEL_B1F", 2));
	map.addWarp(Warp(27, 3, "ROCK_TUNNEL_B1F", 4));
	map.addWarp(Warp(27, 13, "ROCK_TUNNEL_B1F", 1));
	maps.push_back(map);

	map = Map(3, 88, "ROCK_TUNNEL_B1F");
	map.addWarp(Warp(3, 3, "ROCK_TUNNEL_1F", 6));
	map.addWarp(Warp(17, 9, "ROCK_TUNNEL_1F", 4));
	map.addWarp(Warp(23, 3, "ROCK_TUNNEL_1F", 3));
	map.addWarp(Warp(25, 23, "ROCK_TUNNEL_1F", 5));
	maps.push_back(map);

	map = Map(7, 14, "ROUTE_10_NORTH");
	map.addWarp(Warp(11, 1, "ROUTE_10_POKECENTER_1F", 1));
	map.addWarp(Warp(3, 9, "POWER_PLANT", 1));
	maps.push_back(map);

	map = Map(7, 8, "ROUTE_10_POKECENTER_1F");
	map.addWarp(Warp(3, 7, "ROUTE_10_NORTH", 1));
	map.addWarp(Warp(4, 7, "ROUTE_10_NORTH", 1));
	map.addWarp(Warp(0, 7, "POKECENTER_2F", 1));
	maps.push_back(map);

	map = Map(7, 9, "ROUTE_10_POKECENTER_2F_BETA");
	map.addWarp(Warp(0, 7, "ROUTE_10_POKECENTER_1F", 3));
	maps.push_back(map);

	map = Map(18, 3, "ROUTE_10_SOUTH");
	map.addWarp(Warp(6, 1, "ROCK_TUNNEL_1F", 2));
	maps.push_back(map);

	map = Map(18, 2, "ROUTE_12");
	map.addWarp(Warp(11, 33, "ROUTE_12_SUPER_ROD_HOUSE", 1));
	maps.push_back(map);

	map = Map(18, 14, "ROUTE_12_SUPER_ROD_HOUSE");
	map.addWarp(Warp(2, 7, "ROUTE_12", 1));
	map.addWarp(Warp(3, 7, "ROUTE_12", 1));
	maps.push_back(map);

	map = Map(17, 3, "ROUTE_15");
	map.addWarp(Warp(2, 4, "ROUTE_15_FUCHSIA_GATE", 3));
	map.addWarp(Warp(2, 5, "ROUTE_15_FUCHSIA_GATE", 4));
	maps.push_back(map);

	map = Map(17, 13, "ROUTE_15_FUCHSIA_GATE");
	map.addWarp(Warp(0, 4, "FUCHSIA_CITY", 8));
	map.addWarp(Warp(0, 5, "FUCHSIA_CITY", 9));
	map.addWarp(Warp(9, 4, "ROUTE_15", 1));
	map.addWarp(Warp(9, 5, "ROUTE_15", 2));
	maps.push_back(map);

	map = Map(21, 2, "ROUTE_16");
	map.addWarp(Warp(3, 1, "ROUTE_16_FUCHSIA_SPEECH_HOUSE", 1));
	map.addWarp(Warp(14, 6, "ROUTE_16_GATE", 3));
	map.addWarp(Warp(14, 7, "ROUTE_16_GATE", 4));
	map.addWarp(Warp(9, 6, "ROUTE_16_GATE", 1));
	map.addWarp(Warp(9, 7, "ROUTE_16_GATE", 2));
	maps.push_back(map);

	map = Map(21, 23, "ROUTE_16_FUCHSIA_SPEECH_HOUSE");
	map.addWarp(Warp(2, 7, "ROUTE_16", 1));
	map.addWarp(Warp(3, 7, "ROUTE_16", 1));
	maps.push_back(map);

	map = Map(21, 24, "ROUTE_16_GATE");
	map.addWarp(Warp(0, 4, "ROUTE_16", 4));
	map.addWarp(Warp(0, 5, "ROUTE_16", 5));
	map.addWarp(Warp(9, 4, "ROUTE_16", 2));
	map.addWarp(Warp(9, 5, "ROUTE_16", 3));
	maps.push_back(map);

	map = Map(21, 3, "ROUTE_17");
	map.addWarp(Warp(17, 82, "ROUTE_17_ROUTE_18_GATE", 1));
	map.addWarp(Warp(17, 83, "ROUTE_17_ROUTE_18_GATE", 2));
	maps.push_back(map);

	map = Map(21, 26, "ROUTE_17_ROUTE_18_GATE");
	map.addWarp(Warp(0, 4, "ROUTE_17", 1));
	map.addWarp(Warp(0, 5, "ROUTE_17", 2));
	map.addWarp(Warp(9, 4, "ROUTE_18", 1));
	map.addWarp(Warp(9, 5, "ROUTE_18", 2));
	maps.push_back(map);

	map = Map(17, 4, "ROUTE_18");
	map.addWarp(Warp(2, 6, "ROUTE_17_ROUTE_18_GATE", 3));
	map.addWarp(Warp(2, 7, "ROUTE_17_ROUTE_18_GATE", 4));
	maps.push_back(map);

	map = Map(6, 5, "ROUTE_19");
	map.addWarp(Warp(7, 3, "ROUTE_19_FUCHSIA_GATE", 3));
	maps.push_back(map);

	map = Map(6, 3, "ROUTE_19_FUCHSIA_GATE");
	map.addWarp(Warp(4, 0, "FUCHSIA_CITY", 10));
	map.addWarp(Warp(5, 0, "FUCHSIA_CITY", 11));
	map.addWarp(Warp(4, 7, "ROUTE_19", 1));
	map.addWarp(Warp(5, 7, "ROUTE_19", 1));
	maps.push_back(map);

	map = Map(23, 1, "ROUTE_2");
	map.addWarp(Warp(15, 15, "ROUTE_2_NUGGET_HOUSE", 1));
	map.addWarp(Warp(15, 31, "ROUTE_2_GATE", 3));
	map.addWarp(Warp(16, 27, "ROUTE_2_GATE", 1));
	map.addWarp(Warp(17, 27, "ROUTE_2_GATE", 2));
	map.addWarp(Warp(12, 7, "DIGLETTS_CAVE", 3));
	maps.push_back(map);

	map = Map(6, 6, "ROUTE_20");
	map.addWarp(Warp(38, 7, "SEAFOAM_GYM", 1));
	maps.push_back(map);

	map = Map(23, 2, "ROUTE_22");
	map.addWarp(Warp(13, 5, "VICTORY_ROAD_GATE", 1));
	maps.push_back(map);

	map = Map(16, 1, "ROUTE_23");
	map.addWarp(Warp(9, 5, "INDIGO_PLATEAU_POKECENTER_1F", 1));
	map.addWarp(Warp(10, 5, "INDIGO_PLATEAU_POKECENTER_1F", 2));
	map.addWarp(Warp(9, 13, "VICTORY_ROAD", 10));
	map.addWarp(Warp(10, 13, "VICTORY_ROAD", 10));
	maps.push_back(map);

	map = Map(7, 16, "ROUTE_25");
	map.addWarp(Warp(47, 5, "BILLS_HOUSE", 1));
	maps.push_back(map);

	map = Map(24, 1, "ROUTE_26");
	map.addWarp(Warp(7, 5, "VICTORY_ROAD_GATE", 3));
	map.addWarp(Warp(15, 57, "ROUTE_26_HEAL_HOUSE", 1));
	map.addWarp(Warp(5, 71, "DAY_OF_WEEK_SIBLINGS_HOUSE", 1));
	maps.push_back(map);

	map = Map(24, 10, "ROUTE_26_HEAL_HOUSE");
	map.addWarp(Warp(2, 7, "ROUTE_26", 2));
	map.addWarp(Warp(3, 7, "ROUTE_26", 2));
	maps.push_back(map);

	map = Map(24, 2, "ROUTE_27");
	map.addWarp(Warp(33, 7, "ROUTE_27_SANDSTORM_HOUSE", 1));
	map.addWarp(Warp(26, 5, "TOHJO_FALLS", 1));
	map.addWarp(Warp(36, 5, "TOHJO_FALLS", 2));
	maps.push_back(map);

	map = Map(24, 12, "ROUTE_27_SANDSTORM_HOUSE");
	map.addWarp(Warp(2, 7, "ROUTE_27", 1));
	map.addWarp(Warp(3, 7, "ROUTE_27", 1));
	maps.push_back(map);

	map = Map(19, 1, "ROUTE_28");
	map.addWarp(Warp(7, 3, "ROUTE_28_STEEL_WING_HOUSE", 1));
	map.addWarp(Warp(33, 5, "VICTORY_ROAD_GATE", 7));
	maps.push_back(map);

	map = Map(19, 4, "ROUTE_28_STEEL_WING_HOUSE");
	map.addWarp(Warp(2, 7, "ROUTE_28", 1));
	map.addWarp(Warp(3, 7, "ROUTE_28", 1));
	maps.push_back(map);

	map = Map(24, 3, "ROUTE_29");
	map.addWarp(Warp(27, 1, "ROUTE_29_ROUTE_46_GATE", 3));
	maps.push_back(map);

	map = Map(24, 13, "ROUTE_29_ROUTE_46_GATE");
	map.addWarp(Warp(4, 0, "ROUTE_46", 1));
	map.addWarp(Warp(5, 0, "ROUTE_46", 2));
	map.addWarp(Warp(4, 7, "ROUTE_29", 1));
	map.addWarp(Warp(5, 7, "ROUTE_29", 1));
	maps.push_back(map);

	map = Map(23, 12, "ROUTE_2_GATE");
	map.addWarp(Warp(4, 0, "ROUTE_2", 3));
	map.addWarp(Warp(5, 0, "ROUTE_2", 4));
	map.addWarp(Warp(4, 7, "ROUTE_2", 2));
	map.addWarp(Warp(5, 7, "ROUTE_2", 2));
	maps.push_back(map);

	map = Map(23, 11, "ROUTE_2_NUGGET_HOUSE");
	map.addWarp(Warp(2, 7, "ROUTE_2", 1));
	map.addWarp(Warp(3, 7, "ROUTE_2", 1));
	maps.push_back(map);

	map = Map(14, 1, "ROUTE_3");
	map.addWarp(Warp(52, 1, "MOUNT_MOON", 1));
	maps.push_back(map);

	map = Map(26, 1, "ROUTE_30");
	map.addWarp(Warp(7, 39, "ROUTE_30_BERRY_HOUSE", 1));
	map.addWarp(Warp(17, 5, "MR_POKEMONS_HOUSE", 1));
	maps.push_back(map);

	map = Map(26, 9, "ROUTE_30_BERRY_HOUSE");
	map.addWarp(Warp(2, 7, "ROUTE_30", 1));
	map.addWarp(Warp(3, 7, "ROUTE_30", 1));
	maps.push_back(map);

	map = Map(26, 2, "ROUTE_31");
	map.addWarp(Warp(4, 6, "ROUTE_31_VIOLET_GATE", 3));
	map.addWarp(Warp(4, 7, "ROUTE_31_VIOLET_GATE", 4));
	map.addWarp(Warp(34, 5, "DARK_CAVE_VIOLET_ENTRANCE", 1));
	maps.push_back(map);

	map = Map(26, 11, "ROUTE_31_VIOLET_GATE");
	map.addWarp(Warp(0, 4, "VIOLET_CITY", 8));
	map.addWarp(Warp(0, 5, "VIOLET_CITY", 9));
	map.addWarp(Warp(9, 4, "ROUTE_31", 1));
	map.addWarp(Warp(9, 5, "ROUTE_31", 2));
	maps.push_back(map);

	map = Map(10, 1, "ROUTE_32");
	map.addWarp(Warp(11, 73, "ROUTE_32_POKECENTER_1F", 1));
	map.addWarp(Warp(4, 2, "ROUTE_32_RUINS_OF_ALPH_GATE", 3));
	map.addWarp(Warp(4, 3, "ROUTE_32_RUINS_OF_ALPH_GATE", 4));
	map.addWarp(Warp(6, 79, "UNION_CAVE_1F", 4));
	maps.push_back(map);

	map = Map(10, 13, "ROUTE_32_POKECENTER_1F");
	map.addWarp(Warp(3, 7, "ROUTE_32", 1));
	map.addWarp(Warp(4, 7, "ROUTE_32", 1));
	map.addWarp(Warp(0, 7, "POKECENTER_2F", 1));
	maps.push_back(map);

	map = Map(10, 12, "ROUTE_32_RUINS_OF_ALPH_GATE");
	map.addWarp(Warp(0, 4, "RUINS_OF_ALPH_OUTSIDE", 10));
	map.addWarp(Warp(0, 5, "RUINS_OF_ALPH_OUTSIDE", 11));
	map.addWarp(Warp(9, 4, "ROUTE_32", 2));
	map.addWarp(Warp(9, 5, "ROUTE_32", 3));
	maps.push_back(map);

	map = Map(8, 6, "ROUTE_33");
	map.addWarp(Warp(11, 9, "UNION_CAVE_1F", 3));
	maps.push_back(map);

	map = Map(11, 1, "ROUTE_34");
	map.addWarp(Warp(13, 37, "ROUTE_34_ILEX_FOREST_GATE", 1));
	map.addWarp(Warp(14, 37, "ROUTE_34_ILEX_FOREST_GATE", 2));
	map.addWarp(Warp(11, 14, "DAY_CARE", 1));
	map.addWarp(Warp(11, 15, "DAY_CARE", 2));
	map.addWarp(Warp(13, 15, "DAY_CARE", 3));
	maps.push_back(map);

	map = Map(11, 23, "ROUTE_34_ILEX_FOREST_GATE");
	map.addWarp(Warp(4, 0, "ROUTE_34", 1));
	map.addWarp(Warp(5, 0, "ROUTE_34", 2));
	map.addWarp(Warp(4, 7, "ILEX_FOREST", 1));
	map.addWarp(Warp(5, 7, "ILEX_FOREST", 1));
	maps.push_back(map);

	map = Map(10, 2, "ROUTE_35");
	map.addWarp(Warp(9, 33, "ROUTE_35_GOLDENROD_GATE", 1));
	map.addWarp(Warp(10, 33, "ROUTE_35_GOLDENROD_GATE", 2));
	map.addWarp(Warp(3, 5, "ROUTE_35_NATIONAL_PARK_GATE", 3));
	maps.push_back(map);

	map = Map(10, 14, "ROUTE_35_GOLDENROD_GATE");
	map.addWarp(Warp(4, 0, "ROUTE_35", 1));
	map.addWarp(Warp(5, 0, "ROUTE_35", 2));
	map.addWarp(Warp(4, 7, "GOLDENROD_CITY", 12));
	map.addWarp(Warp(5, 7, "GOLDENROD_CITY", 12));
	maps.push_back(map);

	map = Map(10, 15, "ROUTE_35_NATIONAL_PARK_GATE");
	map.addWarp(Warp(3, 0, "NATIONAL_PARK", 3));
	map.addWarp(Warp(4, 0, "NATIONAL_PARK", 4));
	map.addWarp(Warp(3, 7, "ROUTE_35", 3));
	map.addWarp(Warp(4, 7, "ROUTE_35", 3));
	maps.push_back(map);

	map = Map(10, 3, "ROUTE_36");
	map.addWarp(Warp(18, 8, "ROUTE_36_NATIONAL_PARK_GATE", 3));
	map.addWarp(Warp(18, 9, "ROUTE_36_NATIONAL_PARK_GATE", 4));
	map.addWarp(Warp(47, 13, "ROUTE_36_RUINS_OF_ALPH_GATE", 1));
	map.addWarp(Warp(48, 13, "ROUTE_36_RUINS_OF_ALPH_GATE", 2));
	maps.push_back(map);

	map = Map(10, 17, "ROUTE_36_NATIONAL_PARK_GATE");
	map.addWarp(Warp(0, 4, "NATIONAL_PARK", 1));
	map.addWarp(Warp(0, 5, "NATIONAL_PARK", 2));
	map.addWarp(Warp(9, 4, "ROUTE_36", 1));
	map.addWarp(Warp(9, 5, "ROUTE_36", 2));
	maps.push_back(map);

	map = Map(10, 16, "ROUTE_36_RUINS_OF_ALPH_GATE");
	map.addWarp(Warp(4, 0, "ROUTE_36", 3));
	map.addWarp(Warp(5, 0, "ROUTE_36", 4));
	map.addWarp(Warp(4, 7, "RUINS_OF_ALPH_OUTSIDE", 9));
	map.addWarp(Warp(5, 7, "RUINS_OF_ALPH_OUTSIDE", 9));
	maps.push_back(map);

	map = Map(1, 12, "ROUTE_38");
	map.addWarp(Warp(35, 8, "ROUTE_38_ECRUTEAK_GATE", 1));
	map.addWarp(Warp(35, 9, "ROUTE_38_ECRUTEAK_GATE", 2));
	maps.push_back(map);

	map = Map(1, 9, "ROUTE_38_ECRUTEAK_GATE");
	map.addWarp(Warp(0, 4, "ROUTE_38", 1));
	map.addWarp(Warp(0, 5, "ROUTE_38", 2));
	map.addWarp(Warp(9, 4, "ECRUTEAK_CITY", 14));
	map.addWarp(Warp(9, 5, "ECRUTEAK_CITY", 15));
	maps.push_back(map);

	map = Map(1, 13, "ROUTE_39");
	map.addWarp(Warp(1, 3, "ROUTE_39_BARN", 1));
	map.addWarp(Warp(5, 3, "ROUTE_39_FARMHOUSE", 1));
	maps.push_back(map);

	map = Map(1, 10, "ROUTE_39_BARN");
	map.addWarp(Warp(3, 7, "ROUTE_39", 1));
	map.addWarp(Warp(4, 7, "ROUTE_39", 1));
	maps.push_back(map);

	map = Map(1, 11, "ROUTE_39_FARMHOUSE");
	map.addWarp(Warp(2, 7, "ROUTE_39", 2));
	map.addWarp(Warp(3, 7, "ROUTE_39", 2));
	maps.push_back(map);

	map = Map(7, 12, "ROUTE_4");
	map.addWarp(Warp(2, 5, "MOUNT_MOON", 2));
	maps.push_back(map);

	map = Map(22, 1, "ROUTE_40");
	map.addWarp(Warp(9, 5, "ROUTE_40_BATTLE_TOWER_GATE", 1));
	maps.push_back(map);

	map = Map(22, 15, "ROUTE_40_BATTLE_TOWER_GATE");
	map.addWarp(Warp(4, 7, "ROUTE_40", 1));
	map.addWarp(Warp(5, 7, "ROUTE_40", 1));
	map.addWarp(Warp(4, 0, "BATTLE_TOWER_OUTSIDE", 1));
	map.addWarp(Warp(5, 0, "BATTLE_TOWER_OUTSIDE", 2));
	maps.push_back(map);

	map = Map(22, 2, "ROUTE_41");
	map.addWarp(Warp(12, 17, "WHIRL_ISLAND_NW", 1));
	map.addWarp(Warp(36, 19, "WHIRL_ISLAND_NE", 1));
	map.addWarp(Warp(12, 37, "WHIRL_ISLAND_SW", 1));
	map.addWarp(Warp(36, 45, "WHIRL_ISLAND_SE", 1));
	maps.push_back(map);

	map = Map(2, 5, "ROUTE_42");
	map.addWarp(Warp(0, 8, "ROUTE_42_ECRUTEAK_GATE", 3));
	map.addWarp(Warp(0, 9, "ROUTE_42_ECRUTEAK_GATE", 4));
	map.addWarp(Warp(10, 5, "MOUNT_MORTAR_1F_OUTSIDE", 1));
	map.addWarp(Warp(28, 9, "MOUNT_MORTAR_1F_OUTSIDE", 2));
	map.addWarp(Warp(46, 7, "MOUNT_MORTAR_1F_OUTSIDE", 3));
	maps.push_back(map);

	map = Map(2, 4, "ROUTE_42_ECRUTEAK_GATE");
	map.addWarp(Warp(0, 4, "ECRUTEAK_CITY", 1));
	map.addWarp(Warp(0, 5, "ECRUTEAK_CITY", 2));
	map.addWarp(Warp(9, 4, "ROUTE_42", 1));
	map.addWarp(Warp(9, 5, "ROUTE_42", 2));
	maps.push_back(map);

	map = Map(9, 5, "ROUTE_43");
	map.addWarp(Warp(9, 51, "ROUTE_43_MAHOGANY_GATE", 1));
	map.addWarp(Warp(10, 51, "ROUTE_43_MAHOGANY_GATE", 2));
	map.addWarp(Warp(17, 35, "ROUTE_43_GATE", 3));
	map.addWarp(Warp(17, 31, "ROUTE_43_GATE", 1));
	map.addWarp(Warp(18, 31, "ROUTE_43_GATE", 2));
	maps.push_back(map);

	map = Map(9, 4, "ROUTE_43_GATE");
	map.addWarp(Warp(4, 0, "ROUTE_43", 4));
	map.addWarp(Warp(5, 0, "ROUTE_43", 5));
	map.addWarp(Warp(4, 7, "ROUTE_43", 3));
	map.addWarp(Warp(5, 7, "ROUTE_43", 3));
	maps.push_back(map);

	map = Map(9, 3, "ROUTE_43_MAHOGANY_GATE");
	map.addWarp(Warp(4, 0, "ROUTE_43", 1));
	map.addWarp(Warp(5, 0, "ROUTE_43", 2));
	map.addWarp(Warp(4, 7, "MAHOGANY_TOWN", 5));
	map.addWarp(Warp(5, 7, "MAHOGANY_TOWN", 5));
	maps.push_back(map);

	map = Map(2, 6, "ROUTE_44");
	map.addWarp(Warp(56, 7, "ICE_PATH_1F", 1));
	maps.push_back(map);

	map = Map(5, 8, "ROUTE_45");
	map.addWarp(Warp(2, 5, "DARK_CAVE_BLACKTHORN_ENTRANCE", 1));
	maps.push_back(map);

	map = Map(5, 9, "ROUTE_46");
	map.addWarp(Warp(7, 33, "ROUTE_29_ROUTE_46_GATE", 1));
	map.addWarp(Warp(8, 33, "ROUTE_29_ROUTE_46_GATE", 2));
	map.addWarp(Warp(14, 5, "DARK_CAVE_VIOLET_ENTRANCE", 3));
	maps.push_back(map);

	map = Map(25, 1, "ROUTE_5");
	map.addWarp(Warp(17, 15, "ROUTE_5_UNDERGROUND_PATH_ENTRANCE", 1));
	map.addWarp(Warp(8, 17, "ROUTE_5_SAFFRON_GATE", 1));
	map.addWarp(Warp(9, 17, "ROUTE_5_SAFFRON_GATE", 2));
	map.addWarp(Warp(10, 11, "ROUTE_5_CLEANSE_TAG_HOUSE", 1));
	maps.push_back(map);

	map = Map(25, 15, "ROUTE_5_CLEANSE_TAG_HOUSE");
	map.addWarp(Warp(2, 7, "ROUTE_5", 4));
	map.addWarp(Warp(3, 7, "ROUTE_5", 4));
	maps.push_back(map);

	map = Map(25, 14, "ROUTE_5_SAFFRON_GATE");
	map.addWarp(Warp(4, 0, "ROUTE_5", 2));
	map.addWarp(Warp(5, 0, "ROUTE_5", 3));
	map.addWarp(Warp(4, 7, "SAFFRON_CITY", 9));
	map.addWarp(Warp(5, 7, "SAFFRON_CITY", 9));
	maps.push_back(map);

	map = Map(25, 13, "ROUTE_5_UNDERGROUND_PATH_ENTRANCE");
	map.addWarp(Warp(3, 7, "ROUTE_5", 1));
	map.addWarp(Warp(4, 7, "ROUTE_5", 1));
	map.addWarp(Warp(4, 3, "UNDERGROUND_PATH", 1));
	maps.push_back(map);

	map = Map(12, 1, "ROUTE_6");
	map.addWarp(Warp(17, 3, "ROUTE_6_UNDERGROUND_PATH_ENTRANCE", 1));
	map.addWarp(Warp(6, 1, "ROUTE_6_SAFFRON_GATE", 3));
	maps.push_back(map);

	map = Map(12, 1, "ROUTE_6_SAFFRON_GATE");
	map.addWarp(Warp(4, 0, "SAFFRON_CITY", 12));
	map.addWarp(Warp(5, 0, "SAFFRON_CITY", 13));
	map.addWarp(Warp(4, 7, "ROUTE_6", 2));
	map.addWarp(Warp(5, 7, "ROUTE_6", 2));
	maps.push_back(map);

	map = Map(12, 1, "ROUTE_6_UNDERGROUND_PATH_ENTRANCE");
	map.addWarp(Warp(3, 7, "ROUTE_6", 1));
	map.addWarp(Warp(4, 7, "ROUTE_6", 1));
	map.addWarp(Warp(4, 3, "UNDERGROUND_PATH", 2));
	maps.push_back(map);

	map = Map(21, 1, "ROUTE_7");
	map.addWarp(Warp(15, 6, "ROUTE_7_SAFFRON_GATE", 1));
	map.addWarp(Warp(15, 7, "ROUTE_7_SAFFRON_GATE", 2));
	maps.push_back(map);

	map = Map(21, 25, "ROUTE_7_SAFFRON_GATE");
	map.addWarp(Warp(0, 4, "ROUTE_7", 1));
	map.addWarp(Warp(0, 5, "ROUTE_7", 2));
	map.addWarp(Warp(9, 4, "SAFFRON_CITY", 10));
	map.addWarp(Warp(9, 5, "SAFFRON_CITY", 11));
	maps.push_back(map);

	map = Map(18, 1, "ROUTE_8");
	map.addWarp(Warp(4, 4, "ROUTE_8_SAFFRON_GATE", 3));
	map.addWarp(Warp(4, 5, "ROUTE_8_SAFFRON_GATE", 4));
	maps.push_back(map);

	map = Map(18, 13, "ROUTE_8_SAFFRON_GATE");
	map.addWarp(Warp(0, 4, "SAFFRON_CITY", 14));
	map.addWarp(Warp(0, 5, "SAFFRON_CITY", 15));
	map.addWarp(Warp(9, 4, "ROUTE_8", 1));
	map.addWarp(Warp(9, 5, "ROUTE_8", 2));
	maps.push_back(map);

	map = Map(7, 13, "ROUTE_9");
	map.addWarp(Warp(48, 15, "ROCK_TUNNEL_1F", 1));
	maps.push_back(map);

	map = Map(3, 26, "RUINS_OF_ALPH_AERODACTYL_CHAMBER");
	map.addWarp(Warp(3, 9, "RUINS_OF_ALPH_OUTSIDE", 4));
	map.addWarp(Warp(4, 9, "RUINS_OF_ALPH_OUTSIDE", 4));
	map.addWarp(Warp(3, 3, "RUINS_OF_ALPH_INNER_CHAMBER", 8));
	map.addWarp(Warp(4, 3, "RUINS_OF_ALPH_INNER_CHAMBER", 9));
	map.addWarp(Warp(4, 0, "RUINS_OF_ALPH_AERODACTYL_ITEM_ROOM", 1));
	maps.push_back(map);

	map = Map(3, 32, "RUINS_OF_ALPH_AERODACTYL_ITEM_ROOM");
	map.addWarp(Warp(3, 9, "RUINS_OF_ALPH_AERODACTYL_CHAMBER", 5));
	map.addWarp(Warp(4, 9, "RUINS_OF_ALPH_AERODACTYL_CHAMBER", 5));
	map.addWarp(Warp(3, 1, "RUINS_OF_ALPH_AERODACTYL_WORD_ROOM", 1));
	map.addWarp(Warp(4, 1, "RUINS_OF_ALPH_AERODACTYL_WORD_ROOM", 2));
	maps.push_back(map);

	map = Map(3, 36, "RUINS_OF_ALPH_AERODACTYL_WORD_ROOM");
	map.addWarp(Warp(9, 5, "RUINS_OF_ALPH_AERODACTYL_ITEM_ROOM", 3));
	map.addWarp(Warp(10, 5, "RUINS_OF_ALPH_AERODACTYL_ITEM_ROOM", 4));
	map.addWarp(Warp(17, 11, "RUINS_OF_ALPH_INNER_CHAMBER", 8));
	maps.push_back(map);

	map = Map(3, 23, "RUINS_OF_ALPH_HO_OH_CHAMBER");
	map.addWarp(Warp(3, 9, "RUINS_OF_ALPH_OUTSIDE", 1));
	map.addWarp(Warp(4, 9, "RUINS_OF_ALPH_OUTSIDE", 1));
	map.addWarp(Warp(3, 3, "RUINS_OF_ALPH_INNER_CHAMBER", 2));
	map.addWarp(Warp(4, 3, "RUINS_OF_ALPH_INNER_CHAMBER", 3));
	map.addWarp(Warp(4, 0, "RUINS_OF_ALPH_HO_OH_ITEM_ROOM", 1));
	maps.push_back(map);

	map = Map(3, 29, "RUINS_OF_ALPH_HO_OH_ITEM_ROOM");
	map.addWarp(Warp(3, 9, "RUINS_OF_ALPH_HO_OH_CHAMBER", 5));
	map.addWarp(Warp(4, 9, "RUINS_OF_ALPH_HO_OH_CHAMBER", 5));
	map.addWarp(Warp(3, 1, "RUINS_OF_ALPH_HO_OH_WORD_ROOM", 1));
	map.addWarp(Warp(4, 1, "RUINS_OF_ALPH_HO_OH_WORD_ROOM", 2));
	maps.push_back(map);

	map = Map(3, 33, "RUINS_OF_ALPH_HO_OH_WORD_ROOM");
	map.addWarp(Warp(9, 9, "RUINS_OF_ALPH_HO_OH_ITEM_ROOM", 3));
	map.addWarp(Warp(10, 9, "RUINS_OF_ALPH_HO_OH_ITEM_ROOM", 4));
	map.addWarp(Warp(17, 21, "RUINS_OF_ALPH_INNER_CHAMBER", 2));
	maps.push_back(map);

	map = Map(3, 27, "RUINS_OF_ALPH_INNER_CHAMBER");
	map.addWarp(Warp(10, 13, "RUINS_OF_ALPH_OUTSIDE", 5));
	map.addWarp(Warp(3, 15, "RUINS_OF_ALPH_HO_OH_CHAMBER", 3));
	map.addWarp(Warp(4, 15, "RUINS_OF_ALPH_HO_OH_CHAMBER", 4));
	map.addWarp(Warp(15, 3, "RUINS_OF_ALPH_KABUTO_CHAMBER", 3));
	map.addWarp(Warp(16, 3, "RUINS_OF_ALPH_KABUTO_CHAMBER", 4));
	map.addWarp(Warp(3, 21, "RUINS_OF_ALPH_OMANYTE_CHAMBER", 3));
	map.addWarp(Warp(4, 21, "RUINS_OF_ALPH_OMANYTE_CHAMBER", 4));
	map.addWarp(Warp(15, 24, "RUINS_OF_ALPH_AERODACTYL_CHAMBER", 3));
	map.addWarp(Warp(16, 24, "RUINS_OF_ALPH_AERODACTYL_CHAMBER", 4));
	maps.push_back(map);

	map = Map(3, 24, "RUINS_OF_ALPH_KABUTO_CHAMBER");
	map.addWarp(Warp(3, 9, "RUINS_OF_ALPH_OUTSIDE", 2));
	map.addWarp(Warp(4, 9, "RUINS_OF_ALPH_OUTSIDE", 2));
	map.addWarp(Warp(3, 3, "RUINS_OF_ALPH_INNER_CHAMBER", 4));
	map.addWarp(Warp(4, 3, "RUINS_OF_ALPH_INNER_CHAMBER", 5));
	map.addWarp(Warp(4, 0, "RUINS_OF_ALPH_KABUTO_ITEM_ROOM", 1));
	maps.push_back(map);

	map = Map(3, 30, "RUINS_OF_ALPH_KABUTO_ITEM_ROOM");
	map.addWarp(Warp(3, 9, "RUINS_OF_ALPH_KABUTO_CHAMBER", 5));
	map.addWarp(Warp(4, 9, "RUINS_OF_ALPH_KABUTO_CHAMBER", 5));
	map.addWarp(Warp(3, 1, "RUINS_OF_ALPH_KABUTO_WORD_ROOM", 1));
	map.addWarp(Warp(4, 1, "RUINS_OF_ALPH_KABUTO_WORD_ROOM", 2));
	maps.push_back(map);

	map = Map(3, 34, "RUINS_OF_ALPH_KABUTO_WORD_ROOM");
	map.addWarp(Warp(9, 5, "RUINS_OF_ALPH_KABUTO_ITEM_ROOM", 3));
	map.addWarp(Warp(10, 5, "RUINS_OF_ALPH_KABUTO_ITEM_ROOM", 4));
	map.addWarp(Warp(17, 11, "RUINS_OF_ALPH_INNER_CHAMBER", 4));
	maps.push_back(map);

	map = Map(3, 25, "RUINS_OF_ALPH_OMANYTE_CHAMBER");
	map.addWarp(Warp(3, 9, "RUINS_OF_ALPH_OUTSIDE", 3));
	map.addWarp(Warp(4, 9, "RUINS_OF_ALPH_OUTSIDE", 3));
	map.addWarp(Warp(3, 3, "RUINS_OF_ALPH_INNER_CHAMBER", 6));
	map.addWarp(Warp(4, 3, "RUINS_OF_ALPH_INNER_CHAMBER", 7));
	map.addWarp(Warp(4, 0, "RUINS_OF_ALPH_OMANYTE_ITEM_ROOM", 1));
	maps.push_back(map);

	map = Map(3, 31, "RUINS_OF_ALPH_OMANYTE_ITEM_ROOM");
	map.addWarp(Warp(3, 9, "RUINS_OF_ALPH_OMANYTE_CHAMBER", 5));
	map.addWarp(Warp(4, 9, "RUINS_OF_ALPH_OMANYTE_CHAMBER", 5));
	map.addWarp(Warp(3, 1, "RUINS_OF_ALPH_OMANYTE_WORD_ROOM", 1));
	map.addWarp(Warp(4, 1, "RUINS_OF_ALPH_OMANYTE_WORD_ROOM", 2));
	maps.push_back(map);

	map = Map(3, 35, "RUINS_OF_ALPH_OMANYTE_WORD_ROOM");
	map.addWarp(Warp(9, 7, "RUINS_OF_ALPH_OMANYTE_ITEM_ROOM", 3));
	map.addWarp(Warp(10, 7, "RUINS_OF_ALPH_OMANYTE_ITEM_ROOM", 4));
	map.addWarp(Warp(17, 13, "RUINS_OF_ALPH_INNER_CHAMBER", 6));
	maps.push_back(map);

	map = Map(3, 22, "RUINS_OF_ALPH_OUTSIDE");
	map.addWarp(Warp(2, 17, "RUINS_OF_ALPH_HO_OH_CHAMBER", 1));
	map.addWarp(Warp(14, 7, "RUINS_OF_ALPH_KABUTO_CHAMBER", 1));
	map.addWarp(Warp(2, 29, "RUINS_OF_ALPH_OMANYTE_CHAMBER", 1));
	map.addWarp(Warp(16, 33, "RUINS_OF_ALPH_AERODACTYL_CHAMBER", 1));
	map.addWarp(Warp(10, 13, "RUINS_OF_ALPH_INNER_CHAMBER", 1));
	map.addWarp(Warp(17, 11, "RUINS_OF_ALPH_RESEARCH_CENTER", 1));
	map.addWarp(Warp(6, 19, "UNION_CAVE_B1F", 1));
	map.addWarp(Warp(6, 27, "UNION_CAVE_B1F", 2));
	map.addWarp(Warp(7, 5, "ROUTE_36_RUINS_OF_ALPH_GATE", 3));
	map.addWarp(Warp(13, 20, "ROUTE_32_RUINS_OF_ALPH_GATE", 1));
	map.addWarp(Warp(13, 21, "ROUTE_32_RUINS_OF_ALPH_GATE", 2));
	maps.push_back(map);

	map = Map(3, 28, "RUINS_OF_ALPH_RESEARCH_CENTER");
	map.addWarp(Warp(2, 7, "RUINS_OF_ALPH_OUTSIDE", 6));
	map.addWarp(Warp(3, 7, "RUINS_OF_ALPH_OUTSIDE", 6));
	maps.push_back(map);

	map = Map(3, 90, "SAFARI_ZONE_BETA");
	map.addWarp(Warp(9, 23, "SAFARI_ZONE_FUCHSIA_GATE_BETA", 1));
	map.addWarp(Warp(10, 23, "SAFARI_ZONE_FUCHSIA_GATE_BETA", 2));
	maps.push_back(map);

	map = Map(3, 89, "SAFARI_ZONE_FUCHSIA_GATE_BETA");
	map.addWarp(Warp(4, 0, "SAFARI_ZONE_BETA", 1));
	map.addWarp(Warp(5, 0, "SAFARI_ZONE_BETA", 2));
	map.addWarp(Warp(4, 7, "FUCHSIA_CITY", 7));
	map.addWarp(Warp(5, 7, "FUCHSIA_CITY", 7));
	maps.push_back(map);

	map = Map(17, 7, "SAFARI_ZONE_MAIN_OFFICE");
	map.addWarp(Warp(2, 7, "FUCHSIA_CITY", 2));
	map.addWarp(Warp(3, 7, "FUCHSIA_CITY", 2));
	maps.push_back(map);

	map = Map(17, 12, "SAFARI_ZONE_WARDENS_HOME");
	map.addWarp(Warp(2, 7, "FUCHSIA_CITY", 6));
	map.addWarp(Warp(3, 7, "FUCHSIA_CITY", 6));
	maps.push_back(map);

	map = Map(25, 2, "SAFFRON_CITY");
	map.addWarp(Warp(26, 3, "FIGHTING_DOJO", 1));
	map.addWarp(Warp(34, 3, "SAFFRON_GYM", 1));
	map.addWarp(Warp(25, 11, "SAFFRON_MART", 2));
	map.addWarp(Warp(9, 29, "SAFFRON_POKECENTER_1F", 1));
	map.addWarp(Warp(27, 29, "MR_PSYCHICS_HOUSE", 1));
	map.addWarp(Warp(8, 3, "SAFFRON_MAGNET_TRAIN_STATION", 2));
	map.addWarp(Warp(18, 21, "SILPH_CO_1F", 1));
	map.addWarp(Warp(9, 11, "COPYCATS_HOUSE_1F", 1));
	map.addWarp(Warp(18, 3, "ROUTE_5_SAFFRON_GATE", 3));
	map.addWarp(Warp(0, 24, "ROUTE_7_SAFFRON_GATE", 3));
	map.addWarp(Warp(0, 25, "ROUTE_7_SAFFRON_GATE", 4));
	map.addWarp(Warp(16, 33, "ROUTE_6_SAFFRON_GATE", 1));
	map.addWarp(Warp(17, 33, "ROUTE_6_SAFFRON_GATE", 2));
	map.addWarp(Warp(39, 22, "ROUTE_8_SAFFRON_GATE", 1));
	map.addWarp(Warp(39, 23, "ROUTE_8_SAFFRON_GATE", 2));
	maps.push_back(map);

	map = Map(25, 4, "SAFFRON_GYM");
	map.addWarp(Warp(8, 17, "SAFFRON_CITY", 2));
	map.addWarp(Warp(9, 17, "SAFFRON_CITY", 2));
	map.addWarp(Warp(11, 15, "SAFFRON_GYM", 18));
	map.addWarp(Warp(19, 15, "SAFFRON_GYM", 19));
	map.addWarp(Warp(19, 11, "SAFFRON_GYM", 20));
	map.addWarp(Warp(1, 11, "SAFFRON_GYM", 21));
	map.addWarp(Warp(5, 3, "SAFFRON_GYM", 22));
	map.addWarp(Warp(11, 5, "SAFFRON_GYM", 23));
	map.addWarp(Warp(1, 15, "SAFFRON_GYM", 24));
	map.addWarp(Warp(19, 3, "SAFFRON_GYM", 25));
	map.addWarp(Warp(15, 17, "SAFFRON_GYM", 26));
	map.addWarp(Warp(5, 17, "SAFFRON_GYM", 27));
	map.addWarp(Warp(5, 9, "SAFFRON_GYM", 28));
	map.addWarp(Warp(9, 3, "SAFFRON_GYM", 29));
	map.addWarp(Warp(15, 9, "SAFFRON_GYM", 30));
	map.addWarp(Warp(15, 5, "SAFFRON_GYM", 31));
	map.addWarp(Warp(1, 5, "SAFFRON_GYM", 32));
	map.addWarp(Warp(19, 17, "SAFFRON_GYM", 3));
	map.addWarp(Warp(19, 9, "SAFFRON_GYM", 4));
	map.addWarp(Warp(1, 9, "SAFFRON_GYM", 5));
	map.addWarp(Warp(5, 5, "SAFFRON_GYM", 6));
	map.addWarp(Warp(11, 3, "SAFFRON_GYM", 7));
	map.addWarp(Warp(1, 17, "SAFFRON_GYM", 8));
	map.addWarp(Warp(19, 5, "SAFFRON_GYM", 9));
	map.addWarp(Warp(15, 15, "SAFFRON_GYM", 10));
	map.addWarp(Warp(5, 15, "SAFFRON_GYM", 11));
	map.addWarp(Warp(5, 11, "SAFFRON_GYM", 12));
	map.addWarp(Warp(9, 5, "SAFFRON_GYM", 13));
	map.addWarp(Warp(15, 11, "SAFFRON_GYM", 14));
	map.addWarp(Warp(15, 3, "SAFFRON_GYM", 15));
	map.addWarp(Warp(1, 3, "SAFFRON_GYM", 16));
	map.addWarp(Warp(11, 9, "SAFFRON_GYM", 17));
	maps.push_back(map);

	map = Map(25, 9, "SAFFRON_MAGNET_TRAIN_STATION");
	map.addWarp(Warp(8, 17, "SAFFRON_CITY", 6));
	map.addWarp(Warp(9, 17, "SAFFRON_CITY", 6));
	map.addWarp(Warp(6, 5, "GOLDENROD_MAGNET_TRAIN_STATION", 4));
	map.addWarp(Warp(11, 5, "GOLDENROD_MAGNET_TRAIN_STATION", 3));
	maps.push_back(map);

	map = Map(25, 5, "SAFFRON_MART");
	map.addWarp(Warp(2, 7, "SAFFRON_CITY", 3));
	map.addWarp(Warp(3, 7, "SAFFRON_CITY", 3));
	maps.push_back(map);

	map = Map(25, 6, "SAFFRON_POKECENTER_1F");
	map.addWarp(Warp(3, 7, "SAFFRON_CITY", 4));
	map.addWarp(Warp(4, 7, "SAFFRON_CITY", 4));
	map.addWarp(Warp(0, 7, "POKECENTER_2F", 1));
	maps.push_back(map);

	map = Map(25, 7, "SAFFRON_POKECENTER_2F_BETA");
	map.addWarp(Warp(0, 7, "SAFFRON_POKECENTER_1F", 3));
	maps.push_back(map);

	map = Map(6, 4, "SEAFOAM_GYM");
	map.addWarp(Warp(5, 5, "ROUTE_20", 1));
	maps.push_back(map);

	map = Map(25, 10, "SILPH_CO_1F");
	map.addWarp(Warp(2, 7, "SAFFRON_CITY", 7));
	map.addWarp(Warp(3, 7, "SAFFRON_CITY", 7));
	maps.push_back(map);

	map = Map(3, 77, "SILVER_CAVE_ITEM_ROOMS");
	map.addWarp(Warp(13, 3, "SILVER_CAVE_ROOM_2", 3));
	map.addWarp(Warp(7, 15, "SILVER_CAVE_ROOM_2", 4));
	maps.push_back(map);

	map = Map(19, 2, "SILVER_CAVE_OUTSIDE");
	map.addWarp(Warp(23, 19, "SILVER_CAVE_POKECENTER_1F", 1));
	map.addWarp(Warp(18, 11, "SILVER_CAVE_ROOM_1", 1));
	maps.push_back(map);

	map = Map(19, 3, "SILVER_CAVE_POKECENTER_1F");
	map.addWarp(Warp(3, 7, "SILVER_CAVE_OUTSIDE", 1));
	map.addWarp(Warp(4, 7, "SILVER_CAVE_OUTSIDE", 1));
	map.addWarp(Warp(0, 7, "POKECENTER_2F", 1));
	maps.push_back(map);

	map = Map(3, 74, "SILVER_CAVE_ROOM_1");
	map.addWarp(Warp(9, 33, "SILVER_CAVE_OUTSIDE", 2));
	map.addWarp(Warp(15, 1, "SILVER_CAVE_ROOM_2", 1));
	maps.push_back(map);

	map = Map(3, 75, "SILVER_CAVE_ROOM_2");
	map.addWarp(Warp(17, 31, "SILVER_CAVE_ROOM_1", 2));
	map.addWarp(Warp(11, 5, "SILVER_CAVE_ROOM_3", 1));
	map.addWarp(Warp(13, 21, "SILVER_CAVE_ITEM_ROOMS", 1));
	map.addWarp(Warp(23, 3, "SILVER_CAVE_ITEM_ROOMS", 2));
	maps.push_back(map);

	map = Map(3, 76, "SILVER_CAVE_ROOM_3");
	map.addWarp(Warp(9, 33, "SILVER_CAVE_ROOM_2", 2));
	maps.push_back(map);

	map = Map(3, 40, "SLOWPOKE_WELL_B1F");
	map.addWarp(Warp(17, 15, "AZALEA_TOWN", 6));
	map.addWarp(Warp(7, 11, "SLOWPOKE_WELL_B2F", 1));
	maps.push_back(map);

	map = Map(3, 41, "SLOWPOKE_WELL_B2F");
	map.addWarp(Warp(9, 11, "SLOWPOKE_WELL_B1F", 2));
	maps.push_back(map);

	map = Map(18, 11, "SOUL_HOUSE");
	map.addWarp(Warp(4, 7, "LAVENDER_TOWN", 6));
	map.addWarp(Warp(5, 7, "LAVENDER_TOWN", 6));
	maps.push_back(map);

	map = Map(3, 1, "SPROUT_TOWER_1F");
	map.addWarp(Warp(9, 15, "VIOLET_CITY", 7));
	map.addWarp(Warp(10, 15, "VIOLET_CITY", 7));
	map.addWarp(Warp(6, 4, "SPROUT_TOWER_2F", 1));
	map.addWarp(Warp(2, 6, "SPROUT_TOWER_2F", 2));
	map.addWarp(Warp(17, 3, "SPROUT_TOWER_2F", 3));
	maps.push_back(map);

	map = Map(3, 2, "SPROUT_TOWER_2F");
	map.addWarp(Warp(6, 4, "SPROUT_TOWER_1F", 3));
	map.addWarp(Warp(2, 6, "SPROUT_TOWER_1F", 4));
	map.addWarp(Warp(17, 3, "SPROUT_TOWER_1F", 5));
	map.addWarp(Warp(10, 14, "SPROUT_TOWER_3F", 1));
	maps.push_back(map);

	map = Map(3, 3, "SPROUT_TOWER_3F");
	map.addWarp(Warp(10, 14, "SPROUT_TOWER_2F", 4));
	maps.push_back(map);

	map = Map(3, 49, "TEAM_ROCKET_BASE_B1F");
	map.addWarp(Warp(27, 2, "MAHOGANY_MART_1F", 3));
	map.addWarp(Warp(3, 14, "TEAM_ROCKET_BASE_B2F", 1));
	map.addWarp(Warp(5, 15, "TEAM_ROCKET_BASE_B1F", 4));
	map.addWarp(Warp(25, 2, "TEAM_ROCKET_BASE_B1F", 3));
	maps.push_back(map);

	map = Map(3, 50, "TEAM_ROCKET_BASE_B2F");
	map.addWarp(Warp(3, 14, "TEAM_ROCKET_BASE_B1F", 2));
	map.addWarp(Warp(3, 2, "TEAM_ROCKET_BASE_B3F", 1));
	map.addWarp(Warp(27, 2, "TEAM_ROCKET_BASE_B3F", 2));
	map.addWarp(Warp(3, 6, "TEAM_ROCKET_BASE_B3F", 3));
	map.addWarp(Warp(27, 14, "TEAM_ROCKET_BASE_B3F", 4));
	maps.push_back(map);

	map = Map(3, 51, "TEAM_ROCKET_BASE_B3F");
	map.addWarp(Warp(3, 2, "TEAM_ROCKET_BASE_B2F", 2));
	map.addWarp(Warp(27, 2, "TEAM_ROCKET_BASE_B2F", 3));
	map.addWarp(Warp(3, 6, "TEAM_ROCKET_BASE_B2F", 4));
	map.addWarp(Warp(27, 14, "TEAM_ROCKET_BASE_B2F", 5));
	maps.push_back(map);

	map = Map(20, 4, "TIME_CAPSULE");
	map.addWarp(Warp(4, 7, "POKECENTER_2F", 4));
	map.addWarp(Warp(5, 7, "POKECENTER_2F", 4));
	maps.push_back(map);

	map = Map(3, 4, "TIN_TOWER_1F");
	map.addWarp(Warp(9, 15, "ECRUTEAK_CITY", 12));
	map.addWarp(Warp(10, 15, "ECRUTEAK_CITY", 12));
	map.addWarp(Warp(10, 2, "TIN_TOWER_2F", 2));
	maps.push_back(map);

	map = Map(3, 5, "TIN_TOWER_2F");
	map.addWarp(Warp(10, 14, "TIN_TOWER_3F", 1));
	map.addWarp(Warp(10, 2, "TIN_TOWER_1F", 3));
	maps.push_back(map);

	map = Map(3, 6, "TIN_TOWER_3F");
	map.addWarp(Warp(10, 14, "TIN_TOWER_2F", 1));
	map.addWarp(Warp(16, 2, "TIN_TOWER_4F", 2));
	maps.push_back(map);

	map = Map(3, 7, "TIN_TOWER_4F");
	map.addWarp(Warp(2, 4, "TIN_TOWER_5F", 2));
	map.addWarp(Warp(16, 2, "TIN_TOWER_3F", 2));
	map.addWarp(Warp(2, 14, "TIN_TOWER_5F", 3));
	map.addWarp(Warp(17, 15, "TIN_TOWER_5F", 4));
	maps.push_back(map);

	map = Map(3, 8, "TIN_TOWER_5F");
	map.addWarp(Warp(11, 15, "TIN_TOWER_6F", 2));
	map.addWarp(Warp(2, 4, "TIN_TOWER_4F", 1));
	map.addWarp(Warp(2, 14, "TIN_TOWER_4F", 3));
	map.addWarp(Warp(17, 15, "TIN_TOWER_4F", 4));
	maps.push_back(map);

	map = Map(3, 9, "TIN_TOWER_6F");
	map.addWarp(Warp(3, 9, "TIN_TOWER_7F", 1));
	map.addWarp(Warp(11, 15, "TIN_TOWER_5F", 1));
	maps.push_back(map);

	map = Map(3, 10, "TIN_TOWER_7F");
	map.addWarp(Warp(3, 9, "TIN_TOWER_6F", 1));
	map.addWarp(Warp(10, 15, "TIN_TOWER_8F", 1));
	map.addWarp(Warp(12, 7, "TIN_TOWER_7F", 4));
	map.addWarp(Warp(8, 3, "TIN_TOWER_7F", 3));
	map.addWarp(Warp(6, 9, "TIN_TOWER_9F", 5));
	maps.push_back(map);

	map = Map(3, 11, "TIN_TOWER_8F");
	map.addWarp(Warp(2, 5, "TIN_TOWER_7F", 2));
	map.addWarp(Warp(2, 11, "TIN_TOWER_9F", 1));
	map.addWarp(Warp(16, 7, "TIN_TOWER_9F", 2));
	map.addWarp(Warp(10, 3, "TIN_TOWER_9F", 3));
	map.addWarp(Warp(14, 15, "TIN_TOWER_9F", 6));
	map.addWarp(Warp(6, 9, "TIN_TOWER_9F", 7));
	maps.push_back(map);

	map = Map(3, 12, "TIN_TOWER_9F");
	map.addWarp(Warp(12, 3, "TIN_TOWER_8F", 2));
	map.addWarp(Warp(2, 5, "TIN_TOWER_8F", 3));
	map.addWarp(Warp(12, 7, "TIN_TOWER_8F", 4));
	map.addWarp(Warp(7, 9, "TIN_TOWER_ROOF", 1));
	map.addWarp(Warp(16, 7, "TIN_TOWER_7F", 5));
	map.addWarp(Warp(6, 13, "TIN_TOWER_8F", 5));
	map.addWarp(Warp(8, 13, "TIN_TOWER_8F", 6));
	maps.push_back(map);

	map = Map(15, 12, "TIN_TOWER_ROOF");
	map.addWarp(Warp(9, 13, "TIN_TOWER_9F", 4));
	maps.push_back(map);

	map = Map(3, 83, "TOHJO_FALLS");
	map.addWarp(Warp(13, 15, "ROUTE_27", 2));
	map.addWarp(Warp(25, 15, "ROUTE_27", 3));
	maps.push_back(map);

	map = Map(20, 2, "TRADE_CENTER");
	map.addWarp(Warp(4, 7, "POKECENTER_2F", 2));
	map.addWarp(Warp(5, 7, "POKECENTER_2F", 2));
	maps.push_back(map);

	map = Map(23, 6, "TRAINER_HOUSE_1F");
	map.addWarp(Warp(2, 13, "VIRIDIAN_CITY", 3));
	map.addWarp(Warp(3, 13, "VIRIDIAN_CITY", 3));
	map.addWarp(Warp(8, 2, "TRAINER_HOUSE_B1F", 1));
	maps.push_back(map);

	map = Map(23, 7, "TRAINER_HOUSE_B1F");
	map.addWarp(Warp(9, 4, "TRAINER_HOUSE_1F", 3));
	maps.push_back(map);

	map = Map(3, 86, "UNDERGROUND_PATH");
	map.addWarp(Warp(3, 2, "ROUTE_5_UNDERGROUND_PATH_ENTRANCE", 3));
	map.addWarp(Warp(3, 24, "ROUTE_6_UNDERGROUND_PATH_ENTRANCE", 3));
	maps.push_back(map);

	map = Map(3, 37, "UNION_CAVE_1F");
	map.addWarp(Warp(5, 19, "UNION_CAVE_B1F", 3));
	map.addWarp(Warp(3, 33, "UNION_CAVE_B1F", 4));
	map.addWarp(Warp(17, 31, "ROUTE_33", 1));
	map.addWarp(Warp(17, 3, "ROUTE_32", 4));
	maps.push_back(map);

	map = Map(3, 38, "UNION_CAVE_B1F");
	map.addWarp(Warp(3, 3, "RUINS_OF_ALPH_OUTSIDE", 7));
	map.addWarp(Warp(3, 11, "RUINS_OF_ALPH_OUTSIDE", 8));
	map.addWarp(Warp(7, 19, "UNION_CAVE_1F", 1));
	map.addWarp(Warp(3, 33, "UNION_CAVE_1F", 2));
	map.addWarp(Warp(17, 31, "UNION_CAVE_B2F", 1));
	maps.push_back(map);

	map = Map(3, 39, "UNION_CAVE_B2F");
	map.addWarp(Warp(5, 3, "UNION_CAVE_B1F", 5));
	maps.push_back(map);

	map = Map(12, 1, "VERMILION_CITY");
	map.addWarp(Warp(5, 5, "VERMILION_FISHING_SPEECH_HOUSE", 1));
	map.addWarp(Warp(9, 5, "VERMILION_POKECENTER_1F", 1));
	map.addWarp(Warp(7, 13, "POKEMON_FAN_CLUB", 1));
	map.addWarp(Warp(13, 13, "VERMILION_MAGNET_TRAIN_SPEECH_HOUSE", 1));
	map.addWarp(Warp(21, 13, "VERMILION_MART", 2));
	map.addWarp(Warp(21, 17, "VERMILION_DIGLETTS_CAVE_SPEECH_HOUSE", 1));
	map.addWarp(Warp(10, 19, "VERMILION_GYM", 1));
	map.addWarp(Warp(19, 31, "VERMILION_PORT_PASSAGE", 1));
	map.addWarp(Warp(20, 31, "VERMILION_PORT_PASSAGE", 2));
	map.addWarp(Warp(34, 7, "DIGLETTS_CAVE", 1));
	maps.push_back(map);

	map = Map(12, 1, "VERMILION_DIGLETTS_CAVE_SPEECH_HOUSE");
	map.addWarp(Warp(2, 7, "VERMILION_CITY", 6));
	map.addWarp(Warp(3, 7, "VERMILION_CITY", 6));
	maps.push_back(map);

	map = Map(12, 1, "VERMILION_FISHING_SPEECH_HOUSE");
	map.addWarp(Warp(2, 7, "VERMILION_CITY", 1));
	map.addWarp(Warp(3, 7, "VERMILION_CITY", 1));
	maps.push_back(map);

	map = Map(12, 1, "VERMILION_GYM");
	map.addWarp(Warp(4, 17, "VERMILION_CITY", 7));
	map.addWarp(Warp(5, 17, "VERMILION_CITY", 7));
	maps.push_back(map);

	map = Map(12, 1, "VERMILION_MAGNET_TRAIN_SPEECH_HOUSE");
	map.addWarp(Warp(2, 7, "VERMILION_CITY", 4));
	map.addWarp(Warp(3, 7, "VERMILION_CITY", 4));
	maps.push_back(map);

	map = Map(12, 1, "VERMILION_MART");
	map.addWarp(Warp(2, 7, "VERMILION_CITY", 5));
	map.addWarp(Warp(3, 7, "VERMILION_CITY", 5));
	maps.push_back(map);

	map = Map(12, 1, "VERMILION_POKECENTER_1F");
	map.addWarp(Warp(3, 7, "VERMILION_CITY", 2));
	map.addWarp(Warp(4, 7, "VERMILION_CITY", 2));
	map.addWarp(Warp(0, 7, "POKECENTER_2F", 1));
	maps.push_back(map);

	map = Map(12, 1, "VERMILION_POKECENTER_2F_BETA");
	map.addWarp(Warp(0, 7, "VERMILION_POKECENTER_1F", 3));
	maps.push_back(map);

	map = Map(15, 2, "VERMILION_PORT");
	map.addWarp(Warp(9, 5, "VERMILION_PORT_PASSAGE", 5));
	map.addWarp(Warp(7, 17, "FAST_SHIP_1F", 1));
	maps.push_back(map);

	map = Map(15, 9, "VERMILION_PORT_PASSAGE");
	map.addWarp(Warp(15, 0, "VERMILION_CITY", 8));
	map.addWarp(Warp(16, 0, "VERMILION_CITY", 9));
	map.addWarp(Warp(15, 4, "VERMILION_PORT_PASSAGE", 4));
	map.addWarp(Warp(3, 2, "VERMILION_PORT_PASSAGE", 3));
	map.addWarp(Warp(3, 14, "VERMILION_PORT", 1));
	maps.push_back(map);

	map = Map(3, 91, "VICTORY_ROAD");
	map.addWarp(Warp(9, 67, "VICTORY_ROAD_GATE", 5));
	map.addWarp(Warp(1, 49, "VICTORY_ROAD", 3));
	map.addWarp(Warp(1, 35, "VICTORY_ROAD", 2));
	map.addWarp(Warp(13, 31, "VICTORY_ROAD", 5));
	map.addWarp(Warp(13, 17, "VICTORY_ROAD", 4));
	map.addWarp(Warp(17, 33, "VICTORY_ROAD", 7));
	map.addWarp(Warp(17, 19, "VICTORY_ROAD", 6));
	map.addWarp(Warp(0, 11, "VICTORY_ROAD", 9));
	map.addWarp(Warp(0, 27, "VICTORY_ROAD", 8));
	map.addWarp(Warp(13, 5, "ROUTE_23", 3));
	maps.push_back(map);

	map = Map(23, 13, "VICTORY_ROAD_GATE");
	map.addWarp(Warp(17, 7, "ROUTE_22", 1));
	map.addWarp(Warp(18, 7, "ROUTE_22", 1));
	map.addWarp(Warp(9, 17, "ROUTE_26", 1));
	map.addWarp(Warp(10, 17, "ROUTE_26", 1));
	map.addWarp(Warp(9, 0, "VICTORY_ROAD", 1));
	map.addWarp(Warp(10, 0, "VICTORY_ROAD", 1));
	map.addWarp(Warp(1, 7, "ROUTE_28", 2));
	map.addWarp(Warp(2, 7, "ROUTE_28", 2));
	maps.push_back(map);

	map = Map(10, 5, "VIOLET_CITY");
	map.addWarp(Warp(9, 17, "VIOLET_MART", 2));
	map.addWarp(Warp(18, 17, "VIOLET_GYM", 1));
	map.addWarp(Warp(30, 17, "EARLS_POKEMON_ACADEMY", 1));
	map.addWarp(Warp(3, 15, "VIOLET_NICKNAME_SPEECH_HOUSE", 1));
	map.addWarp(Warp(31, 25, "VIOLET_POKECENTER_1F", 1));
	map.addWarp(Warp(21, 29, "VIOLET_KYLES_HOUSE", 1));
	map.addWarp(Warp(23, 5, "SPROUT_TOWER_1F", 1));
	map.addWarp(Warp(39, 24, "ROUTE_31_VIOLET_GATE", 1));
	map.addWarp(Warp(39, 25, "ROUTE_31_VIOLET_GATE", 2));
	maps.push_back(map);

	map = Map(10, 7, "VIOLET_GYM");
	map.addWarp(Warp(4, 15, "VIOLET_CITY", 2));
	map.addWarp(Warp(5, 15, "VIOLET_CITY", 2));
	maps.push_back(map);

	map = Map(10, 11, "VIOLET_KYLES_HOUSE");
	map.addWarp(Warp(3, 7, "VIOLET_CITY", 6));
	map.addWarp(Warp(4, 7, "VIOLET_CITY", 6));
	maps.push_back(map);

	map = Map(10, 6, "VIOLET_MART");
	map.addWarp(Warp(2, 7, "VIOLET_CITY", 1));
	map.addWarp(Warp(3, 7, "VIOLET_CITY", 1));
	maps.push_back(map);

	map = Map(10, 9, "VIOLET_NICKNAME_SPEECH_HOUSE");
	map.addWarp(Warp(3, 7, "VIOLET_CITY", 4));
	map.addWarp(Warp(4, 7, "VIOLET_CITY", 4));
	maps.push_back(map);

	map = Map(10, 10, "VIOLET_POKECENTER_1F");
	map.addWarp(Warp(3, 7, "VIOLET_CITY", 5));
	map.addWarp(Warp(4, 7, "VIOLET_CITY", 5));
	map.addWarp(Warp(0, 7, "POKECENTER_2F", 1));
	maps.push_back(map);

	map = Map(23, 3, "VIRIDIAN_CITY");
	map.addWarp(Warp(32, 7, "VIRIDIAN_GYM", 1));
	map.addWarp(Warp(21, 9, "VIRIDIAN_NICKNAME_SPEECH_HOUSE", 1));
	map.addWarp(Warp(23, 15, "TRAINER_HOUSE_1F", 1));
	map.addWarp(Warp(29, 19, "VIRIDIAN_MART", 2));
	map.addWarp(Warp(23, 25, "VIRIDIAN_POKECENTER_1F", 1));
	maps.push_back(map);

	map = Map(23, 4, "VIRIDIAN_GYM");
	map.addWarp(Warp(4, 17, "VIRIDIAN_CITY", 1));
	map.addWarp(Warp(5, 17, "VIRIDIAN_CITY", 1));
	maps.push_back(map);

	map = Map(23, 8, "VIRIDIAN_MART");
	map.addWarp(Warp(2, 7, "VIRIDIAN_CITY", 4));
	map.addWarp(Warp(3, 7, "VIRIDIAN_CITY", 4));
	maps.push_back(map);

	map = Map(23, 5, "VIRIDIAN_NICKNAME_SPEECH_HOUSE");
	map.addWarp(Warp(2, 7, "VIRIDIAN_CITY", 2));
	map.addWarp(Warp(3, 7, "VIRIDIAN_CITY", 2));
	maps.push_back(map);

	map = Map(23, 9, "VIRIDIAN_POKECENTER_1F");
	map.addWarp(Warp(3, 7, "VIRIDIAN_CITY", 5));
	map.addWarp(Warp(4, 7, "VIRIDIAN_CITY", 5));
	map.addWarp(Warp(0, 7, "POKECENTER_2F", 1));
	maps.push_back(map);

	map = Map(23, 10, "VIRIDIAN_POKECENTER_2F_BETA");
	map.addWarp(Warp(0, 7, "VIRIDIAN_POKECENTER_1F", 3));
	maps.push_back(map);

	map = Map(3, 71, "WHIRL_ISLAND_B1F");
	map.addWarp(Warp(5, 5, "WHIRL_ISLAND_NW", 2));
	map.addWarp(Warp(35, 3, "WHIRL_ISLAND_NE", 2));
	map.addWarp(Warp(29, 9, "WHIRL_ISLAND_NE", 3));
	map.addWarp(Warp(9, 31, "WHIRL_ISLAND_SW", 3));
	map.addWarp(Warp(23, 31, "WHIRL_ISLAND_SW", 2));
	map.addWarp(Warp(31, 29, "WHIRL_ISLAND_SE", 2));
	map.addWarp(Warp(25, 21, "WHIRL_ISLAND_B2F", 1));
	map.addWarp(Warp(13, 27, "WHIRL_ISLAND_B2F", 2));
	map.addWarp(Warp(17, 21, "WHIRL_ISLAND_CAVE", 1));
	maps.push_back(map);

	map = Map(3, 72, "WHIRL_ISLAND_B2F");
	map.addWarp(Warp(11, 5, "WHIRL_ISLAND_B1F", 7));
	map.addWarp(Warp(7, 11, "WHIRL_ISLAND_B1F", 8));
	map.addWarp(Warp(7, 25, "WHIRL_ISLAND_LUGIA_CHAMBER", 1));
	map.addWarp(Warp(13, 31, "WHIRL_ISLAND_SW", 5));
	maps.push_back(map);

	map = Map(3, 69, "WHIRL_ISLAND_CAVE");
	map.addWarp(Warp(7, 5, "WHIRL_ISLAND_B1F", 9));
	map.addWarp(Warp(3, 13, "WHIRL_ISLAND_NW", 4));
	maps.push_back(map);

	map = Map(3, 73, "WHIRL_ISLAND_LUGIA_CHAMBER");
	map.addWarp(Warp(9, 13, "WHIRL_ISLAND_B2F", 3));
	maps.push_back(map);

	map = Map(3, 67, "WHIRL_ISLAND_NE");
	map.addWarp(Warp(3, 13, "ROUTE_41", 2));
	map.addWarp(Warp(17, 3, "WHIRL_ISLAND_B1F", 2));
	map.addWarp(Warp(13, 11, "WHIRL_ISLAND_B1F", 3));
	maps.push_back(map);

	map = Map(3, 66, "WHIRL_ISLAND_NW");
	map.addWarp(Warp(5, 7, "ROUTE_41", 1));
	map.addWarp(Warp(5, 3, "WHIRL_ISLAND_B1F", 1));
	map.addWarp(Warp(3, 15, "WHIRL_ISLAND_SW", 4));
	map.addWarp(Warp(7, 15, "WHIRL_ISLAND_CAVE", 2));
	maps.push_back(map);

	map = Map(3, 70, "WHIRL_ISLAND_SE");
	map.addWarp(Warp(5, 13, "ROUTE_41", 4));
	map.addWarp(Warp(5, 3, "WHIRL_ISLAND_B1F", 6));
	maps.push_back(map);

	map = Map(3, 68, "WHIRL_ISLAND_SW");
	map.addWarp(Warp(5, 7, "ROUTE_41", 3));
	map.addWarp(Warp(17, 3, "WHIRL_ISLAND_B1F", 5));
	map.addWarp(Warp(3, 3, "WHIRL_ISLAND_B1F", 4));
	map.addWarp(Warp(3, 15, "WHIRL_ISLAND_NW", 3));
	map.addWarp(Warp(17, 15, "WHIRL_ISLAND_B2F", 4));
	maps.push_back(map);

	map = Map(16, 3, "WILLS_ROOM");
	map.addWarp(Warp(5, 17, "INDIGO_PLATEAU_POKECENTER_1F", 4));
	map.addWarp(Warp(4, 2, "KOGAS_ROOM", 1));
	map.addWarp(Warp(5, 2, "KOGAS_ROOM", 2));
	maps.push_back(map);

	map = Map(4, 2, "WISE_TRIOS_ROOM");
	map.addWarp(Warp(7, 4, "ECRUTEAK_CITY", 4));
	map.addWarp(Warp(7, 5, "ECRUTEAK_CITY", 5));
	map.addWarp(Warp(1, 4, "ECRUTEAK_TIN_TOWER_ENTRANCE", 5));
	maps.push_back(map);

	map = Map(13, 1, "ROUTE_1");
	maps.push_back(map);

	map = Map(12, 2, "ROUTE_11");
	maps.push_back(map);

	map = Map(17, 1, "ROUTE_13");
	maps.push_back(map);

	map = Map(17, 2, "ROUTE_14");
	maps.push_back(map);

	map = Map(6, 7, "ROUTE_21");
	maps.push_back(map);

	map = Map(7, 15, "ROUTE_24");
	maps.push_back(map);

	map = Map(10, 4, "ROUTE_37");
	maps.push_back(map);

}