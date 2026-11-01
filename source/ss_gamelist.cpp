//
// Created by cpasjuste on 03/12/2019.
//

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <unordered_set>
#include "ss_api.h"
#include "ss_gamelist.h"

#define ROM_BIOS (1 << 4)

using namespace ss_api;

void GameList::clear() {
    games.clear();
    romPaths.clear();
    systemList.systems.clear();
    editors.clear();
    developers.clear();
    genres.clear();
    players.clear();
    ratings.clear();
    rotations.clear();
    resolutions.clear();
    dates.clear();
}

bool GameList::append(const std::string &xmlPath, const std::string &rPath,
                      bool sort, const std::vector<std::string> &filters,
                      const System &system, bool availableOnly, const GameAddedCb &cb) {
    tinyxml2::XMLDocument doc;
    std::vector<Io::File> files;

    // add all files first
    if (!rPath.empty()) {
        romPaths.emplace_back(rPath);
        files = Io::getDirList(rPath, false, filters);
    }

    xml = xmlPath;

    // Per-rom gamelist.xml matching is no longer used: rom directories are
    // scanned from file names and enriched by the core (chinese .glt list +
    // FBNeo driver data). The xml path is kept ONLY for data files without a
    // rom dir - i.e. loading favorites.xml (rPath empty).
    if (rPath.empty() && !xmlPath.empty()) {
        tinyxml2::XMLError e = doc.LoadFile(xmlPath.c_str());
        if (e == tinyxml2::XML_SUCCESS) {
            tinyxml2::XMLNode *pRoot = doc.FirstChildElement("gameList");
            if (!pRoot) {
                pRoot = doc.FirstChildElement("datafile");
                if (!pRoot) {
                    SS_PRINT("GameList: wrong xml format: \'Data\', \'gameList\' or \'datafile\' tag not found\n");
                    format = Format::Unknown;
                    doc.Clear();
                    return false;
                }
                format = Format::FbNeoDat;
            } else {
                format = Format::EmulationStation;
            }

            tinyxml2::XMLNode *gameNode = pRoot->FirstChildElement("game");
            while (gameNode) {
                Game game;
                Game::parseGame(&game, gameNode, "", format);

                // set game "real path", minus filename (for pFBN)
                game.romsPath = rPath;

                // is rom available?
                const std::string n = game.path;
                auto it = std::find_if(files.begin(), files.end(), [n](const Io::File &f) {
                    return f.name == n;
                });
                if (it != files.end()) {
                    game.available = true;
                    files.erase(it);
                } else if (availableOnly) {
                    // move to next node (game)
                    gameNode = gameNode->NextSibling();
                    continue;
                }

                // add stuff for later filtering
                if (system.id) game.system = system;
                System sys1 = game.system;
                auto itSys = std::find_if(systemList.systems.begin(), systemList.systems.end(), [sys1](const System &sys2) {
                    return sys1.id == sys2.id || sys1.name == sys2.name;
                });
                if (itSys == systemList.systems.end()) {
                    systemList.systems.emplace_back(sys1);
                }

                Game::Editor ed = game.editor.name.empty() ? Game::Editor{0, "UNKNOWN"} : game.editor;
                auto itEd = std::find_if(editors.begin(), editors.end(), [ed](const Game::Editor &e) {
                    return ed.id == e.id || ed.name == e.name;
                });
                if (itEd == editors.end()) {
                    editors.emplace_back(ed);
                }

                Game::Developer dev = game.developer.name.empty() ? Game::Developer{0, "UNKNOWN"} : game.developer;
                auto itDev = std::find_if(developers.begin(), developers.end(), [dev](const Game::Developer &d) {
                    return dev.id == d.id || dev.name == d.name;
                });
                if (itDev == developers.end()) {
                    developers.emplace_back(dev);
                }

                Game::Genre genre = game.genre.name.empty() ? Game::Genre{0, "UNKNOWN"} : game.genre;
                auto itGenre = std::find_if(genres.begin(), genres.end(), [genre](const Game::Genre &g) {
                    return genre.id == g.id || genre.name == g.name;
                });
                if (itGenre == genres.end()) {
                    genres.emplace_back(genre);
                }

                auto itPlayers = std::find(players.begin(), players.end(), game.playersInt);
                if (itPlayers == players.end()) {
                    players.emplace_back(game.playersInt);
                }

                auto itRat = std::find(ratings.begin(), ratings.end(), game.rating);
                if (itRat == ratings.end()) {
                    ratings.emplace_back(game.rating);
                }

                auto itRot = std::find(rotations.begin(), rotations.end(), game.rotation);
                if (itRot == rotations.end()) {
                    rotations.emplace_back(game.rotation);
                }

                std::string resolution = game.resolution.empty() ? "UNKNOWN" : game.resolution;
                auto it2 = std::find(resolutions.begin(), resolutions.end(), resolution);
                if (it2 == resolutions.end()) {
                    resolutions.emplace_back(resolution);
                }

                it2 = std::find(dates.begin(), dates.end(), game.date);
                if (it2 == dates.end()) {
                    dates.emplace_back(game.date);
                }

                // callback
                if (cb) cb(&game);

                //判断下是不是bios 以及是否在fbneo的驱动中支持
                if (!(game.genre.id & ROM_BIOS) && game.name != "UnsupportedROM" && game.path != "neogeo.zip") {
                    games.emplace_back(game);
                }
                // move to next node (game)
                gameNode = gameNode->NextSibling();
            }
            doc.Clear();
        } else {
            SS_PRINT("GameList: %s\n", doc.ErrorName());
        }
    }

    // add "unknown" files (not in database)
    for (const auto &file: files) {
        Game game;
        game.id = std::hash<std::string>()(rPath + "/" + file.name);
        game.path = file.name;
        game.romsPath = rPath;
        game.name = file.name;
        game.available = true;
        game.system = system;
        System sys1 = game.system;
        auto itSys = std::find_if(systemList.systems.begin(), systemList.systems.end(), [sys1](const System &sys2) {
            return sys1.id == sys2.id || sys1.name == sys2.name;;
        });
        if (itSys == systemList.systems.end()) {
            systemList.systems.emplace_back(sys1);
        }
        // callback
        if (cb) cb(&game);
		
		//判断下是不是bios 以及是否在fbneo的驱动中支持		
		if (!(game.genre.id & ROM_BIOS) && game.name != "UnsupportedROM" && game.path != "neogeo.zip") {
			games.emplace_back(game);
		}		
        //games.emplace_back(game);
    }

    if (sort) {
        sortAlpha();
    }

    return true;
}

