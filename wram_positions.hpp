#pragma once

const uint16_t gambatte_offset = 0x20;
const uint16_t johto_badges = 0x1857;
const uint16_t kanto_badges = 0x1858;
const uint16_t tm_list = 0x1859;
const uint16_t number_of_items = 0x1892;
const uint16_t item_list = 0x1893;
const uint16_t number_of_key_items = 0x18BC;
const uint16_t key_item_list = 0x188D;
const uint16_t pokedex_byte = 0x184C;
const uint16_t pokegear_byte = 0x1957;

const uint16_t y_coordinate = 0x1CB7;
const uint16_t x_coordinate = 0x1CB8;
const uint16_t map_number = 0x1CB6;
const uint16_t map_bank = 0x1CB5;
const uint16_t warp_number = 0x1CB4;

typedef uint8_t ITEM;
const ITEM SQUIRTBOTTLE = 175;
const ITEM SECRETPOTION = 67;
const ITEM CARD_KEY = 127;
const ITEM S_S_TICKET = 68;
const ITEM PASS = 134;
const ITEM MACHINE_PART = 128;
const ITEM CLEAR_BELL = 70;
const ITEM RAINBOW_WING = 178;
const ITEM SILVER_WING = 71;
const ITEM BASEMENT_KEY = 133;
const ITEM LOST_ITEM = 130;
const ITEM RED_SCALE = 66;
const ITEM MYSTERY_EGG = 69;
const ITEM BICYCLE = 7;
const ITEM BLUE_CARD = 116;
const ITEM COIN_CASE = 54;
const ITEM ITEMFINDER = 55;
const ITEM OLD_ROD = 58;
const ITEM GOOD_ROD = 59;
const ITEM SUPER_ROD = 61;
const ITEM WATER_STONE = 24;

typedef uint8_t JOHTO_BADGE;
const JOHTO_BADGE ZEPHYRBADGE = 0;
const JOHTO_BADGE HIVEBADGE = 1;
const JOHTO_BADGE PLAINBADGE = 2;
const JOHTO_BADGE FOGBADGE = 3;
const JOHTO_BADGE MINERALBADGE = 4;
const JOHTO_BADGE STORMBADGE = 5;
const JOHTO_BADGE GLACIERBADGE = 6;
const JOHTO_BADGE RISINGBADGE = 7;

typedef uint8_t KANTO_BADGE;
const KANTO_BADGE BOULDERBADGE = 0;
const KANTO_BADGE CASCADEBADGE = 1;
const KANTO_BADGE THUNDERBADGE = 2;
const KANTO_BADGE RAINBOWBADGE = 3;
const KANTO_BADGE SOULBADGE = 4;
const KANTO_BADGE MARSHBADGE = 5;
const KANTO_BADGE VOLCANOBADGE = 6;
const KANTO_BADGE EARTHBADGE = 7;

typedef uint8_t TM_HM_ID;
const TM_HM_ID TM08_ID = 7;
const TM_HM_ID HM01_ID = 50;
const TM_HM_ID HM02_ID = 51;
const TM_HM_ID HM03_ID = 52;
const TM_HM_ID HM04_ID = 53;
const TM_HM_ID HM05_ID = 54;
const TM_HM_ID HM06_ID = 55;
const TM_HM_ID HM07_ID = 56;

typedef uint8_t TMHM;
const TMHM HM01 = 0;
const TMHM HM02 = 1;
const TMHM HM03 = 2;
const TMHM HM04 = 3;
const TMHM HM05 = 4;
const TMHM HM06 = 5;
const TMHM HM07 = 6;
const TMHM TM08 = 7;

typedef uint8_t POKEDEX_BIT;
const POKEDEX_BIT POKEDEX = 0;
const POKEDEX_BIT UNOWN_DEX = 1;

typedef uint8_t POKEGEAR_BIT;
const POKEGEAR_BIT MAP_CARD = 0;
const POKEGEAR_BIT RADIO_CARD = 1;
const POKEGEAR_BIT POKEGEAR = 2; // Phone Card in reality, but they come together
const POKEGEAR_BIT EXPN_CARD = 3;