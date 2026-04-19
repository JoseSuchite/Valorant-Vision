/*
* Web scraper for VLR.gg to extract professional Valorant player names and their teams for Valorant-Vision project
* unfortunately VLR.gg does not have a public API, so we have to scrape the data directly from their website.
* Thus, the scraper relies heavily on the structure of VLR's HTML, so if they change their website this code might break and need to be updated.
*/

#include "../headers/webScraper.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <regex>
#include <filesystem>
#include <system_error>

#include <curl/curl.h>

#include <QApplication>
#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

// Static member definitions
std::vector<PlayerRecord>       TextDetection::proPlayers;
std::unordered_set<std::string> TextDetection::teamLinks;
std::unordered_set<std::string> TextDetection::playerSet;

namespace {

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
    {
        size_t totalSize = size * nmemb;
        std::string* buffer = static_cast<std::string*>(userp);
        buffer->append((char*)contents, totalSize);
        return totalSize;
    }

    // Resolves to the executable's directory so paths work regardless of working directory
    std::filesystem::path dataDir()
    {
        return std::filesystem::current_path() / "data";
    }

    std::filesystem::path playersFilePath()
    {
        return dataDir() / "players.txt";
    }

    void trim(std::string& s)
    {
        auto start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) { s.clear(); return; }
        auto end = s.find_last_not_of(" \t\r\n");
        s = s.substr(start, end - start + 1);
    }

}

const std::vector<PlayerRecord>& TextDetection::getPlayers()
{
    return proPlayers;
}

bool TextDetection::fileExists()
{
    std::error_code ec;
    if (!std::filesystem::exists(playersFilePath(), ec) || ec)
        return false;

    std::ifstream file(playersFilePath());
    if (!file.is_open())
        return false;

    std::string line;
    while (std::getline(file, line))
        if (!line.empty()) return true;

    return false;
}

void TextDetection::loadPlayersFromFile()
{
    proPlayers.clear();
    playerSet.clear();

    std::ifstream file(playersFilePath());
    if (!file.is_open())
    {
        std::cerr << "Could not open players.txt\n";
        return;
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty()) continue;
        auto space = line.find(' ');
        if (space == std::string::npos) continue;

        PlayerRecord rec;
        rec.team = line.substr(0, space);
        rec.name = line.substr(space + 1);
        proPlayers.push_back(rec);
        playerSet.insert(rec.name);
    }

    std::cout << "Loaded " << proPlayers.size() << " players from file.\n";
}

void TextDetection::savePlayersToFile()
{
    namespace fs = std::filesystem;

    fs::path dir = dataDir();
    std::error_code ec;
    if (!fs::exists(dir, ec))
        fs::create_directories(dir, ec);

    fs::path filePath = playersFilePath();
    std::ofstream file(filePath);

    if (!file.is_open())
    {
        std::cerr << "Failed to open " << filePath << "\n";
        return;
    }

    for (const auto& p : proPlayers)
        file << p.team << " " << p.name << "\n";

    std::cout << "Saved " << proPlayers.size() << " players to: " << filePath << "\n";
}

std::string TextDetection::downloadPage(const std::string& url, int maxRetries)
{
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string html;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = CURLE_FAILED_INIT;
    for (int attempt = 0; attempt < maxRetries; ++attempt)
    {
        html.clear();
        res = curl_easy_perform(curl);
        if (res == CURLE_OK) break;

        std::cerr << "Attempt " << (attempt + 1) << " failed for "
            << url << ": " << curl_easy_strerror(res) << "\n";
        std::cout.flush();
    }

    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        std::cerr << "All retries failed for: " << url << "\n";
        return "";
    }
    return html;
}

void TextDetection::extractTeams(const std::string& html)
{
    std::regex teamRegex(R"(/team/[0-9]+/[A-Za-z0-9\-]+)");
    std::smatch match;
    std::string remaining = html;

    while (std::regex_search(remaining, match, teamRegex))
    {
        teamLinks.insert("https://www.vlr.gg" + match[0].str());
        remaining = match.suffix().str();
    }
}

