#pragma once
#include <string>
#include <unordered_set>

// Minimal keyboard state: the platform layer (MyRender.cpp) feeds SDL key
// down/up events in, and the camera/game code polls GetKey each frame. Key names
// are SDL key names ("W", "A", "Left", "Space", ...).
class Input
{
public:
    static Input& Get()
    {
        static Input instance;
        return instance;
    }

    void SetKeyDown(const std::string& key) { _down.insert(key); }
    void SetKeyUp(const std::string& key)   { _down.erase(key); }
    bool GetKey(const std::string& key) const { return _down.count(key) != 0; }

private:
    std::unordered_set<std::string> _down;
};
