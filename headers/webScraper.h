#ifndef WEBSCRAPER_H
#define WEBSCRAPER_H

#include <string>
#include <vector>
#include <unordered_set>

// / Simple struct to hold player information: team and name
struct PlayerRecord {
    std::string team;
    std::string name;
};

// Main class for text detection and web scraping of player data from VLR.gg
class WebScraper {
public:
    // Main entry point function
    static bool prepareForMatch(const std::string& teamA, const std::string& teamB);

    // Getter function 
    static const std::vector<PlayerRecord>& getPlayers();

    // Cascade steps functions
    static bool scrapeTeamsByAbbreviation(const std::string& teamA, const std::string& teamB);
    static void scrapePlayers();

    // File I/O functions
    static void loadPlayersFromFile();
    static void savePlayersToFile();
    static bool fileExists();

    // Network functions
    static std::string downloadPage(const std::string& url, int maxRetries = 3);

    // HTML parsing functions
    static void extractTeams(const std::string& html);
    static std::string extractTeamTag(const std::string& html);
    static std::string extractPlayers(const std::string& html);

    // Qt region selection dialog function
    static std::vector<std::string> promptRegionSelection();

    // Uncomment to debug two teams
    // static void scrapeDebugTeams();

// private helper functions for string manipulation, file paths, etc. are defined in the cpp file
private:
    static std::vector<PlayerRecord>       proPlayers;
    static std::unordered_set<std::string> teamLinks;
    static std::unordered_set<std::string> playerSet;
};

#endif // WEBSCRAPER_H
