#pragma once
#include <string>

class Input
{
public:
    static Input& Get()
    {
        static Input instance;
        return instance;
    }

    bool GetKeyDown(const std::string& key_name) { return false; }
    void SetKeyDown(const std::string& key_name) {}
};
