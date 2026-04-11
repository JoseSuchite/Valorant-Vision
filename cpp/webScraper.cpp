/*
* Web scraper for VLR.gg to extract professional Valorant player names and their teams for Valorant-Vision project
* unfortunately VLR.gg does not have a public API, so we have to scrape the data directly from their website.
* Thus, the scraper relies heavily on the structure of VLR's HTML, so if they change their website this code might break and need to be updated.
*/

#include "../headers/textDetection.h"

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
std::vector<PlayerRecord>       TextDetection::proPlayers;  // Stores all loaded player records
std::unordered_set<std::string> TextDetection::teamLinks;   // Stores team page URLs found during scraping
std::unordered_set<std::string> TextDetection::playerSet;   // Stores player names

// Internal helper functions
// These functions are used internally for file handling, network requests, and HTML parsing.
namespace {

	// Callback for libcurl to write downloaded data into a string
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
    {
        size_t totalSize = size * nmemb;
        std::string* buffer = static_cast<std::string*>(userp);
        buffer->append((char*)contents, totalSize);
        return totalSize;
    }

    // All file I/O resolves relative to executable location
    // Goes up three levels: Qt/ -> build/ -> out/ -> project root
    std::filesystem::path dataDir()
    {
        return std::filesystem::current_path()
            .parent_path()
            .parent_path()
            .parent_path()
            / "data";
    }

	// Full path to players.txt
    std::filesystem::path playersFilePath()
    {
        return dataDir() / "players.txt";
    }

	// Trim whitespace from both ends of a string
    void trim(std::string& s)
    {
        auto start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) { s.clear(); return; }
        auto end = s.find_last_not_of(" \t\r\n");
        s = s.substr(start, end - start + 1);
    }

} 

// public getter for main.cpp to access loaded roster
const std::vector<PlayerRecord>& TextDetection::getPlayers()
{
    return proPlayers;
}

// fileExists checks if players.txt exists and is non-empty
// Very basic check so might need to be expanded in the future
bool TextDetection::fileExists()
{
    std::error_code ec;

	// Check if file exists
    if (!std::filesystem::exists(playersFilePath(), ec) || ec)
        return false;

	// Check if file is non-empty by looking for at least one non-empty line
    std::ifstream file(playersFilePath());
    if (!file.is_open())
        return false;

    std::string line;
    while (std::getline(file, line))
        if (!line.empty()) return true;

    return false;
}

