#include "simpleloader.h"

#include <iostream>
#include <string>

namespace simpleloader {
    namespace {
        Handler g_loadHandler;
        SceneHandler g_sceneHandler;
    }

    void setLoadHandler(Handler h)  { g_loadHandler  = std::move(h); }
    void setSceneHandler(SceneHandler h) { g_sceneHandler = std::move(h); }

    void checkIfLoaded() {
        if (g_loadHandler) g_loadHandler(0);
    }

    // Configuring the scene
    void sceneConfig() {
        // Number of planets
        int planetCount = 6;
        // Name of .obj that the planets will use (file must be next to executable)
        std::string objName = "sphere.obj";

        if (g_sceneHandler) g_sceneHandler(planetCount, objName);
    }
}