void GameList::sortAlpha(bool byZipName, bool gamesOnly) {
    // sort games
    if (byZipName) {
        std::sort(games.begin(), games.end(), Api::sortGameByPath);
    } else {
        std::sort(games.begin(), games.end(), Api::sortGameByNamePinYin);
    }

    // sort lists
    if (!gamesOnly) {
        std::sort(systemList.systems.begin(), systemList.systems.end(), Api::sortSystemByName);
        std::sort(editors.begin(), editors.end(), Api::sortEditorByName);
        std::sort(developers.begin(), developers.end(), Api::sortDeveloperByName);
        std::sort(genres.begin(), genres.end(), Api::sortGenreByName);
        std::sort(players.begin(), players.end(), Api::sortInteger);
        std::sort(ratings.begin(), ratings.end(), Api::sortInteger);
        std::sort(rotations.begin(), rotations.end(), Api::sortInteger);
        std::sort(resolutions.begin(), resolutions.end(), Api::sortByName);
        std::sort(dates.begin(), dates.end(), Api::sortByName);
    }
}

bool GameList::save(const std::string &dstPath, const std::string &imageType,
                    const std::string &thumbnailType, const std::string &videoType) {
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLElement *elem;

    tinyxml2::XMLDeclaration *dec = doc.NewDeclaration();
    doc.InsertFirstChild(dec);

    tinyxml2::XMLNode *pRoot = doc.NewElement("gameList");
    doc.InsertEndChild(pRoot);

    tinyxml2::XMLElement *pGames = pRoot->ToElement();

    // sort games
    std::sort(games.begin(), games.end(), Api::sortGameByName);

    for (const auto &game: games) {
        tinyxml2::XMLElement *gameElement = doc.NewElement("game");
        if (game.id > 0) {
            gameElement->SetAttribute("id", (int64_t) game.id);
        }
        Api::addXmlElement(&doc, gameElement, "path", game.path);
        Api::addXmlElement(&doc, gameElement, "name", game.name);
        Api::addXmlElement(&doc, gameElement, "desc", game.synopsis);
        if (game.rating > 0) {
            std::string rating = std::to_string((float) game.rating / 20.0f);
            Api::addXmlElement(&doc, gameElement, "rating", rating.substr(0, rating.find('.') + 3));
        }
        if (!game.date.empty() && game.date != "UNKNOWN") {
            Api::addXmlElement(&doc, gameElement, "releasedate", game.date + "0101T000000");
        }
        if (game.developer.id > 0) {
            elem = Api::addXmlElement(&doc, gameElement, "developer", game.developer.name);
            if (elem) elem->SetAttribute("id", game.developer.id);
        }
        if (game.editor.id > 0) {
            elem = Api::addXmlElement(&doc, gameElement, "publisher", game.editor.name);
            if (elem) elem->SetAttribute("id", game.editor.id);
        }
        if (game.genre.id > 0) {
            Api::addXmlElement(&doc, gameElement, "genre", game.genre.name);
            Api::addXmlElement(&doc, gameElement, "genreid", std::to_string(game.genre.id));
        }
        Api::addXmlElement(&doc, gameElement, "players", game.players);
        // pemu
        Api::addXmlElement(&doc, gameElement, "cloneof", game.cloneOf);
        elem = Api::addXmlElement(&doc, gameElement, "system", game.system.name);
        if (elem) {
            elem->SetAttribute("id", game.system.id);
            elem->SetAttribute("parentid", game.system.parentId);
        }
        if (game.rotation != 0) {
            Api::addXmlElement(&doc, gameElement, "rotation", std::to_string(game.rotation));
        }
        Api::addXmlElement(&doc, gameElement, "resolution", game.resolution);
        // pemu

        const std::vector<std::string> es_names = {"image", "thumbnail", "video"};
        const std::vector<std::string> ss_names = {imageType, thumbnailType, videoType};
        for (size_t i = 0; i < 3; i++) {
            Game::Media media = game.getMedia(ss_names.at(i));
            if (!media.url.empty()) {
                std::string mediaPath = media.url;
                if (mediaPath.rfind("http", 0) == 0) {
                    mediaPath = "media/" + ss_names.at(i) + "/"
                                + game.path.substr(0, game.path.find_last_of('.') + 1) + media.format;
                }
                elem = Api::addXmlElement(&doc, gameElement, es_names.at(i), mediaPath);
                if (elem) elem->SetAttribute("type", media.type.c_str());
            }
        }
        pGames->InsertEndChild(gameElement);
    }

    tinyxml2::XMLError e = doc.SaveFile(dstPath.c_str());
    if (e != tinyxml2::XML_SUCCESS) {
        SS_PRINT("GameList::save: %s\n", doc.ErrorName());
        doc.Clear();
        return false;
    }

    doc.Clear();

    return true;
}

