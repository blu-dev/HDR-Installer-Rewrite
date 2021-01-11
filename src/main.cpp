#include "console.h"
#include "menu.hpp"
#include "tree_node.hpp"
#include "utils.hpp"

static struct {
    gh::OauthToken token;
    bool isEndUser;
    bool isBetaTester;
    bool isDeveloper;
} user;

int main(int argc, char** argv) {
    user.token = nullptr;
    user.isEndUser = false;
    user.isBetaTester = false;
    user.isDeveloper = false;

    socketInitializeDefault();
    curl_global_init(CURL_GLOBAL_DEFAULT);

    TreeNode start;
    NodeViewer viewer(&start);
    console_init();
    console_set_status("This is a test");
    user.token = loadOauthToken();
    user.isEndUser = gh::isEndUser(user.token);
    user.isBetaTester = gh::isBetaTester(user.token);
    user.isDeveloper = gh::isDeveloper(user.token);
    std::vector<std::string> entries;
    if (user.isEndUser) {
        entries.push_back("Install HDR");
        std::vector<gh::Release> releases = gh::getReleases(user.token, RELEASE_REPO);
        if (releases.size() == 0)
            makeEmpty(start.SpawnChild(), "Install HDR", "No current release builds are avaialble.");
        else {
            std::vector<std::string> names;
            names.resize(releases.size());
            for (size_t i = 0; i < releases.size(); i++)
                names[i] = releases[i].name;
            makeMenu(start.SpawnChild(), "Install HDR", names);
        }
    }
    if (user.isBetaTester) {
        entries.push_back("Install HDR-Beta");
        std::vector<gh::Release> releases = gh::getReleases(user.token, BETA_REPO);
        if (releases.size() == 0) 
            makeEmpty(start.SpawnChild(), "Install HDR-Beta", "No current beta builds are available.");
        else {
            std::vector<std::string> names;
            names.resize(releases.size());
            for (size_t i = 0; i < releases.size(); i++)
                names[i] = releases[i].name;
            makeMenu(start.SpawnChild(), "Install HDR-Beta", names);
        }
    }
    if (user.isDeveloper) {
        entries.push_back("Install HDR-Dev");
        std::vector<gh::Release> releases = gh::getReleases(user.token, DEV_REPO);
        if (releases.size() == 0)
            makeEmpty(start.SpawnChild(), "Install HDR-Dev", "No current developer builds are available.");
        else {
            std::vector<std::string> names;
            names.resize(releases.size());
            for (size_t i = 0; i < releases.size(); i++)
                names[i] = releases[i].name;
            makeMenu(start.SpawnChild(), "Install HDR-Beta", names);
        }
    }
    makeMenu(&start, "This is a menu title", entries);
    while (appletMainLoop()) {
        consoleClear();
        hidScanInput();
        TreeNode* current = viewer.GetCurrent();
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
        if (kDown & KEY_B)
            viewer.ShiftFocus(-1);
        if (checkType(current) == NodeType::MENU) {
            size_t size = menuGetEntryCount(current);
            size_t selected = menuGetSelected(current);
            if (kDown & KEY_A)
                viewer.ShiftFocus(selected);
            else if (kDown & KEY_UP) {
                if (selected == 0)
                    selected = size;
                menuSelect(current, selected - 1);
            }
            else if (kDown & KEY_DOWN) menuSelect(current, selected + 1);
        }
        if (kDown & KEY_PLUS) break;
        viewer.Focus();
        consoleUpdate(NULL);
    }
    destroyOauthToken(user.token);
    console_exit();
    curl_global_cleanup();
    socketExit();
    return 0;
}