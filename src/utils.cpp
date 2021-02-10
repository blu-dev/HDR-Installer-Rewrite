#include "utils.hpp"

void pauseForText(int seconds) {
    consoleUpdate(NULL);
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
}

namespace { // CURL helper stuff
    struct  CURL_builder {
        CURL* request;
        curl_slist* headers;

        CURL_builder() : request(nullptr), headers(nullptr) { request = curl_easy_init(); }
        ~CURL_builder() {
            if (request != nullptr)
                curl_easy_cleanup(request);
            if (headers != nullptr)
                curl_slist_free_all(headers);
        }

        operator bool() { return request != nullptr; } // for convenience
        template <typename T>
        CURL_builder& SetOPT(CURLoption opt, T param) { curl_easy_setopt(request, opt, param); return *this; } // generic to mimic the #define macro CURL uses
        CURL_builder& SetHeaders(std::vector<std::string> _headers) {
            if (headers != nullptr)
                curl_slist_free_all(headers);
            headers = nullptr;
            for (std::string x : _headers)
                headers = curl_slist_append(headers, x.c_str());
            SetOPT(CURLOPT_HTTPHEADER, headers);
            return *this;
        }
        CURL_builder& SetURL(const std::string& url) { SetOPT(CURLOPT_URL, url.c_str()); return *this; }
        CURLcode Perform() { return curl_easy_perform(request); }
    };

    std::string makeAuthHeader(gh::OauthToken token) {
        std::stringstream buffer; // it's better to use stringstreams instead of string + string + ... + string
        buffer << "Authorization: token " << token; // GitHub API V3
        return buffer.str();
    }

    size_t jsonWriteCallback(char* to_write, size_t size, size_t byte_count, void* user_data) {
        *(std::stringstream*)user_data << to_write;
        return size * byte_count;
    }

    /*
    const int NUM_PROGRESS_CHARS = 50;
    void print_progress(size_t progress, size_t max) {
        size_t prog_chars;
        if (max == 0) prog_chars = NUM_PROGRESS_CHARS;
        else prog_chars = ((float) progress / max) * NUM_PROGRESS_CHARS;

        std::cout << "\n\n\n";
        std::cout << "Downloading... Please be patient\n\n";
        if (prog_chars < NUM_PROGRESS_CHARS) std::cout << YELLOW;
        else std::cout << GREEN;

        std::cout << "[";
        for (size_t i = 0; i < prog_chars; i++) { // some some godforsaken reason, both this for loop and the one a few lines below cause everything to lock up and stop downloading...
            std::cout << "=";
        }

        if (prog_chars < NUM_PROGRESS_CHARS) std::cout << ">";
        else std::cout << "=";

        for (size_t i = 0; i < NUM_PROGRESS_CHARS - prog_chars; i++) {
            std::cout << " ";
        }

        std::cout << "]\t" << progress << "%\n" RESET;
    }
    */

    int download_progress(void* ptr, double TotalToDownload, double NowDownloaded, double TotalToUpload, double NowUploaded) {
        consoleClear();
        int percent_complete = (int)((NowDownloaded/TotalToDownload)*100.0);
        std::cout << WHITE "\n\nDownloading... " << percent_complete << "%\n" RESET;
        //print_progress( percent_complete, 100 ); // percent out of 100
        std::cout << "\nPress B to cancel\n";
        consoleUpdate(NULL);

        hidScanInput();
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
        if (kDown & KEY_B) {
            return 1;
        }

        // if you don't return 0, the transfer will be aborted
        return 0; 
    }
}

namespace gh {

    namespace { // gh detail stuff
        enum GithubPermissions {
            NONE = 0x0,
            ADMIN = 0x1,
            PUSH = 0x2,
            PULL = 0x4
        };