std::string TextDetection::extractTeamTag(const std::string& html)
{
    std::smatch match;

    std::regex primaryRegex(R"(<h2 class="wf-title team-header-tag">([^<]+)</h2>)");
    if (std::regex_search(html, match, primaryRegex))
    {
        std::string tag = match[1].str();
        trim(tag);
        return tag;
    }

    std::regex fallbackRegex(R"(<span class="m-item-team-tag">\s*([^<]+?)\s*</span>)");
    if (std::regex_search(html, match, fallbackRegex))
    {
        std::string tag = match[1].str();
        trim(tag);
        return tag;
    }

    return "";
}

std::string TextDetection::extractPlayers(const std::string& html)
{
    std::unordered_set<std::string> teamPlayerSet;
    std::string teamName = extractTeamTag(html);

    if (teamName.empty())
    {
        std::cerr << "Could not find team abbreviation (tag) in page HTML\n";
        return "";
    }

    const std::string playersLabel = "wf-module-label";
    const std::string playersText = "players";
    const std::string staffText = "staff";

    size_t playersPos = std::string::npos;
    size_t searchPos = 0;

    while (searchPos < html.size())
    {
        size_t labelPos = html.find(playersLabel, searchPos);
        if (labelPos == std::string::npos) break;

        size_t closeTag = html.find('>', labelPos);
        if (closeTag == std::string::npos) break;

        size_t nextClose = html.find('<', closeTag + 1);
        if (nextClose == std::string::npos) break;

        std::string labelContent = html.substr(closeTag + 1, nextClose - closeTag - 1);

        auto s = labelContent.find_first_not_of(" \t\r\n");
        auto e = labelContent.find_last_not_of(" \t\r\n");
        std::string trimmed = (s == std::string::npos) ? "" : labelContent.substr(s, e - s + 1);

        if (trimmed == playersText)
        {
            playersPos = nextClose;
            break;
        }

        searchPos = labelPos + 1;
    }

    if (playersPos == std::string::npos)
        return "";

    size_t staffPos = html.size();
    size_t searchPos2 = playersPos;

    while (searchPos2 < html.size())
    {
        size_t labelPos = html.find(playersLabel, searchPos2);
        if (labelPos == std::string::npos) break;

        size_t closeTag = html.find('>', labelPos);
        if (closeTag == std::string::npos) break;

        size_t nextClose = html.find('<', closeTag + 1);
        if (nextClose == std::string::npos) break;

        std::string labelContent = html.substr(closeTag + 1, nextClose - closeTag - 1);

        auto s = labelContent.find_first_not_of(" \t\r\n");
        auto e = labelContent.find_last_not_of(" \t\r\n");
        std::string trimmed = (s == std::string::npos) ? "" : labelContent.substr(s, e - s + 1);

        if (trimmed == staffText)
        {
            staffPos = labelPos;
            break;
        }

        searchPos2 = labelPos + 1;
    }

    std::string playersSection = html.substr(playersPos, staffPos - playersPos);

    std::regex blockRegex(
        R"(<div class="team-roster-item-name-alias">\s*([\s\S]*?)\s*</div>)"
    );

    std::regex tagRegex(R"(<[^>]+>)");

    std::string remaining = playersSection;
    std::smatch match;

    while (std::regex_search(remaining, match, blockRegex))
    {
        std::string inner = match[1].str();
        std::string player = std::regex_replace(inner, tagRegex, "");

        auto s = player.find_first_not_of(" \t\r\n");
        auto e = player.find_last_not_of(" \t\r\n");

        if (s != std::string::npos)
            player = player.substr(s, e - s + 1);
        else
            player.clear();

        if (!player.empty() && teamPlayerSet.insert(player).second)
        {
            PlayerRecord rec;
            rec.name = player;
            rec.team = teamName;

            proPlayers.push_back(rec);
            playerSet.insert(player);
        }

        remaining = match.suffix().str();
    }

    return teamName;
}