// Reads from players.txt and populates proPlayers and playerSet
void TextDetection::loadPlayersFromFile()
{
	// Clear existing data before loading new data
    proPlayers.clear();
    playerSet.clear();

	// Check if file exists before trying to read
    std::ifstream file(playersFilePath());
    if (!file.is_open())
    {
        std::cerr << "Could not open players.txt\n";
        return;
    }

	// Read each line, expecting format: "TEAM PLAYERNAME"
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

// Writes current proPlayers data to players.txt in "TEAM PLAYERNAME" format
void TextDetection::savePlayersToFile()
{
    namespace fs = std::filesystem;

	// Ensure data directory exists
    fs::path dir = dataDir();
    std::error_code ec;
    if (!fs::exists(dir, ec))
        fs::create_directories(dir, ec);

	// Open file for writing (overwrites existing file)
    fs::path filePath = playersFilePath();
    std::ofstream file(filePath);

	// Check if file opened successfully
    if (!file.is_open())
    {
        std::cerr << "Failed to open " << filePath << "\n";
        return;
    }

	// Write each player record in "TEAM PLAYERNAME" format
    for (const auto& p : proPlayers)
        file << p.team << " " << p.name << "\n";

    std::cout << "Saved " << proPlayers.size() << " players to: " << filePath << "\n";
}

// Uses libcurl to download the HTML content of a page with retries and error handling 
// Amount of retries set in header file (default 3) but can also be adjusted when calling the function
std::string TextDetection::downloadPage(const std::string& url, int maxRetries)
{
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    // Set common options for all requests
    std::string html;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

	// Attempt the request with retries
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

	// Clean up curl resources
    curl_easy_cleanup(curl);

	// If all attempts failed, return empty string
    if (res != CURLE_OK)
    {
        std::cerr << "All retries failed for: " << url << "\n";
        return "";
    }
    return html;
}

// Extracts team page URLs from the main teams listing page HTML and stores them in teamLinks
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

// Extracts the team abbreviation (tag) from a team page's HTML
std::string TextDetection::extractTeamTag(const std::string& html)
{
    std::smatch match;

	// Primary regex looks for the main team header tag
	std::regex primaryRegex(R"(<h2 class="wf-title team-header-tag">([^<]+)</h2>)");    // uses html, so if they change the structure of the page this might break
    if (std::regex_search(html, match, primaryRegex))
    {
        std::string tag = match[1].str();
        trim(tag);
        return tag;
    }

	std::regex fallbackRegex(R"(<span class="m-item-team-tag">\s*([^<]+?)\s*</span>)"); // uses html, so if they change the structure of the page this might break
    if (std::regex_search(html, match, fallbackRegex))
    {
        std::string tag = match[1].str();
        trim(tag);
        return tag;
    }

    return "";
}

// Extracts player names from a team page's HTML by finding the "players" section and parsing player entries
std::string TextDetection::extractPlayers(const std::string& html)
{
    std::unordered_set<std::string> teamPlayerSet;
    std::string teamName = extractTeamTag(html);

	// If we can't find the team name, return early
    if (teamName.empty())
    {
        std::cerr << "Could not find team abbreviation (tag) in page HTML\n";
        return "";
    }

	// team page has separate sections for players and staff so declare labels and search for them to isolate the players section
    const std::string playersLabel = "wf-module-label";
    const std::string playersText = "players";
    const std::string staffText = "staff";

    size_t playersPos = std::string::npos;
    size_t searchPos = 0;

	// Searches through the HTML to find the position of the "players" section by looking for the label and matching its text content
	// Once again relies on HTML structure, so if they change the page this might break
    while (searchPos < html.size())
    {
		// Find the next label element
        size_t labelPos = html.find(playersLabel, searchPos);
        if (labelPos == std::string::npos) break;

		// Find the closing '>' of the label tag
        size_t closeTag = html.find('>', labelPos);
        if (closeTag == std::string::npos) break;

		// Find the next '<' which indicates the end of the label content
        size_t nextClose = html.find('<', closeTag + 1);
        if (nextClose == std::string::npos) break;

		// Extract the text content of the label
        std::string labelContent = html.substr(closeTag + 1, nextClose - closeTag - 1);

		// Trim whitespace from label content before comparing to "players"
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

	// Now find the position of the "staff" section which comes after players to determine the end of the players section
    size_t staffPos = html.size();
    size_t searchPos2 = playersPos;

	// Similar search loop to find the "staff" label and its position in the HTML
    while (searchPos2 < html.size())
    {
		// Same loop as before but looking for "staff" instead of "players"
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

	// Extract the HTML section that contains the players by taking the substring between the "players" and "staff" positions
    std::string playersSection = html.substr(playersPos, staffPos - playersPos);

	// Now we have the section of the HTML that contains player entries, we can use a regex to extract each player's name
    std::regex blockRegex(
        R"(<div class="team-roster-item-name-alias">\s*([\s\S]*?)\s*</div>)"
    );

    std::regex tagRegex(R"(<[^>]+>)");

    std::string remaining = playersSection;
    std::smatch match;

	// Loop through each player entry found by the regex, extract the player's name, trim whitespace, and add to the roster if it's not a duplicate 
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

// Attempts to find and scrape the two teams by their abbreviations (tags) using VLR.gg's search functionality
bool TextDetection::scrapeTeamsByAbbreviation(const std::string& teamA, const std::string& teamB)
{
    proPlayers.clear();
    playerSet.clear();

	// Flags to track if we successfully found and scraped each team
    bool foundA = false, foundB = false;

	// For each team abbreviation, perform a search on VLR.gg and look through the results for a matching team page
    for (const auto& abbr : { teamA, teamB })
    {
        std::cout << "Searching VLR.gg for: " << abbr << "\n";
        std::cout.flush(); 

		// Download the search results page for the team abbreviation
        std::string searchHTML = downloadPage("https://www.vlr.gg/search/?q=" + abbr + "&type=teams");
        if (searchHTML.empty()) continue;

		// Extract team page URLs from the search results HTML and store them in teamLinks
        teamLinks.clear();
        extractTeams(searchHTML);

		// Loop through the team page URLs found in the search results and check if any of them match the desired team abbreviation
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

	// Failure warnings for debugging/validation if we couldn't find one or both teams by their abbreviations
    if (!foundA) std::cerr << "Could not find team " << teamA << "\n";
    if (!foundB) std::cerr << "Could not find team " << teamB << "\n";

    if (!proPlayers.empty())
        savePlayersToFile();

    return foundA && foundB;
}

// If we couldn't find the teams by their abbreviations and there's no existing players.txt file, show a dialog to select regions to scrape all teams from
// Could use more styling, is very basic pop up right now 
std::vector<std::string> TextDetection::promptRegionSelection()
{
	// List of regions and their corresponding VLR.gg ranking page URLs
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

	// Create a dialog with a list of regions and OK/Cancel buttons
    QDialog dialog;
    dialog.setWindowTitle("Select Regions to Scrape");
    dialog.setMinimumWidth(300);

	// Vertical layout for the dialog
    QVBoxLayout* layout = new QVBoxLayout(&dialog);

	// Instruction label at the top of the dialog
    QLabel* label = new QLabel(
        "Could not find teams via search.\n"
        "Select regions to scrape all teams from:"
    );
    layout->addWidget(label);

	// List widget to display regions with multi-selection enabled
    QListWidget* list = new QListWidget();
    list->setSelectionMode(QAbstractItemView::MultiSelection);
    for (const auto& region : regionList)
        list->addItem(QString::fromStdString(region.first));
    layout->addWidget(list);

	// Horizontal layout for OK and Cancel buttons
    QHBoxLayout* buttons = new QHBoxLayout();
    QPushButton* ok = new QPushButton("Scrape Selected");
    QPushButton* cancel = new QPushButton("Skip — Scrape All");
    buttons->addWidget(ok);
    buttons->addWidget(cancel);
    layout->addLayout(buttons);

	// Connect button signals to dialog slots for accepting or rejecting the selection
    QObject::connect(ok, &QPushButton::clicked, &dialog, &QDialog::accept);
    QObject::connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);

	// Execute the dialog and if the user accepted, gather the selected regions' URLs based on their selection in the list widget
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

// Final resort if we couldn't find the teams by their abbreviations and there's no existing players.txt file,
// and the user either skipped the region selection or didn't select any regions
void TextDetection::scrapePlayers()
{
	// Clear any existing data before starting the full scrape
    proPlayers.clear();
    teamLinks.clear();
    playerSet.clear();

	// List of all region ranking page URLs to scrape teams from
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

	// First, scrape all the team pages from the region ranking pages to populate teamLinks
    std::cout << "Downloading all ranking pages...\n";
    for (const auto& region : regions)
    {
		std::cout << "Scraping: " << region << "\n"; // Log the region being scraped
        std::cout.flush();
        std::string html = downloadPage(region);
        if (!html.empty())
            extractTeams(html);
    }
    
	// Then, for each team page URL found, scrape the player names and teams to populate proPlayers
    std::cout << "Teams found: " << teamLinks.size() << "\n";

	// Loop through each team page URL in teamLinks, download the page, and extract player information
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

// Main initialization function called by main.cpp after OCR attempts to detect teams
bool TextDetection::prepareForMatch(const std::string& teamA, const std::string& teamB)
{
	// Log the team abbreviations we're looking for the ones being passed in 
    std::cout << "Looking for teams: " << teamA << " / " << teamB << "\n";
    std::cout.flush();

	// try to scrape the teams directly by their abbreviations first
    if (!teamA.empty() && !teamB.empty())
    {
        if (scrapeTeamsByAbbreviation(teamA, teamB))
        {
            std::cout << "Both teams scraped successfully.\n";
            return true;
        }
        std::cout << "Targeted scrape failed.\n";
    }

	// next check if there's an existing players.txt file we can load from
    if (fileExists())
    {
        std::cout << "Loading existing players.txt...\n";
        loadPlayersFromFile();
        return !proPlayers.empty();
    }

	// if there is no file, prompt the user to select regions to scrape from
    std::cout << "No players.txt found, prompting region selection...\n";
    std::cout.flush();
    std::vector<std::string> regions = promptRegionSelection();

	// if the user selected regions, scrape teams and players from those regions, otherwise fall back to scraping all regions
    if (!regions.empty())
    {
        proPlayers.clear();
        playerSet.clear();
        teamLinks.clear();

		// First, scrape the selected region ranking pages to find team page URLs
        for (const auto& url : regions)
        {
            std::cout << "Downloading: " << url << "\n";
            std::cout.flush();
            std::string html = downloadPage(url);
            if (!html.empty())
                extractTeams(html);
        }

		// Log the number of team pages found from the selected regions before scraping player information
        std::cout << "Team pages found: " << teamLinks.size() << "\n";

		// Then, for each team page URL found, scrape the player names and teams to populate proPlayers
        for (const auto& url : teamLinks)
        {
            std::cout << "Scraping: " << url << "\n";
            std::cout.flush();
            std::string html = downloadPage(url);
            if (!html.empty())
                extractPlayers(html);
        }

		// Log the number of players scraped from the selected regions before saving to file
        std::cout << "Players scraped: " << proPlayers.size() << "\n";

        if (!proPlayers.empty())
        {
            savePlayersToFile();
            return true;
        }
    }

    // if all else fails, run a full scrape of all regions 
    std::cout << "Running full scrape of all regions...\n";
    scrapePlayers();
    return !proPlayers.empty();
}

/* Uncomment if you want to debug only two teams. 
void TextDetection::scrapeDebugTeams()
{
    proPlayers.clear();
    playerSet.clear();

    static const std::vector<std::string> debugTeams = {
        "https://www.vlr.gg/team/2593/paper-rex",
        "https://www.vlr.gg/team/624/sentinels",
    };

    for (const auto& url : debugTeams)
    {
        std::cout << "[DEBUG] Scraping: " << url << "\n";
        std::cout.flush();
        std::string html = downloadPage(url);
        if (html.empty()) continue;

        std::string tag = extractPlayers(html);
        if (tag.empty())
            std::cerr << "[DEBUG] No team tag found for: " << url << "\n";
        else
            std::cout << "[DEBUG] Scraped team: " << tag << "\n";
    }

    std::cout << "[DEBUG] Total players: " << proPlayers.size() << "\n";
    savePlayersToFile();
}
*/