        inline GithubPermissions operator|(GithubPermissions a, GithubPermissions b) {
        	return static_cast<GithubPermissions>(static_cast<int>(a) | static_cast<int>(b));
        }
        inline GithubPermissions operator&(GithubPermissions a, GithubPermissions b) {
        	return static_cast<GithubPermissions>(static_cast<int>(a) & static_cast<int>(b));
        }
        inline GithubPermissions operator^(GithubPermissions a, GithubPermissions b) {
        	return static_cast<GithubPermissions>(static_cast<int>(a) ^ static_cast<int>(b));
        }
        inline GithubPermissions operator~(GithubPermissions a) {
            return static_cast<GithubPermissions>(~static_cast<int>(a) & 0x7);
        }
        inline GithubPermissions& operator|=(GithubPermissions& a, GithubPermissions b) {
        	a = a | b;
        	return a;
        }
        inline GithubPermissions& operator&=(GithubPermissions& a, GithubPermissions b) {
        	a = a & b;
        	return a;
        }
        inline GithubPermissions& operator^=(GithubPermissions& a, GithubPermissions b) {
        	a = a ^ b;
        	return a;
        }

        /// token: User authentication token. If (token == nullptr), this function will return perform the check if it is a public repository
        /// full_name: Name of the repository
        /// permissions: The permission set to be checked
        bool userHasPermissions(OauthToken token, const std::string& full_name, GithubPermissions permissions) {
            bool ret = false;
            if (token == nullptr && (permissions & ~GithubPermissions::PULL) != 0) // Even if we have read access, we won't have any other perms
                return ret;
            CURL_builder curl;
            if (curl) {
                START_BREAKABLE
                if (token != nullptr)
                    curl.SetHeaders({ makeAuthHeader(token) });
                
                std::stringstream buffer;
                buffer << "https://api.github.com/repos/" << full_name;
                std::string api_url = buffer.str();
                buffer.str(""); buffer.clear(); // clears bits and set string to nothing
                CURLcode result =
                    curl.SetURL(api_url.c_str())
                        .SetOPT(CURLOPT_WRITEDATA, &buffer)
                        .SetOPT(CURLOPT_WRITEFUNCTION, jsonWriteCallback)
                        .SetOPT(CURLOPT_USERAGENT, "HDR-User")
                        .Perform();
                json parsed;
                try { parsed = json::parse(buffer.str()); }
                catch (json::parse_error& e) { break; }
                if (!parsed.is_object() || !parsed.contains("full_name")) // a bad response won't contain the full_name key 
                    break;
                if (token == nullptr) { // if we are this far and there is no user, it is a public repository
                    ret = true;
                    break;
                }
                json perms_list = parsed["permissions"];
                GithubPermissions repo_perms = GithubPermissions::NONE;
                if (perms_list["admin"].get<bool>()) repo_perms |= GithubPermissions::ADMIN;
                if (perms_list["push"].get<bool>()) repo_perms |= GithubPermissions::PUSH;
                if (perms_list["pull"].get<bool>()) repo_perms |= GithubPermissions::PULL;
                ret = ((repo_perms & permissions) == permissions);
                END_BREAKABLE
            }
            return ret;
        }
    }

    bool isEndUser(OauthToken token) {
        return userHasPermissions(token, RELEASE_REPO, GithubPermissions::PULL);
    }

    bool isBetaTester(OauthToken token) {
        return userHasPermissions(token, BETA_REPO, GithubPermissions::PULL);
    }

    bool isDeveloper(OauthToken token) {
        return userHasPermissions(token, DEV_REPO, GithubPermissions::PULL);
    }

