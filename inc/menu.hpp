#pragma once
#include "utils.hpp"
#include "tree_node.hpp"

#define MENU_MAGIC          0x1234
#define DOWNLOADABLE_MAGIC  0xDEAD
#define EMPTY_MAGIC         0xBEEF

struct GhDownload {
    gh::OauthToken token;
    std::string repository;
    std::string tag;
    //bool is_downloaded;
};

enum class NodeType {
    UNKNOWN,
    MENU,
    DOWNLOADABLE,
    EMPTY
};

NodeType checkType(TreeNode* node);

void makeMenu(TreeNode* node, const std::string& title, const std::vector<std::string>& entries);
void makeDownloadable(TreeNode* node, const std::string& title, const std::string& body, const GhDownload& download);
void makeEmpty(TreeNode* node, const std::string& title, const std::string& message);

void menuSelect(TreeNode* node, size_t selected);
size_t menuGetSelected(TreeNode* node);
size_t menuGetEntryCount(TreeNode* node);