GameList GameList::filter(bool available, bool clones, int system, int parent_system,
                          int editor, int developer, int player, int rating, int rotation, int genre,
                          const std::string &resolution, const std::string &date) {
    GameList gl;
    // update gamelist information
    gl.xml = xml;
    gl.romPaths = romPaths;
    gl.editors = editors;
    gl.developers = developers;
    gl.players = players;
    gl.ratings = ratings;
    gl.rotations = rotations;
    gl.resolutions = resolutions;
    gl.dates = dates;
    gl.genres = genres;
    gl.systemList = systemList;

    // update gamelist games
    std::copy_if(games.begin(), games.end(), std::back_inserter(gl.games),
                 [available, clones, system, parent_system, editor, developer, player, rating,
                         rotation, genre, resolution, date](const Game &game) {
                     return (!available || (available && game.available))
                            && (clones || !game.isClone())
                            && (system == -1 || game.system.id == system)
                            && (parent_system == -1 || game.system.parentId == parent_system)
                            && (editor == -1 || game.editor.id == editor)
                            && (developer == -1 || game.developer.id == developer)
                            && (player == -1 || game.playersInt == player)
                            && (rating == -1 || game.rating == rating)
                            && (rotation == -1 || game.rotation == rotation)
                            && (genre == -1 || game.genre.id == genre)
                            && (resolution == "ALL" || game.resolution == resolution
                                || game.resolution.empty() && resolution == "UNKNOWN")
                            && (date == "ALL" || game.date == date);
                 });
    return gl;
}

std::vector<Game> GameList::findGamesByName(const std::string &name) {
    std::vector<Game> matches;

    auto it = std::copy_if(games.begin(), games.end(), std::back_inserter(matches), [name](const Game &game) {
        return game.name == name;
    });

    return matches;
}

std::vector<Game> GameList::findGamesByName(const Game &game) {
    std::vector<Game> matches;

    auto it = std::copy_if(games.begin(), games.end(), std::back_inserter(matches), [game](const Game &g) {
        return game.name == g.name && game.path != g.path;
    });

    return matches;
}