    std::vector<Release> getReleases(OauthToken token, const std::string& repository) {
        std::vector<Release> ret = std::vector<Release>();
        if (!userHasPermissions(token, repository, GithubPermissions::PULL))
            return ret;
        CURL_builder curl;
        if (curl) {
            START_BREAKABLE
            if (token != nullptr)
                curl.SetHeaders({ makeAuthHeader(token) });
            
            std::stringstream buffer;
            buffer << "https://api.github.com/repos/" << repository << "/releases";
            std::string api_url = buffer.str();
            buffer.str(""); buffer.clear(); // clears bits and set string to nothing


            CURLcode result =
                curl.SetURL(api_url.c_str())
                    .SetOPT(CURLOPT_WRITEDATA, &buffer)
                    .SetOPT(CURLOPT_WRITEFUNCTION, jsonWriteCallback)
                    .SetOPT(CURLOPT_USERAGENT, "HDR-User")
                    .Perform();
            if (result != CURLE_OK)
                break;
            json parsed;
            try { parsed = json::parse(buffer.str()); }
            catch (json::parse_error& e) { break; }
            if (!parsed.is_array()) // results come in a json array
                break;
            for (auto& x : parsed.items()) {
                auto value = x.value();
                if (!value.contains("name") || !value.contains("tag_name") || !value.contains("body"))
                    continue;
                ret.push_back({ value["name"].get<std::string>(), value["tag_name"].get<std::string>(), value["body"].get<std::string>() });
            }
            END_BREAKABLE
        }
        return ret;
    }

    
    AssetInfos getReleaseInfos(OauthToken token, const std::string& repository, const std::string& tag) {
        AssetInfos ret = {};
        if (!userHasPermissions(token, repository, GithubPermissions::PULL)) {
            std::cout << RED "\nUser does not have permissions for that repo\n" RESET;
            pauseForText(2);
            return ret;
        }
        CURL_builder curl;
        if (curl) {
            START_BREAKABLE
            if (token != nullptr) {
                curl.SetHeaders({ makeAuthHeader(token) });
            }
            else {
                std::cout << RED "\nInvalid token passed!\n" RESET;
                pauseForText(2);
            }
            
            std::stringstream buffer;
            buffer << "https://api.github.com/repos/" << repository << "/releases/tags/" << tag;
            std::string api_url = buffer.str();
            buffer.str(""); buffer.clear();

            CURLcode result =
                curl.SetURL(api_url.c_str())
                    .SetOPT(CURLOPT_WRITEDATA, &buffer)
                    .SetOPT(CURLOPT_WRITEFUNCTION, jsonWriteCallback)
                    .SetOPT(CURLOPT_USERAGENT, "HDR-User")
                    .Perform();
            if (result != CURLE_OK) {
                std::cout << RED "\nBad curl attempt\n" RESET;
                pauseForText(2);
                break;
            }
            json parsed;
            try { parsed = json::parse(buffer.str()); }
            catch (json::parse_error& e) { break; }
            if (!parsed.is_object() || !parsed.contains("assets")) {
                std::cout << RED "\nFailed to parse json properly\n" RESET;
                pauseForText(2);
                break;
            }
            json assets = parsed["assets"];
            for (auto& x : assets.items()) {
                auto value = x.value();
                ret.push_back({ 
                    std::filesystem::path(value["url"].get<std::string>()), 
                    value["content_type"].get<std::string>(),
                    value["name"].get<std::string>()
                });
            }
            END_BREAKABLE
        }
        else {
            std::cout << RED "\nFailed to build CURL object\n" RESET;
            pauseForText(2);
        }
        return ret;
    }

    DownloadResult downloadRelease(OauthToken token, const std::string& repository, const std::string& tag, const std::string& filepath_root) {
        DownloadResult ret = DownloadResult::CURL_ERROR;
        if (!userHasPermissions(token, repository, GithubPermissions::PULL))
            return ret;
        CURL_builder curl;
        if (curl) {
            START_BREAKABLE
            AssetInfos assets = getReleaseInfos(token, repository, tag);
            if (assets.size() < 1) {
                ret = DownloadResult::DOES_NOT_EXIST;
                break;
            }

            std::vector<std::string> headers;
            if (token != nullptr) headers.push_back(makeAuthHeader(token));
            headers.push_back("Accept: application/octet-stream");

            for (size_t i = 0; i < assets.size(); i++) {

                std::filesystem::path url = assets[i].url;

                std::string path = filepath_root + url.filename().string();
                FILE* file = fopen(path.c_str(), "wb"); // using C file IO because that is what CURL requires

                if (assets.size() > 1)
                    std::cout << "\nDownloading multiple files... " GREEN "(" << i+1 << "/" << assets.size() << ")\n" RESET;

                CURLcode result =
                    curl.SetHeaders(headers)
                        .SetURL(url.c_str())
                        .SetOPT(CURLOPT_WRITEDATA, file)
                        .SetOPT(CURLOPT_USERAGENT, "HDR-User")
                        .SetOPT(CURLOPT_FOLLOWLOCATION, 1L)
                        .SetOPT(CURLOPT_SSL_VERIFYPEER, 0L)
                        .SetOPT(CURLOPT_SSL_VERIFYHOST, 0L)
                        .SetOPT(CURLOPT_NOPROGRESS, 0L)
                        .SetOPT(CURLOPT_PROGRESSFUNCTION, download_progress)
                        .Perform();
                if (result != CURLE_OK) {
                    fclose(file);
                    if (std::filesystem::exists(path))
                        std::filesystem::remove(path);
                    return DownloadResult::DOWNLOAD_FAILED;
                }
                fclose(file);
                if (std::filesystem::exists(path) && assets[i].content_type == "application/zip") { // if it's a zip, extract to root then delete it
                    std::cout << GREEN "\nExtracting...\n" RESET;
                    consoleUpdate(NULL);
                    //if (!std::filesystem::exists(TMP_EXTRACTED))
                        //std::filesystem::create_directories(TMP_EXTRACTED);
                    elz::extractZip(path, SYSTEM_ROOT/*TMP_EXTRACTED*/);
                    consoleClear();
                    std::filesystem::remove(path);
                }
                else { // otherwise, just rename the file to it's proper name instead of it's asset id
                    std::filesystem::path new_path = filepath_root + assets[i].filename;
                    rename(path.c_str(), new_path.c_str());
                }
            }
            ret = DownloadResult::SUCCESS;
            END_BREAKABLE
        }
        return ret;
    }
}

