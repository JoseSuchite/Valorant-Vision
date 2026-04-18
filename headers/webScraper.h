#ifndef TEXTDETECTION_H
#define TEXTDETECTION_H

#include <string>
#include <vector>
#include <unordered_set>

struct PlayerRecord {
    std::string team;
    std::string name;
};

class TextDetection {
public:
    static bool prepareForMatch(const std::string& teamA, const std::string& teamB);
    static const std::vector<PlayerRecord>& getPlayers();

    static bool scrapeTeamsByAbbreviation(const std::string& teamA, const std::string& teamB);
    static void scrapePlayers();

    static void loadPlayersFromFile();
    static void savePlayersToFile();
    static bool fileExists();

    static std::string downloadPage(const std::string& url, int maxRetries = 3);

    static void extractTeams(const std::string& html);
    static std::string extractTeamTag(const std::string& html);
    static std::string extractPlayers(const std::string& html);

    static std::vector<std::string> promptRegionSelection();
    static std::vector<std::string> scrapeAllTeamAbbreviations();

private:
    static std::vector<PlayerRecord>       proPlayers;
    static std::unordered_set<std::string> teamLinks;
    static std::unordered_set<std::string> playerSet;
};

#endif // TEXTDETECTION_H