std::vector<Game> GameList::findGamesBySystem(int systemId) {
    std::vector<Game> matches;

    auto it = std::copy_if(games.begin(), games.end(), std::back_inserter(matches), [systemId](const Game &game) {
        return game.system.id == systemId;
    });

    return matches;
}

Game GameList::findGameById(unsigned long id) {
    auto it = std::find_if(games.begin(), games.end(), [id](const Game &game) {
        return game.id == id;
    });

    if (it != games.end()) {
        return *it;
    }

    return {};
}

Game GameList::findGameByPath(const std::string &path) {
    auto it = std::find_if(games.begin(), games.end(), [path](const Game &game) {
        return game.path == path;
    });

    if (it != games.end()) {
        return *it;
    }

    return {};
}

Game GameList::findGameByPathAndSystem(const std::string &path, int systemId) {
    auto it = std::find_if(games.begin(), games.end(), [path, systemId](const Game &game) {
        return game.system.id == systemId && game.path == path;
    });

    if (it != games.end()) {
        return *it;
    }

    return {};
}

Game::Editor GameList::findEditorByName(const std::string &name) {
    auto it = std::find_if(editors.begin(), editors.end(), [name](const Game::Editor &ed) {
        return ed.name == name;
    });

    if (it != editors.end()) {
        return *it;
    }

    return {};
}

std::vector<std::string> GameList::getEditorNames() {
    std::vector<std::string> list;
    list.emplace_back("ALL");
    for (const auto &ed: editors) {
        list.emplace_back(ed.name);
    }
    return list;
}

Game::Developer GameList::findDeveloperByName(const std::string &name) {
    auto it = std::find_if(developers.begin(), developers.end(), [name](const Game::Developer &dev) {
        return dev.name == name;
    });

    if (it != developers.end()) {
        return *it;
    }

    return {};
}

Game::Genre GameList::findGenreByName(const std::string &name) {
    auto it = std::find_if(genres.begin(), genres.end(), [name](const Game::Genre &genre) {
        return genre.name == name;
    });

    if (it != genres.end()) {
        return *it;
    }

    return {};
}

std::vector<std::string> GameList::getDeveloperNames() {
    std::vector<std::string> list;
    list.emplace_back("ALL");
    for (const auto &dev: developers) {
        list.emplace_back(dev.name);
    }
    return list;
}

std::vector<std::string> GameList::getGenreNames() {
    std::vector<std::string> list;
    list.emplace_back("ALL");
    for (const auto &genre: genres) {
        list.emplace_back(genre.name);
    }
    return list;
}

std::vector<std::string> GameList::getRatingNames() {
    std::vector<std::string> list;
    list.emplace_back("ALL");
    for (const auto &i: ratings) {
        list.emplace_back(std::to_string(i));
    }
    return list;
}

std::vector<std::string> GameList::getRotationNames() {
    std::vector<std::string> list;
    list.emplace_back("ALL");
    for (const auto &i: rotations) {
        list.emplace_back(std::to_string(i));
    }
    return list;
}

std::vector<std::string> GameList::getPlayersNames() {
    std::vector<std::string> list;
    list.emplace_back("ALL");
    for (const auto &i: players) {
        list.emplace_back(std::to_string(i));
    }
    return list;
}

std::vector<std::string> GameList::getDates() {
    std::vector<std::string> list;
    list.emplace_back("ALL");
    for (const auto &i: dates) {
        list.emplace_back(i);
    }
    return list;
}

std::vector<std::string> GameList::getResolutions() {
    std::vector<std::string> list;
    list.emplace_back("ALL");
    for (const auto &i: resolutions) {
        list.emplace_back(i);
    }
    return list;
}

bool GameList::exist(unsigned long id) {
    auto it = std::find_if(games.begin(), games.end(), [id](const Game &game) {
        return game.id == id;
    });

    return it != games.end();
}

bool GameList::remove(unsigned long id) {
    auto it = std::find_if(games.begin(), games.end(), [id](const Game &game) {
        return game.id == id;
    });

    if (it != games.end()) {
        games.erase(it);
        return true;
    }

    return false;
}

size_t GameList::getAvailableCount(int systemId) {
    if (systemId < 0) {
        return std::count_if(games.begin(), games.end(), [](const Game &game) {
            return game.available;
        });
    } else {
        return std::count_if(games.begin(), games.end(), [systemId](const Game &game) {
            return game.available && game.system.id == systemId;
        });
    }
}