gh::OauthToken loadOauthToken() {
    gh::OauthToken ret = nullptr;
    std::stringstream buffer;
    buffer << APP_PATH << OAUTH_FILE;
    std::string oauth_path = buffer.str();

    if (std::filesystem::exists(oauth_path)) {
        std::ifstream file;
        file.open(oauth_path, std::ios_base::in);
        if (file.is_open()) {
            buffer.str(""); buffer.clear();
            buffer << file.rdbuf();
            std::string token = buffer.str();
            char* tmp = new char[token.size() + 1];
            tmp[token.size()] = '\0';
            token.copy(tmp, token.size());
            ret = tmp;
        }
        file.close();
    }
    return ret;
}

void destroyOauthToken(gh::OauthToken token) {
    delete token;
}

void prep() {
    std::string dirs[3] = {
        MODS_FOLDER,
        APP_PATH,
        SKYLINE_PATH,
    };
    for (std::string dir : dirs) {
        if (!std::filesystem::exists(dir))
            std::filesystem::create_directories(dir);
    }

    /*
    if (!std::filesystem::exists(INSTALLED_MODS)) { // installed mods file doesn't exist
        installed_json["Installed"] = {};
        SaveJson(installed_json);
    }
    else { // installed mods file exists, parse json data from it
        installed_json = GetJson();
    }

    if (!installed_json.is_object() || !installed_json.contains("Installed")) {
        std::cout << RED "\nInstalled mods json file is malformed!\n" RESET;
        pauseForText(3);
    }
    */
}


/*
void SaveJson(json j_obj, std::filesystem::path path) {
    if (std::filesystem::exists(path))
        std::filesystem::remove(path);

    std::ofstream writer;
    writer.open(path);
    if (writer.is_open())
        writer << j_obj.dump(4);
    writer.close();
}

json GetJson(std::filesystem::path path) {
    json ret = NULL;

    if (!std::filesystem::exists(path)) {
        std::cout << "Json path does not exist!\n"; pauseForText(3);
    }

    std::ifstream reader;
    std::stringstream buffer;
    reader.open(path);
    if (reader.is_open())
        buffer << reader.rdbuf();
    reader.close();

    try { ret = json::parse(buffer.str()); }
    catch (json::parse_error& e) { std::cout << "\n\n" << e.what() << "\n"; pauseForText(3); }

    return ret;
}


void UninstallRelease(std::string release_title) {
    for (std::pair<std::string, bool> pair : installed_json["Installed"][release_title]["Files"].get<std::vector<std::pair<std::string, bool>>>()) {
        // If the path exists and that path didn't exist before we installed that file, delete it
        if (std::filesystem::exists(pair.first) && !pair.second) {
            std::filesystem::remove(pair.first);
        }
    }
    installed_json["Installed"].erase(release_title);
}
*/