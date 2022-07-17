#pragma once

#include <windows.h>
#include <vector>
#include <string>

#include "wram_positions.hpp"

// To ensure correct resolution of symbols, add Psapi.lib to TARGETLIBS
#pragma comment(lib, "psapi.lib")

#define DEBUG

const uint8_t POKEGEAR_NBYTES = 1;
const uint8_t POKEDEX_NBYTES = 1;
const uint8_t BADGES_NBYTES = 2;
const uint8_t TMHM_NBYTES = 57;
const uint8_t KEYITEMS_NBYTES = 26;
const uint8_t ITEMS_NBYTES = 41;
const uint8_t MAPS_NBYTES = 5;

class Map;
class Warp;
class RandoState;

struct gambatte_wram_info {
	DWORD processId;
	uint32_t base_address;
};

std::string convertToJson(bool b);

class Warp {
private:
	uint8_t x;
	uint8_t y;
	std::string vanilla_destination;
	uint8_t warp_destination_id;
	Map* map_destination = NULL;
public:
	Warp(uint8_t y, uint8_t x, std::string vanilla_destination, uint8_t warp_destination);
	void add_map_destination(Map* map_destination) { this->map_destination = map_destination; }
	Map* get_map_destination() { return this->map_destination; }
	uint8_t getX() { return this->x; };
	uint8_t getY() { return this->y; };
	std::string get_vanilla_destination() { return this->vanilla_destination; };
	uint8_t get_warp_destination_id() { return this->warp_destination_id; };
};

class Map {
private:
	uint8_t map_group;
	uint8_t map_const;
	std::string name;
	std::vector<Warp> warps;

public:
	Map(uint8_t map_group, uint8_t map_const, std::string name);
	void addWarp(Warp warp) { this->warps.push_back(warp); }
	void setWarp(uint8_t warp, Map* destination);
	uint8_t getMapGroup() { return this->map_group; }
	uint8_t getMapConst() { return this->map_const; }
	std::string getName() { return this->name; }
	std::vector<Warp> getWarps() { return this->warps; }
	int8_t getWarpId(uint8_t x, uint8_t y);

	friend std::ostream& operator<<(std::ostream& stream, const Map& status);
};

class RandoState {
private:
	DWORD processId = 0;
	uint32_t base_address = 0;

	// Sacrifice memory for the sake of speed of execution
	std::string items[255];
	std::string tmsHmsOwned[8] = { "", "", "", "", "", "", "", "" };
	std::string kantoBadges[8] = { "", "", "", "", "", "", "", "" };
	std::string johtoBadges[8] = { "", "", "", "", "", "", "", "" };
	std::string pokegear[4] = { "", "", "", "" };
	std::string hasPokedex = "";
	std::string hasUnownDex = "";

	std::vector<Map> maps;
	Map* currentMap = NULL;
	uint8_t x = 0;
	uint8_t y = 0;

	uint8_t getByte(LPVOID* buffer, uint8_t position);

	gambatte_wram_info GetBaseAddress(DWORD processId, TCHAR* processName);

	// Map CRUDs

	void initialiseMaps();

	Map* getMap(uint8_t map_group, uint8_t map_const);

	// Item rando updates

	void updateBadges(HANDLE hProcess);

	void updateTMsAndHMs(HANDLE hProcess);

	void updatePokegear(HANDLE hProcess);

	void updatePokedex(HANDLE hProcess);

	void updateKeyItems(HANDLE hProcess);

	void updateItems(HANDLE hProcess);

	void updateMapInfo(HANDLE hProcess);

public:
	RandoState();

	void updateStatus();

	friend std::ostream& operator<<(std::ostream& stream, const RandoState& status);
};