size_t GameList::getCount(int systemId) {
    if (systemId < 0) return games.size();
    return std::count_if(games.begin(), games.end(), [systemId](const Game &game) {
        return game.system.id == systemId;
    });
}
// =============================================================================
// binary cache (pemu: fast startup without rescanning roms)
// format: "PCGL" magic + u32 version + u32 count, then per game all fields,
// strings length-prefixed (u32 + bytes), integers native LE.
// =============================================================================
static void writeStr(std::ofstream &f, const std::string &s) {
    uint32_t len = (uint32_t) s.size();
    f.write((const char *) &len, 4);
    if (len) f.write(s.data(), (std::streamsize) len);
}

static void writeI32(std::ofstream &f, int32_t v) { f.write((const char *) &v, 4); }
static void writeU32(std::ofstream &f, uint32_t v) { f.write((const char *) &v, 4); }
static void writeU64(std::ofstream &f, uint64_t v) { f.write((const char *) &v, 8); }

bool GameList::saveBinary(const std::string &dstPath) {
    std::ofstream f(dstPath, std::ios::binary | std::ios::trunc);
    if (!f.is_open()) {
        SS_PRINT("GameList::saveBinary: open failed: %s\n", dstPath.c_str());
        return false;
    }

    f.write("PCGL", 4);
    writeU32(f, 1);
    writeU32(f, (uint32_t) romPaths.size());
    for (const auto &rp : romPaths) {
        writeStr(f, rp);
    }
    writeU32(f, (uint32_t) games.size());

    for (const auto &g : games) {
        writeU64(f, (uint64_t) g.id);
        writeI32(f, g.rating);
        writeI32(f, g.rotation);
        f.put(g.available ? 1 : 0);
        writeI32(f, g.playersInt);
        writeStr(f, g.cloneOf);
        writeStr(f, g.players);
        writeStr(f, g.resolution);
        writeStr(f, g.name);
        writeStr(f, g.synopsis);
        writeStr(f, g.date);
        writeStr(f, g.path);
        writeStr(f, g.romsPath);
        writeI32(f, g.system.id);
        writeI32(f, g.system.parentId);
        writeStr(f, g.system.name);
        writeI32(f, g.editor.id);
        writeStr(f, g.editor.name);
        writeI32(f, g.developer.id);
        writeStr(f, g.developer.name);
        writeI32(f, g.genre.id);
        writeStr(f, g.genre.name);
        writeU32(f, (uint32_t) g.medias.size());
        for (const auto &m : g.medias) {
            writeStr(f, m.url);
            writeStr(f, m.type);
            writeStr(f, m.format);
        }
    }

    f.close();
    printf("GameList::saveBinary: %s, %u games\n", dstPath.c_str(), (unsigned int) games.size());
    return true;
}

