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

#include "console.h"

#define START_BREAKABLE do {
#define END_BREAKABLE   } while (false);

static constexpr char* APP_VERSION  = "1.3.5";
static constexpr char* APP_PATH     = "sdmc:/switch/HDR_Installer/";
static constexpr char* MODS_FOLDER  = "sdmc:/ultimate/mods/";
static constexpr char* SKYLINE_PATH = "sdmc:/atmosphere/contents/01006A800016E000/romfs/skyline/plugins/";
static constexpr char* RELEASE_REPO = "blu-dev/HDR-Release-Builds";
static constexpr char* BETA_REPO    = "blu-dev/HDR-Beta-Builds";
static constexpr char* DEV_REPO     = "blu-dev/HDR-Dev-Builds";
static constexpr char* APP_REPO     = "FaultyPine/HDR-Installer-Homebrew";

static constexpr char* OAUTH_FILE   = "oauth.txt";

namespace gh {
    struct Release {
        std::string name;
        std::string tag;
    };

    enum class DownloadResult {
        SUCCESS,
        CURL_ERROR,
        DOES_NOT_EXIST,
        DOWNLOAD_FAILED,
        ACCESS_DENIED
    };
    typedef const char* OauthToken;
    typedef int AssetID;
    typedef int (*DownloadCallback)(void* data, double total_dl, double current_dl, double total_up, double current_up);

    bool isEndUser(OauthToken token);
    bool isBetaTester(OauthToken token);
    bool isDeveloper(OauthToken token);

    std::vector<Release> getReleases(OauthToken token, const std::string& repository);
    AssetID getAssetID(OauthToken token, const std::string& repository, const std::string& tag, const std::string& asset_name);
    DownloadResult getAsset(OauthToken token, const std::string& repository, const std::string& tag, const std::string& asser_name, const std::string& filepath, DownloadCallback cb);
}

gh::OauthToken loadOauthToken();
void destroyOauthToken(gh::OauthToken token);