#include "../include/characters.h"
#include "../include/common.h"
#include "../include/strings.h"

#ifdef _WIN32
#include <shlwapi.h>
#define strcasestr StrStrIA
#else
#define _GNU_SOURCE // For strcasestr(), which is not standard
#endif

#include <ctype.h>
#include <string.h>

// Removes leading and/or trailing characters from a string.
void trim_string(char *string, char *ltrim, char *rtrim)
{
    size_t len = strlen(string);

    if (ltrim) {
        size_t spn = strspn(string, ltrim);
        if (spn)
            memmove(string, string + spn, len + 1 - spn);
        len -= spn;
    }

    if (rtrim) {
        size_t rtrimlen = strlen(rtrim);
        for (size_t i = len - 1; i > 0; i--) {
            for (size_t j = 0; j < rtrimlen; j++)
                if (rtrim[j] == string[i]) {
                    string[i] = '\0';
                    goto next;
                }
            break;
            
next:
            continue;
        }
    }
}

// Case-insensitively tests if a string contains a word
inline char *strwrd(const char *string, char *word)
{
    char *p = strcasestr(string, word);
    if (p == NULL)
        return NULL;

    size_t word_len = strlen(word);
    size_t string_len = strlen(string);

    do {
        if ((p != string && isalnum(p[-1]))
                || (p != string + (string_len - word_len) && isalnum(p[word_len]))) {
            continue;
        }
        return p;
    } while ((p = strcasestr(++p, word)));

    return NULL;
}

// Case-insensitively replaces all occurences of a word in a character array
// "string" must be of length MAX_FILENAME_LEN
static void replace_word(char *string, char *word, char *replace)
{
    char *p = string;
    size_t word_len = strlen(word);
    size_t replace_len = strlen(replace);

    while ((p = strwrd(p, word))) {
        // Abort if new string length would be too large
        if (strlen(string) - word_len + replace_len > MAX_FILENAME_LEN) return;

        memmove(p + replace_len, p + word_len, strlen(p + word_len) + 1);
        memcpy(p, replace, replace_len);

        p++;
    }
}

// Removes unused curly braces expressions; returns 0 on success
static int curlycrunch(char *string, int position)
{
    char temp[MAX_FILENAME_LEN] = "";

    // Search left
    for (int i = position; i >= 0; i--) {
        if (string[i] == '{') {
            strncpy(temp, string, i);
            //printf("%s left : %s\n", __func__, temp); // DEBUG
            break;
        }
        if (string[i] == '}') {
            return 1;
        }
    }

    // Search right
    for (size_t i = position; i < strlen(string); i++) {
        if (string[i] == '}') {
            strcat(temp, &string[i + 1]);
            //printf("%s right: %s\n", __func__, temp); // DEBUG
            break;
        }
        if (string[i] == '{' || i == strlen(string) - 1) {
            return 1;
        }
    }

    strcpy(string, temp);
    return 0;
}

// Replaces all occurences of "search" in "string" (an array of char), e.g.:
// strreplace(temp, "%title%", title)
// "string" must be of length MAX_FILENAME_LEN
// "search" and "replace" must not be equal
inline char *strreplace(char *string, char *search, char *replace)
{
    char *p;

    while ((p = strstr(string, search))) {
        char temp[MAX_FILENAME_LEN] = "";
        int position; // Position of first character of "search" in "format_string"
        if (replace == NULL) replace = "";
        position = p - string;

        //printf("Search string: %s\n", search); // DEBUG
        //printf("Replace string: %s\n", replace); // DEBUG

        // If replace string is an empty pattern variable, remove associated
        // curly braces expression
        if (search[0] == '%' && strcmp(replace, "") == 0 &&
                curlycrunch(string, position) == 0) return NULL;

        strncpy(temp, string, position);
        //printf("Current string (step 1): \"%s\"\n", temp); // DEBUG
        strcat(temp, replace);
        //printf("Current string (step 2): \"%s\"\n", temp);  // DEBUG
        strcat(temp, string + position + strlen(search));
        //printf("Current string (step 3): \"%s\"\n\n", temp);  //DEBUG
        strcpy(string, temp);
    }

    return p;
}

