#include "menu.hpp"

NodeType checkType(TreeNode* node) {
    NodeType ret = NodeType::UNKNOWN;
    uint32_t* magic_ptr = (uint32_t*)node->GetUserData();
    if (magic_ptr != nullptr) {
        switch (*magic_ptr) {
            case MENU_MAGIC:
                ret = NodeType::MENU;
                break;
            case DOWNLOADABLE_MAGIC:
                ret = NodeType::DOWNLOADABLE;
                break;
            case EMPTY_MAGIC:
                ret = NodeType::EMPTY;
                break;
            default:
                ret = NodeType::UNKNOWN;
                break;
        }
    }
    return ret;
}

struct Menu {
    uint32_t magic;
    std::string title;
    std::vector<std::string> entries;
    size_t selected;
};

void _destroyMenu(void* menu) {
    delete (Menu*)menu;
}

struct Downloadable {
    uint32_t magic;
    std::string title;
    std::string body;
    GhDownload download;
};

void _destroyDownloadable(void* dl) {
    delete (Downloadable*)dl;
}

struct Empty {
    uint32_t magic;
    std::string title;
    std::string message;
};

void _destroyEmpty(void* empty) {
    delete (Empty*)empty;
}

void menuFocus(TreeNode* node) {
    Menu& menu = *(Menu*)node->GetUserData();
    /*
    for (size_t i = 0; i < node->GetChildCount(); i++) {
        if (checkType(node->GetChild(i)) == NodeType::DOWNLOADABLE) {
            std::string instructions = RESET "\n\n\n(A -> Install, Y -> Uninstall)";
            if (menu.title.find(instructions) == std::string::npos) {
                menu.title.append(instructions.c_str());
                break;
            }
            Downloadable& child_downloadable = *(Downloadable*)node->GetChild(i)->GetUserData();
            if (child_downloadable.download.is_downloaded && menu.entries.at(i).find("(Installed)") == std::string::npos)
                menu.entries.at(i).append(" (Installed)");
        }
    }
    */
    std::cout << GREEN "\n\n" << menu.title << "\n\n\n" RESET;
    size_t child_count = menu.entries.size();
    if (child_count > 0) {
        for (size_t i = 0; i < child_count; i++) {
            if (menu.selected == i)
                std::cout << GREEN "--> " CYAN;
            std::cout << menu.entries[i] << std::endl;
            if (menu.selected == i)
                std::cout << RESET;
        }
        TreeNode* child = node->GetChild(menu.selected);
        if (checkType(child) == NodeType::DOWNLOADABLE) {
            Downloadable& downloadable = *(Downloadable*)child->GetUserData();
            std::cout << "\n\n\n" << downloadable.body;
        }
    }
}

void downloadableFocus(TreeNode* node) {
    Downloadable& downloadable = *(Downloadable*)node->GetUserData();

    /*if (installed_json["Installed"].contains(downloadable.title)) {
        std::cout << GREEN "\n\nUninstalling previous installation of " RESET << downloadable.title;
        consoleUpdate(NULL);
        UninstallRelease(downloadable.title);
        downloadable.download.is_downloaded = false;
    }*/

    time_t seconds = time(NULL);
    gh::DownloadResult dl = gh::downloadRelease(downloadable.download.token, downloadable.download.repository, downloadable.download.tag);

    std::cout << "\n";
    switch (dl) {
        case gh::DownloadResult::SUCCESS: {
            //downloadable.download.is_downloaded = true;
            std::cout << GREEN "\n\nSuccessfully installed: " RESET << downloadable.title << "\n\nTime elapsed: " << time(NULL) - seconds << " seconds\n";
            /*
            std::vector<std::pair<std::string, bool>> files;
            for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(TMP_EXTRACTED)) {
                std::string path = dirEntry.path().string();
                std::string new_path = path.replace(path.find(TMP_EXTRACTED), strlen(TMP_EXTRACTED)+1, SYSTEM_ROOT);
                files.push_back(std::make_pair(path, std::filesystem::exists(path)));
            }
            installed_json["Installed"][downloadable.title] = 
            {
                { "Files", files }
            };
            SaveJson();
            fsdevDeleteDirectoryRecursively(TMP_EXTRACTED);
            */
            break;
        }
        case gh::DownloadResult::CURL_ERROR:
            std::cout << RED "Curl error" RESET;
            break;
        case gh::DownloadResult::DOES_NOT_EXIST:
            std::cout << RED "Does not exist" RESET;
            break;
        case gh::DownloadResult::DOWNLOAD_FAILED:
            std::cout << RED "Download failed" RESET;
            break;
        case gh::DownloadResult::ACCESS_DENIED:
            std::cout << RED "Access denied" RESET;
            break;
        default:
            std::cout << RED "Unknown result." RESET;
            break;
    }

    std::cout << WHITE "\n\nPress B to exit.\n" RESET;
    consoleUpdate(NULL);
    u64 k;
    do {
        hidScanInput();
        k = hidKeysDown(CONTROLLER_P1_AUTO);
    } while (!(k & KEY_B));

}

void emptyFocus(TreeNode* node) {
    Empty& empty = *(Empty*)node->GetUserData();
    std::cout << GREEN "\n\n" << empty.title << "\n\n\n" RESET;
    std::cout << empty.message << std::endl;
}

void makeMenu(TreeNode* node, const std::string& title, const std::vector<std::string>& entries) {
    Menu* menu = new Menu;
    menu->magic = MENU_MAGIC;
    menu->title = title;
    menu->entries = entries;
    menu->selected = 0;
    node->SetUserData(menu);
    node->SetOnFocus(menuFocus);
    node->SetDestroyUserData(_destroyMenu);
}

void makeDownloadable(TreeNode* node, const std::string& title, const std::string& body, const GhDownload& download) {
    Downloadable* dl = new Downloadable;
    dl->magic = DOWNLOADABLE_MAGIC;
    dl->title = title;
    dl->body = body;
    dl->download = download;
    node->SetUserData(dl);
    node->SetOnFocus(downloadableFocus);
    node->SetDestroyUserData(_destroyDownloadable);
}

void makeEmpty(TreeNode* node, const std::string& title, const std::string& message) {
    Empty* empty = new Empty;
    empty->magic = EMPTY_MAGIC;
    empty->title = title;
    empty->message = message;
    node->SetUserData(empty);
    node->SetOnFocus(emptyFocus);
    node->SetDestroyUserData(_destroyEmpty);
}

void menuSelect(TreeNode* node, size_t selected) {
    Menu* menu = (Menu*)node->GetUserData();
    menu->selected = (selected % menu->entries.size());
}

size_t menuGetSelected(TreeNode* node) {
    Menu* menu = (Menu*)node->GetUserData();
    return menu->selected;
}

size_t menuGetEntryCount(TreeNode* node) {
    Menu* menu = (Menu*)node->GetUserData();
    return menu->entries.size();
}