bool GameList::loadBinary(const std::string &srcPath) {
    // bulk-read the whole cache file in one shot: the old per-field ifstream
    // read() ran ~20+ syscalls per game (~100k for 5000 games) on slow sd
    std::ifstream f(srcPath, std::ios::binary | std::ios::ate);
    if (!f.is_open()) {
        return false;
    }
    std::streamsize size = f.tellg();
    f.seekg(0, std::ios::beg);
    std::vector<char> data((size_t) (size > 0 ? size : 0));
    if (size > 0 && !f.read(data.data(), size)) {
        return false;
    }
    const char *p = data.data();
    const char *end = p + data.size();

    auto readU32 = [&](uint32_t &v) -> bool {
        if (end - p < 4) return false;
        std::memcpy(&v, p, 4);
        p += 4;
        return true;
    };
    auto readI32 = [&](int32_t &v) -> bool {
        if (end - p < 4) return false;
        std::memcpy(&v, p, 4);
        p += 4;
        return true;
    };
    auto readU64 = [&](uint64_t &v) -> bool {
        if (end - p < 8) return false;
        std::memcpy(&v, p, 8);
        p += 8;
        return true;
    };
    auto readStr = [&](std::string &s) -> bool {
        uint32_t len = 0;
        if (!readU32(len)) return false;
        if ((size_t) (end - p) < len) return false;
        s.assign(p, len);
        p += len;
        return true;
    };

    char magic[4] = {0};
    if (end - p < 4) {
        return false;
    }
    std::memcpy(magic, p, 4);
    p += 4;
    if (std::memcmp(magic, "PCGL", 4) != 0) {
        SS_PRINT("GameList::loadBinary: bad magic in %s\n", srcPath.c_str());
        return false;
    }

    uint32_t version = 0, romPathCount = 0, count = 0;
    if (!readU32(version) || version != 1 || !readU32(romPathCount)) {
        return false;
    }
    romPaths.clear();
    for (uint32_t i = 0; i < romPathCount; i++) {
        std::string rp;
        if (!readStr(rp)) return false;
        romPaths.emplace_back(rp);
    }
    if (!readU32(count)) {
        return false;
    }

    games.clear();
    games.reserve(count);
    for (uint32_t i = 0; i < count; i++) {
        Game g;
        uint64_t id = 0;
        int32_t v = 0;
        if (!readU64(id)) return false;
        g.id = (unsigned long) id;
        if (!readI32(g.rating)) return false;
        if (!readI32(g.rotation)) return false;
        if (end - p < 1) return false;
        g.available = *p++ == 1;
        if (!readI32(g.playersInt)) return false;
        if (!readStr(g.cloneOf)) return false;
        if (!readStr(g.players)) return false;
        if (!readStr(g.resolution)) return false;
        if (!readStr(g.name)) return false;
        if (!readStr(g.synopsis)) return false;
        if (!readStr(g.date)) return false;
        if (!readStr(g.path)) return false;
        if (!readStr(g.romsPath)) return false;
        if (!readI32(g.system.id)) return false;
        if (!readI32(g.system.parentId)) return false;
        if (!readStr(g.system.name)) return false;
        if (!readI32(g.editor.id)) return false;
        if (!readStr(g.editor.name)) return false;
        if (!readI32(g.developer.id)) return false;
        if (!readStr(g.developer.name)) return false;
        if (!readI32(g.genre.id)) return false;
        if (!readStr(g.genre.name)) return false;
        uint32_t mcount = 0;
        if (!readU32(mcount)) return false;
        g.medias.reserve(mcount);
        for (uint32_t m = 0; m < mcount; m++) {
            Game::Media media;
            if (!readStr(media.url)) return false;
            if (!readStr(media.type)) return false;
            if (!readStr(media.format)) return false;
            g.medias.emplace_back(media);
        }
        games.emplace_back(std::move(g));
    }

    // rebuild the filter lists from the loaded games (dedup via sets instead
    // of the O(n^2) find_if over growing vectors)
    systemList.systems.clear();
    editors.clear();
    developers.clear();
    genres.clear();
    players.clear();
    ratings.clear();
    rotations.clear();
    resolutions.clear();
    dates.clear();

    std::unordered_set<int> editorIds, devIds, genreIds;
    std::unordered_set<std::string> editorNames, devNames, genreNames;
    std::unordered_set<std::string> resolutionSet, dateSet;

    for (const auto &g : games) {
        if (!systemList.find(g.system)) {
            systemList.systems.emplace_back(g.system);
        }
        if (!editorIds.count(g.editor.id) && !editorNames.count(g.editor.name)) {
            editors.emplace_back(g.editor);
            editorIds.insert(g.editor.id);
            editorNames.insert(g.editor.name);
        }
        if (!devIds.count(g.developer.id) && !devNames.count(g.developer.name)) {
            developers.emplace_back(g.developer);
            devIds.insert(g.developer.id);
            devNames.insert(g.developer.name);
        }
        if (!genreIds.count(g.genre.id) && !genreNames.count(g.genre.name)) {
            genres.emplace_back(g.genre);
            genreIds.insert(g.genre.id);
            genreNames.insert(g.genre.name);
        }
        if (std::find(players.begin(), players.end(), g.playersInt) == players.end()) {
            players.emplace_back(g.playersInt);
        }
        if (std::find(ratings.begin(), ratings.end(), g.rating) == ratings.end()) {
            ratings.emplace_back(g.rating);
        }
        if (std::find(rotations.begin(), rotations.end(), g.rotation) == rotations.end()) {
            rotations.emplace_back(g.rotation);
        }
        std::string resolution = g.resolution.empty() ? "UNKNOWN" : g.resolution;
        if (resolutionSet.insert(resolution).second) {
            resolutions.emplace_back(resolution);
        }
        if (dateSet.insert(g.date).second) {
            dates.emplace_back(g.date);
        }
    }

    printf("GameList::loadBinary: %s, %u games, %zu systems\n",
           srcPath.c_str(), (unsigned int) games.size(), systemList.systems.size());
    return true;
}