bool TextDetection::scrapeTeamsByAbbreviation(const std::string& teamA, const std::string& teamB)
{
    proPlayers.clear();
    playerSet.clear();

    bool foundA = false, foundB = false;

    for (const auto& abbr : { teamA, teamB })
    {
        std::cout << "Searching VLR.gg for: " << abbr << "\n";
        std::cout.flush();

        std::string searchHTML = downloadPage("https://www.vlr.gg/search/?q=" + abbr + "&type=teams");
        if (searchHTML.empty()) continue;

        teamLinks.clear();
        extractTeams(searchHTML);

        for (const auto& url : teamLinks)
        {
            std::cout << "Checking: " << url << "\n";
            std::cout.flush();

            std::string html = downloadPage(url);
            if (html.empty()) continue;

            std::string tag = extractTeamTag(html);
            if (tag != abbr) continue;

            std::cout << "Found and scraping: " << tag << "\n";
            std::cout.flush();
            extractPlayers(html);

            if (tag == teamA) foundA = true;
            if (tag == teamB) foundB = true;
            break;
        }
    }

    if (!foundA) std::cerr << "Could not find team " << teamA << "\n";
    if (!foundB) std::cerr << "Could not find team " << teamB << "\n";

    if (!proPlayers.empty())
        savePlayersToFile();

    return foundA && foundB;
}

std::vector<std::string> TextDetection::promptRegionSelection()
{
    static const std::vector<std::pair<std::string, std::string>> regionList = {
        { "Europe",        "https://www.vlr.gg/rankings/europe"        },
        { "North America", "https://www.vlr.gg/rankings/north-america" },
        { "Brazil",        "https://www.vlr.gg/rankings/brazil"        },
        { "Asia-Pacific",  "https://www.vlr.gg/rankings/asia-pacific"  },
        { "China",         "https://www.vlr.gg/rankings/china"         },
        { "Japan",         "https://www.vlr.gg/rankings/japan"         },
        { "LA South",      "https://www.vlr.gg/rankings/la-s"          },
        { "LA North",      "https://www.vlr.gg/rankings/la-n"          },
        { "Oceania",       "https://www.vlr.gg/rankings/oceania"       },
        { "GC",            "https://www.vlr.gg/rankings/gc"            },
        { "MENA",          "https://www.vlr.gg/rankings/mena"          },
        { "Collegiate",    "https://www.vlr.gg/rankings/collegiate"    },
    };

    std::vector<std::string> selected;

    QDialog dialog;
    dialog.setWindowTitle("Select Regions to Scrape");
    dialog.setMinimumWidth(300);

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    QLabel* label = new QLabel(
        "Could not find teams via search.\n"
        "Select regions to scrape all teams from:"
    );
    layout->addWidget(label);

    QListWidget* list = new QListWidget();
    list->setSelectionMode(QAbstractItemView::MultiSelection);
    for (const auto& region : regionList)
        list->addItem(QString::fromStdString(region.first));
    layout->addWidget(list);

    QHBoxLayout* buttons = new QHBoxLayout();
    QPushButton* ok = new QPushButton("Scrape Selected");
    QPushButton* cancel = new QPushButton("Skip — Scrape All");
    buttons->addWidget(ok);
    buttons->addWidget(cancel);
    layout->addLayout(buttons);

    QObject::connect(ok, &QPushButton::clicked, &dialog, &QDialog::accept);
    QObject::connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted)
    {
        for (auto* item : list->selectedItems())
        {
            int row = list->row(item);
            selected.push_back(regionList[row].second);
        }
    }

    return selected;
}

void TextDetection::scrapePlayers()
{
    proPlayers.clear();
    teamLinks.clear();
    playerSet.clear();

    static const std::vector<std::string> regions = {
        "https://www.vlr.gg/rankings/europe",
        "https://www.vlr.gg/rankings/north-america",
        "https://www.vlr.gg/rankings/brazil",
        "https://www.vlr.gg/rankings/asia-pacific",
        "https://www.vlr.gg/rankings/china",
        "https://www.vlr.gg/rankings/japan",
        "https://www.vlr.gg/rankings/la-s",
        "https://www.vlr.gg/rankings/la-n",
        "https://www.vlr.gg/rankings/oceania",
        "https://www.vlr.gg/rankings/gc",
        "https://www.vlr.gg/rankings/mena",
        "https://www.vlr.gg/rankings/collegiate"
    };

    std::cout << "Downloading all ranking pages...\n";
    for (const auto& region : regions)
    {
        std::cout << "Scraping: " << region << "\n";
        std::cout.flush();
        std::string html = downloadPage(region);
        if (!html.empty())
            extractTeams(html);
    }

    std::cout << "Teams found: " << teamLinks.size() << "\n";

    for (const auto& url : teamLinks)
    {
        std::cout << "Scraping: " << url << "\n";
        std::cout.flush();
        std::string html = downloadPage(url);
        if (!html.empty())
            extractPlayers(html);
    }

    std::cout << "Players found: " << proPlayers.size() << "\n";
    savePlayersToFile();
}

