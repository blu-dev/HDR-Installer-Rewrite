#include "menu.hpp"

NodeType checkType(TreeNode* node) {
    NodeType ret = NodeType::UNKNOWN;
    uint32_t* magic_ptr = (uint32_t*)node->GetUserData();
    if (magic_ptr != nullptr)
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

void menuFocus(TreeNode* node) {
    Menu& menu = *(Menu*)node->GetUserData();
    std::cout << GREEN "\n\n" << menu.title << "\n\n\n" RESET;
    size_t child_count = menu.entries.size();
    if (child_count > 0) {
        for (size_t i = 0; i < child_count; i++) {
            if (menu.selected == i)
                std::cout << CYAN;
            std::cout << menu.entries[i] << std::endl;
            if (menu.selected == i)
                std::cout << RESET;
        }
    }
}

struct Downloadable {
    uint32_t magic;
    std::string title;
    GhDownload download;
};

void _destroyDownloadable(void* dl) {
    delete (Downloadable*)dl;
}

void downloadableFocus(TreeNode* node) {

}

struct Empty {
    uint32_t magic;
    std::string title;
    std::string message;
};

void _destroyEmpty(void* empty) {
    delete (Empty*)empty;
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

void makeDownloadable(TreeNode* node, const std::string& title, const GhDownload& download) {
    Downloadable* dl = new Downloadable;
    dl->magic = DOWNLOADABLE_MAGIC;
    dl->title = title;
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