// Converts a string (title, to be specific) to mixed-case style
void mixed_case(char *title)
{
    int len = strlen(title);

    // Apply mixed-case style
    title[0] = toupper(title[0]);
    for (int i = 1; i < len; i++) {
        if (isspace(title[i - 1]) || is_in_set(title[i - 1], ":-;~_1234567890")) {
            title[i] = toupper(title[i]);
        } else {
            title[i] = tolower(title[i]);
        }
    }

    // Make sure certain words are spelled correctly
    char *special_words[] = {
        // Roman numerals
        "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI", "XII",
        "XIII", "XIV", "XV", "XVI", "XVII", "XVIII", "XVIX", "XX",
        // Games
        "20XX",
        "2Dark",
        "2K",
        "2K14",
        "2K15",
        "2K16",
        "2K17",
        "2K18",
        "2K19",
        "2K20",
        "2K21",
        "2K22",
        "2X",
        "3D",
        "4K",
        "ABC",
        "ACA",
        "ADR1FT",
        "AER",
        "AI",
        "AO",
        "ARK",
        "ATV",
        "AVICII",
        "AdVenture",
        "AereA",
        "AeternoBlade",
        "AnywhereVR",
        "ArmaGallant",
        "Avenger iX",
        "BMX",
        "BaZooka",
        "BioHazard",
        "BioShock",
        "BlazBlue",
        "BlazeRush",
        "BloodRayne",
        "BoxVR",
        "CastleStorm",
        "ChromaGun",
        "CrossCode",
        "CruisinMix",
        "DC",
        "DCL",
        "DEX",
        "DG2",
        "DX",
        "DJMax",
        "DLC",
        "DS",
        "DUB",
        "DWVR",
        "DarkWatch",
        "DayZ",
        "DmC",
        "DreamMix",
        "DreamWorks",
        "EA",
        "EBKore",
        "ECHO",
        "EFootball",
        "EP",
        "ESP",
        "ESPN",
        "EVE",
        "EX",
        "EXA",
        "EarthNight",
        "FEZ",
        "FIA",
        "FIFA",
        "F.I.S.T.",
        "FX2",
        "FX3",
        "FantaVision",
        "Fate/Extella",
        "FightN",
        "FighterZ",
        "FlOw",
        "FlatOut",
        "GI",
        "GODS",
        "GP",
        "Gris",
        "GU",
        "GoldenEye",
        "GreedFall",
        "HD",
        "HOA",
        "HiQ",
        "Hitman GO",
        "ICO",
        "IF",
        "InFamous",
        "IxSHE",
        "JJ",
        "JoJos",
        "JoyRide",
        "JumpJet",
        "KO",
        "KOI",
        "KeyWe",
        "KickBeat",
        "LA Cops",
        "LittleBigPlanet",
        "LocoRoco",
        "LoveR Kiss",
        "MLB",
        "MS",
        "MV",
        "MX",
        "MXGP",
        "MalFunction",
        "MasterCube",
        "McIlroy",
        "McMorris",
        "MechWarrior",
        "MediEvil",
        "MegaDrive",
        "MotoGP",
        "MudRunner",
        "NASCAR",
        "NBA",
        "N.E.R.O.",
        "NESTS",
        "NFL",
        "NG",
        "NHL",
        "NT",
        "NY",
        "NecroDancer",
        "NeoGeo",
        "NeoWave",
        "NeuroVoider",
        "NieR",
        "OG",
        "OK",
        "OMG",
        "OhShape",
        "OlliOlli",
        "OutRun",
        "OwlBoy",
        "PAW",
        "PES",
        "PGA",
        "PS2",
        "PS4",
        "PSN",
        "PaRappa",
        "PixARK",
        "PixelJunk",
        "PlayStation",
        "Project CARS",
        "ProStreet",
        "QuiVr",
        "RBI",
        "REV",
        "RICO",
        "RIGS",
        "RiME",
        "RPG",
        "RemiLore",
        "RiMS",
        "RollerCoaster",
        "Romancing SaGa",
        "RyoRaiRai",
        "SD",
        "SG/ZH",
        "SH1FT3R",
        "SNES",
        "SNK",
        "SSX",
        "SVC",
        "SaGa Frontier",
        "SaGa Scarlet",
        "SkullGirls",
        "SkyScrappers",
        "SmackDown",
        "SnowRunner",
        "SoulCalibur",
        "SpeedRunners",
        "SpinMaster",
        "SquarePants",
        "SteamWorld",
        "SuperChargers",
        "SuperEpic",
        "TMNT",
        "Tron RUN/r",
        "TT",
        "TV",
        "ToeJam",
        "TowerFall",
        "TrackMania",
        "TrainerVR",
        "UEFA",
        "UFC",
        "UN",
        "UNO",
        "UglyDolls",
        "UnMetal",
        "VA",
        "VFR",
        "VIIR",
        "VR",
        "VRobot",
        "VRog",
        "VirZOOM",
        "WMD",
        "WRC",
        "WWE",
        "WWII",
        "WindJammers",
        "XCOM",
        "XD",
        "XL",
        "XXL",
        "YU-NO",
        "YoRHa",
        "ZX",
        "eSports",
        "eX+",
        "pFBA",
        "pNES",
        "pSNES",
        "reQuest",
        "tRrLM();",
        "theHunter",
        "vs",
        NULL
    };
    for (int i = 0; special_words[i]; i++) {
        replace_word(title, special_words[i], special_words[i]);
    }
}

int lower_strcmp(char *string1, char *string2)
{
    if (strlen(string1) == strlen(string2)) {
        for (size_t i = 0; i < strlen(string1); i++) {
            if (tolower(string1[i]) != tolower(string2[i])) {
                return 1;
            }
        }
    } else {
        return 1;
    }

    return 0;
}
