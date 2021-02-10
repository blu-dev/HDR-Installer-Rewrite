#pragma once
#include <switch.h>
#include <filesystem>
#include <sys/select.h>
#include <curl/curl.h>

#include <iostream>
#include <cstring>
#include <vector>
#include <cmath>
#include <thread>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <time.h>
#include <filesystem>
#include <stdio.h>
#include "json.hpp"
#include <elzip/elzip.hpp>

#include "console.h"

using json = nlohmann::json;

#define START_BREAKABLE do {
#define END_BREAKABLE   } while (false);

static constexpr char* APP_VERSION    = "1.3.5";
static constexpr char* APP_PATH       = "sdmc:/switch/HDR_Installer/";
static constexpr char* MODS_FOLDER    = "sdmc:/ultimate/mods/";
//static constexpr char* INSTALLED_MODS = "sdmc:/switch/HDR_Installer/Installed.json";
//static constexpr char* TMP_EXTRACTED  = "sdmc:/TempExtractedFiles";
static constexpr char* SYSTEM_ROOT    = "sdmc:/";
static constexpr char* SKYLINE_PATH   = "sdmc:/atmosphere/contents/01006A800016E000/romfs/skyline/plugins/";
static constexpr char* RELEASE_REPO   = "blu-dev/HDR-Release-Builds";
static constexpr char* BETA_REPO      = "blu-dev/HDR-Beta-Builds";
static constexpr char* DEV_REPO       = "blu-dev/HDR-Dev-Builds";
static constexpr char* APP_REPO       = "FaultyPine/HDR-Installer-Homebrew";

static constexpr char* OAUTH_FILE   = "oauth.txt";

namespace gh {
    struct Release {
        std::string name;
        std::string tag;
        std::string body;
    };
    struct AssetInfo {
        std::filesystem::path url;
        std::string content_type;
        std::string filename;
    };

    enum class DownloadResult {
        SUCCESS,
        CURL_ERROR,
        DOES_NOT_EXIST,
        DOWNLOAD_FAILED,
        ACCESS_DENIED
    };
    typedef const char* OauthToken;
    // using paths for urls gives access to really helpful member funcs
    // tuple is : (asset url, asset content type, asset filename)
    typedef std::vector<AssetInfo> AssetInfos;
    typedef int (*DownloadCallback)(void* data, double total_dl, double current_dl, double total_up, double current_up);

    bool isEndUser(OauthToken token);
    bool isBetaTester(OauthToken token);
    bool isDeveloper(OauthToken token);

    std::vector<Release> getReleases(OauthToken token, const std::string& repository);
    DownloadResult downloadRelease(OauthToken token, const std::string& repository, const std::string& tag, const std::string& filepath_root = SYSTEM_ROOT);
}
void pauseForText(int seconds = 3);
gh::OauthToken loadOauthToken();
void destroyOauthToken(gh::OauthToken token);
void prep();
/*
extern json installed_json;
void SaveJson(json j_obj = installed_json, std::filesystem::path path = INSTALLED_MODS);
json GetJson(std::filesystem::path path = INSTALLED_MODS);
void UninstallRelease(std::string release_title);
*/