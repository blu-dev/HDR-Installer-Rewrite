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
//json installed_json;


void CreateReleasesMenu(TreeNode* start, std::vector<std::string>* entries, const std::string releases_name, const std::string REPO) {
    entries->push_back("Install " + releases_name);
    std::vector<gh::Release> releases = gh::getReleases(user.token, REPO);
    if (releases.size() == 0)
        makeEmpty(start->SpawnChild(), releases_name, "No current release builds are available.");
    else {
        std::vector<std::string> names;
        names.resize(releases.size());

        for (size_t i = 0; i < releases.size(); i++)
            names[i] = releases[i].name;
        makeMenu(start->SpawnChild(), releases_name + " Releases:", names);

        for (size_t i = 0; i < releases.size(); i++) {
            //bool is_installed = installed_json["Installed"].contains(releases[i].name);
            const GhDownload dl = {user.token, REPO, releases[i].tag/*, is_installed*/};
            makeDownloadable(start->GetChild(start->GetChildCount()-1)->SpawnChild(), releases[i].name, releases[i].body, dl);
        }
    }
}


const std::string console_status = "\n" RED "X" RESET " to launch smash" MAGENTA "\t\t\t\tHDR Installer Ver. " + std::string(APP_VERSION) + WHITE "\t\t\t\t\t" RED "+" RESET " to exit" RESET;

int main(int argc, char** argv) {
    appletSetCpuBoostMode(ApmCpuBoostMode_FastLoad);
    user.token = nullptr;
    user.isEndUser = false;
    user.isBetaTester = false;
    user.isDeveloper = false;

    console_init();
    console_set_status(console_status.c_str());
    socketInitializeDefault();
    curl_global_init(CURL_GLOBAL_DEFAULT);

    prep();
    /*
    if (installed_json == NULL) {
        std::cout << RED "\nError parsing previously-installed mods." RESET << "\n\nIf you don't know what this error is, \ntry deleting the file at " << INSTALLED_MODS << "\n\nExiting...\n";
        pauseForText(6);
    }
    */

    TreeNode start;
    NodeViewer viewer(&start);
    user.token = loadOauthToken();
    user.isEndUser = gh::isEndUser(user.token);
    user.isBetaTester = gh::isBetaTester(user.token);
    user.isDeveloper = gh::isDeveloper(user.token);
    std::vector<std::string> entries;
    if (user.isEndUser) {
        CreateReleasesMenu(&start, &entries, "HDR", RELEASE_REPO);
    }
    if (user.isBetaTester) {
        CreateReleasesMenu(&start, &entries, "HDR-Beta", BETA_REPO);
    }
    if (user.isDeveloper) {
        CreateReleasesMenu(&start, &entries, "HDR-Dev", DEV_REPO);
    }
    makeMenu(&start, "Main Menu", entries);
    appletSetCpuBoostMode(ApmCpuBoostMode_Normal);
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
        /* Launch smash */
        if (kDown & KEY_X) {
            std::cout << WHITE "\n\n\nLaunching smash... Please be patient, your switch hasn't froze, it's just loading.\n" RESET;
            consoleUpdate(NULL);
            appletRequestLaunchApplication(0x01006A800016E000, NULL);
        }
        if (kDown & KEY_PLUS) break;
        viewer.Focus();
        if (checkType(viewer.GetCurrent()) == NodeType::DOWNLOADABLE)
            viewer.ShiftFocus(-1);
        consoleUpdate(NULL);
    }
    destroyOauthToken(user.token);
    console_exit();
    curl_global_cleanup();
    socketExit();
    return 0;
}