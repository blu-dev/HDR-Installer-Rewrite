#pragma once
#include <string>
#include <vector>

class TreeNode {
    public:
        typedef void (*OnFocus)(TreeNode* focus);
        typedef void (*OnDestroy)(void* userData);
    private:
        TreeNode* m_pParent;
        std::vector<TreeNode*> m_Children;
        void* m_pUserData;
        OnFocus m_FocusCallback;
        OnDestroy m_DestroyCallback;
    protected:
        void HalfRemoveChild(TreeNode* child) {
            for (size_t i = 0; i < m_Children.size(); i++) {
                if (child == m_Children[i]) {
                    child->m_pParent = nullptr;
                    m_Children.erase(m_Children.begin() + i);
                    break;
                }
            }
        }
    public:

        TreeNode(void* userData = nullptr) : m_pParent(nullptr), m_Children(), m_pUserData(userData), m_FocusCallback(nullptr) { }
        TreeNode(TreeNode* parent, void* userData = nullptr) : m_pParent(parent), m_Children(), m_pUserData(userData), m_FocusCallback(nullptr) {}
        ~TreeNode() {
            if (m_pParent != nullptr)
                m_pParent->HalfRemoveChild(this);
            for (TreeNode* child : m_Children) {
                child->m_pParent = nullptr;
                delete child;
            }
            if (m_DestroyCallback != nullptr)
                m_DestroyCallback(m_pUserData);
            m_pUserData = nullptr;
        }

        constexpr void SetUserData(void* userData) { m_pUserData = userData; }
        constexpr void* GetUserData() { return m_pUserData; }

        constexpr void SetDestroyUserData(OnDestroy destroyer) { m_DestroyCallback = destroyer; }
        
        TreeNode* SpawnChild(void* userData = nullptr) {
            TreeNode* ret = new TreeNode(this, userData);
            m_Children.push_back(ret);
            return ret;
        }
        void RemoveChild(TreeNode* child) {
            for (size_t i = 0; i < m_Children.size(); i++)
                if (child == m_Children[i]) {
                    child->m_pParent = nullptr;
                    m_Children.erase(m_Children.begin() + i);
                    delete child;
                    break;
                }
        }
        void RemoveChild(size_t child) { 
            if (child >= GetChildCount())
                return;
            m_Children.erase(m_Children.begin() + child);
        }

        size_t GetChildCount() { return m_Children.size(); }
        TreeNode* GetChild(size_t child) { 
            if (child >= GetChildCount())
                return nullptr;
            return m_Children[child];
        }
        constexpr TreeNode* GetParent() { return m_pParent; }

        constexpr void SetOnFocus(OnFocus focus) { m_FocusCallback = focus; }
        void Focus() { if (m_FocusCallback != nullptr) m_FocusCallback(this); }
};

// Not necessary but it is a helper class
class NodeViewer {
    private:
        TreeNode* m_pCurrentNode;
    public:
        NodeViewer(TreeNode* start) : m_pCurrentNode(start) {}
        bool ShiftFocus(TreeNode* newFocus) {
            if (newFocus == nullptr)
                return false;
            TreeNode* parent = m_pCurrentNode->GetParent();
            if (newFocus == parent)
                m_pCurrentNode = newFocus;
            else if (m_pCurrentNode->GetChildCount() > 0)
                for (size_t i = 0; i < m_pCurrentNode->GetChildCount(); i++)
                    if (newFocus == m_pCurrentNode->GetChild(i))
                        m_pCurrentNode = newFocus;
            return newFocus == m_pCurrentNode;
        }
        bool ShiftFocus(int newFocus) {
            if (newFocus == -1)
                return ShiftFocus(m_pCurrentNode->GetParent());
            else if (newFocus < m_pCurrentNode->GetChildCount())
                return ShiftFocus(m_pCurrentNode->GetChild((size_t)newFocus));
            return false;
        }
        constexpr TreeNode* GetCurrent() { return m_pCurrentNode; }
        void Focus() {
            m_pCurrentNode->Focus();
        }
};