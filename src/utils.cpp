#include "utils.hpp"
#include "json.hpp"

using json = nlohmann::json;

namespace { // CURL helper stuff
    struct CURL_builder {
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
                if (!value.contains("name") || !value.contains("tag_name"))
                    continue;
                ret.push_back({ value["name"].get<std::string>(), value["tag_name"].get<std::string>() });
            }
            END_BREAKABLE
        }
        return ret;
    }

    AssetID getAssetID(OauthToken token, const std::string& repository, const std::string& tag, const std::string& asset_name) {
        AssetID ret = -1;
        if (!userHasPermissions(token, repository, GithubPermissions::PULL))
            return ret;
        CURL_builder curl;
        if (curl) {
            START_BREAKABLE
            if (token != nullptr)
                curl.SetHeaders({ makeAuthHeader(token) });
            
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
            if (result != CURLE_OK)
                break;
            json parsed;
            try { parsed = json::parse(buffer.str()); }
            catch (json::parse_error& e) { break; }
            if (!parsed.is_object() || !parsed.contains("assets"))
                break;
            json assets = parsed["assets"];
            for (auto& x : assets.items()) {
                auto value = x.value();
                if (value.contains("name") && value["name"].get<std::string>() == asset_name) {
                    ret = value["id"].get<int>();
                    break;
                }
            }
            END_BREAKABLE
        }
        return ret;
    }
    
    DownloadResult getAsset(OauthToken token, const std::string& repository, const std::string& tag, const std::string& asset_name, const std::string& filepath, DownloadCallback cb) {
        DownloadResult ret = DownloadResult::CURL_ERROR;
        if (!userHasPermissions(token, repository, GithubPermissions::PULL))
            return ret;
        CURL_builder curl;
        if (curl) {
            START_BREAKABLE
            AssetID id = getAssetID(token, repository, tag, asset_name);
            if (id == -1) {
                ret = DownloadResult::DOES_NOT_EXIST;
                break;
            }

            std::vector<std::string> headers;
            if (token != nullptr) headers.push_back(makeAuthHeader(token));
            headers.push_back("Accept: application/octet-stream"); // In order to get the full file downloaded, we have to specify this

            std::stringstream buffer;
            buffer << "https://api.github.com/repos/" << repository << "/releases/assets/" << id;
            std::string api_url = buffer.str();
            buffer.str(""); buffer.clear();

            FILE* file = fopen(filepath.c_str(), "wb"); // using C file IO because that is what CURL requires

            if (cb != nullptr)
                curl.SetOPT(CURLOPT_NOPROGRESS, 0L).SetOPT(CURLOPT_PROGRESSFUNCTION, cb);
            CURLcode result =
                curl.SetHeaders(headers)
                    .SetURL(api_url.c_str())
                    .SetOPT(CURLOPT_WRITEDATA, file)
                    .SetOPT(CURLOPT_USERAGENT, "HDR-USer")
                    .Perform();
            if (result != CURLE_OK) {
                ret = DownloadResult::DOWNLOAD_FAILED;
                break;
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