std::vector<std::string> TextDetection::scrapeAllTeamAbbreviations()
{
    static const std::vector<std::string> regions = {
        "https://www.vlr.gg/rankings/europe",
        "https://www.vlr.gg/rankings/north-america",
        "https://www.vlr.gg/rankings/brazil",
        "https://www.vlr.gg/rankings/asia-pacific",
        "https://www.vlr.gg/rankings/china",
        "https://www.vlr.gg/rankings/japan",
        "https://www.vlr.gg/rankings/la-s",
        "https://www.vlr.gg/rankings/la-n",
        "https://www.vlr.gg/rankings/oceania",
        "https://www.vlr.gg/rankings/gc",
        "https://www.vlr.gg/rankings/mena",
        "https://www.vlr.gg/rankings/collegiate"
    };

    // Team abbreviations appear inline on ranking pages in this span —
    // no need to visit individual team pages.
    std::regex tagRegex(R"(<span class="m-item-team-tag">\s*([^<]+?)\s*</span>)");

    std::unordered_set<std::string> seen;
    std::vector<std::string> result;

    for (const auto& url : regions)
    {
        std::string html = downloadPage(url);
        if (html.empty()) continue;

        std::smatch match;
        std::string remaining = html;
        while (std::regex_search(remaining, match, tagRegex))
        {
            std::string tag = match[1].str();
            // Trim whitespace
            auto s = tag.find_first_not_of(" \t\r\n");
            auto e = tag.find_last_not_of(" \t\r\n");
            if (s != std::string::npos)
                tag = tag.substr(s, e - s + 1);

            if (!tag.empty() && seen.insert(tag).second)
                result.push_back(tag);

            remaining = match.suffix().str();
        }
    }

    std::cout << "Scraped " << result.size() << " team abbreviations.\n";
    return result;
}

bool TextDetection::prepareForMatch(const std::string& teamA, const std::string& teamB)
{
    std::cout << "Looking for teams: " << teamA << " / " << teamB << "\n";
    std::cout.flush();

    if (!teamA.empty() && !teamB.empty())
    {
        if (scrapeTeamsByAbbreviation(teamA, teamB))
        {
            std::cout << "Both teams scraped successfully.\n";
            return true;
        }
        std::cout << "Targeted scrape failed.\n";
    }

    if (fileExists())
    {
        std::cout << "Loading existing players.txt...\n";
        loadPlayersFromFile();
        return !proPlayers.empty();
    }

    std::cout << "No players.txt found, prompting region selection...\n";
    std::cout.flush();
    std::vector<std::string> regions = promptRegionSelection();

    if (!regions.empty())
    {
        proPlayers.clear();
        playerSet.clear();
        teamLinks.clear();

        for (const auto& url : regions)
        {
            std::cout << "Downloading: " << url << "\n";
            std::cout.flush();
            std::string html = downloadPage(url);
            if (!html.empty())
                extractTeams(html);
        }

        std::cout << "Team pages found: " << teamLinks.size() << "\n";

        for (const auto& url : teamLinks)
        {
            std::cout << "Scraping: " << url << "\n";
            std::cout.flush();
            std::string html = downloadPage(url);
            if (!html.empty())
                extractPlayers(html);
        }

        std::cout << "Players scraped: " << proPlayers.size() << "\n";

        if (!proPlayers.empty())
        {
            savePlayersToFile();
            return true;
        }
    }

    std::cout << "Running full scrape of all regions...\n";
    scrapePlayers();
    return !proPlayers.empty();
}
