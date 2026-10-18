#pragma once

#include <functional>
#include <string>

namespace simpleloader {
    using Handler = std::function<void(int)>;
    using SceneHandler = std::function<void(int, const std::string&)>;

    void setLoadHandler(Handler h);
    void setSceneHandler(SceneHandler h);
    
    void checkIfLoaded();
    void sceneConfig();
}