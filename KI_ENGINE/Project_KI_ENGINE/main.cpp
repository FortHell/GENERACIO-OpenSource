#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_OPENGL

#include <Windows.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <vector>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

std::string loadShader(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader: " << path << std::endl;
        return "";
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint makeShader(const char* vs, const char* fs) {
    GLuint v = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v, 1, &vs, nullptr);
    glCompileShader(v);

    GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f, 1, &fs, nullptr);
    glCompileShader(f);

    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);

    glDeleteShader(v);
    glDeleteShader(f);
    return p;
}

GLuint loadShaderProgram(const std::string& vertPath, const std::string& fragPath) {
    std::string vertSrc = loadShader(vertPath);
    std::string fragSrc = loadShader(fragPath);
    return makeShader(vertSrc.c_str(), fragSrc.c_str());
}

static const float nearZ = 0.1f;
static const float farZ = 50.f;

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

std::vector<Vertex> loadOBJ(const char* path) {
    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec3> temp_normals;
    std::vector<Vertex> out_vertices;

    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "v") {
            glm::vec3 v;
            ss >> v.x >> v.y >> v.z;
            temp_vertices.push_back(v);
        }
        else if (type == "vn") {
            glm::vec3 n;
            ss >> n.x >> n.y >> n.z;
            temp_normals.push_back(n);
        }
        else if (type == "f") {
            std::vector<std::pair<int, int>> faceVerts; // {vIdx, nIdx}

            std::string vertexData;
            while (ss >> vertexData) {
                int vIdx = 0, tIdx = 0, nIdx = 0;

                if (sscanf_s(vertexData.c_str(), "%d/%d/%d", &vIdx, &tIdx, &nIdx) == 3) {
                    // v/vt/vn
                }
                else if (sscanf_s(vertexData.c_str(), "%d//%d", &vIdx, &nIdx) == 2) {
                    // v//vn
                }
                else if (sscanf_s(vertexData.c_str(), "%d/%d", &vIdx, &tIdx) == 2) {
                    // v/vt (no normal)
                    nIdx = 0;
                }
                else {
                    sscanf_s(vertexData.c_str(), "%d", &vIdx);
                    // v only
                }

                faceVerts.push_back({ vIdx, nIdx });
            }

            // Fan triangulation for quads and ngons
            for (size_t i = 1; i + 1 < faceVerts.size(); i++) {
                auto emit = [&](std::pair<int, int> vi) {
                    glm::vec3 pos = temp_vertices[vi.first - 1];
                    glm::vec3 nrm = vi.second > 0
                        ? temp_normals[vi.second - 1]
                        : glm::vec3(0, 1, 0);
                    out_vertices.push_back({ pos, nrm });
                    };
                emit(faceVerts[0]);
                emit(faceVerts[i]);
                emit(faceVerts[i + 1]);
            }
        }
    }
    return out_vertices;
}

static const float floorVerts[] = {
    // Position            // Normal (Straight Up)
    -5.f, -0.1f, -5.f,     0.0f, 1.0f, 0.0f,
     5.f, -0.1f, -5.f,     0.0f, 1.0f, 0.0f,
     5.f, -0.1f,  5.f,     0.0f, 1.0f, 0.0f,
    -5.f, -0.1f,  5.f,     0.0f, 1.0f, 0.0f
};

static const uint16_t floorIdx[] = { 0, 1, 2, 2, 3, 0 };

// XR projection matrix
glm::mat4 xrProj(const XrFovf& f) {
    float tanLeft = tanf(f.angleLeft);
    float tanRight = tanf(f.angleRight);
    float tanUp = tanf(f.angleUp);
    float tanDown = tanf(f.angleDown);
    const float nearZ = 0.1f;
    const float farZ = 50.0f;
    const float tanWidth = tanRight - tanLeft;
    const float tanHeight = tanUp - tanDown;

    glm::mat4 proj(0.0f);
    proj[0][0] = 2.0f / tanWidth;
    proj[1][1] = 2.0f / tanHeight;
    proj[2][0] = (tanRight + tanLeft) / tanWidth;
    proj[2][1] = (tanUp + tanDown) / tanHeight;
    proj[2][2] = -(farZ + nearZ) / (farZ - nearZ);
    proj[2][3] = -1.0f;
    proj[3][2] = -(2.0f * farZ * nearZ) / (farZ - nearZ);
    return proj;
}

XrActionSet actionSet;
XrAction handPoseAction;

XrSpace leftHandSpace = XR_NULL_HANDLE;
XrSpace rightHandSpace = XR_NULL_HANDLE;

XrPath handSubactionPaths[2];

int main() {
    // GL window
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    // Temporary size
    GLFWwindow* win = glfwCreateWindow(100, 100, "KI ENGINE VR View", nullptr, nullptr);
    glfwMakeContextCurrent(win);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_CULL_FACE);
    glfwSwapInterval(0);

    // Shaders
    GLuint shp = loadShaderProgram("vertex.glsl", "fragment.glsl");
    GLuint mvpLoc = glGetUniformLocation(shp, "mvp");
    GLuint colLoc = glGetUniformLocation(shp, "objColor");

    GLuint mirror = loadShaderProgram("vertex_mirror.glsl", "fragment_mirror.glsl");
    GLuint mirrorVAO;
    glGenVertexArrays(1, &mirrorVAO);

    GLint success;
    glGetProgramiv(shp, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shp, 512, NULL, infoLog);
        std::cout << "SHADER LINK ERROR: " << infoLog << std::endl;
    }

    // Cube Mesh
    std::vector<Vertex> meshData = loadOBJ("cube.obj");

    std::cout << "Loaded OBJ: " << meshData.size() << " vertices." << std::endl;

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, meshData.size() * sizeof(Vertex), meshData.data(), GL_STATIC_DRAW);

    // Position (Location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    // Normal (Location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    // Floor Mesh
    GLuint fvao, fvbo, febo;
    glGenVertexArrays(1, &fvao);
    glBindVertexArray(fvao);

    glGenBuffers(1, &fvbo);
    glBindBuffer(GL_ARRAY_BUFFER, fvbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVerts), floorVerts, GL_STATIC_DRAW);

    // Position Attribute (Location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal Attribute (Location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &febo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, febo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(floorIdx), floorIdx, GL_STATIC_DRAW);

    // XR Instance
    XrInstance inst;
    XrInstanceCreateInfo ici{ XR_TYPE_INSTANCE_CREATE_INFO };
    strcpy_s(ici.applicationInfo.applicationName, "KI ENGINE");
	ici.applicationInfo.apiVersion = XR_API_VERSION_1_0; // IMPORTANT! SteamVR only supports 1.0
    const char* ext[] = { XR_KHR_OPENGL_ENABLE_EXTENSION_NAME };
    ici.enabledExtensionCount = 1;
    ici.enabledExtensionNames = ext;
    xrCreateInstance(&ici, &inst);
    std::cout << "XR instance OK\n";

    // --- Action Set ---
    XrActionSetCreateInfo asci{ XR_TYPE_ACTION_SET_CREATE_INFO };
    strcpy_s(asci.actionSetName, "gameplay");
    strcpy_s(asci.localizedActionSetName, "Gameplay");
    asci.priority = 0;
    xrCreateActionSet(inst, &asci, &actionSet);

    // --- Hand Pose Action ---
    xrStringToPath(inst, "/user/hand/left", &handSubactionPaths[0]);
    xrStringToPath(inst, "/user/hand/right", &handSubactionPaths[1]);

    XrActionCreateInfo aci{ XR_TYPE_ACTION_CREATE_INFO };
    aci.actionType = XR_ACTION_TYPE_POSE_INPUT;
    strcpy_s(aci.actionName, "hand_pose");
    strcpy_s(aci.localizedActionName, "Hand Pose");
    aci.countSubactionPaths = 2;
    aci.subactionPaths = handSubactionPaths;

    xrCreateAction(actionSet, &aci, &handPoseAction);

    // XR system
    XrSystemId sysId;
    XrSystemGetInfo sgi{ XR_TYPE_SYSTEM_GET_INFO };
    sgi.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    xrGetSystem(inst, &sgi, &sysId);

    XrPath profile;
    xrStringToPath(inst,
        "/interaction_profiles/valve/index_controller",
        &profile);

    XrPath leftGripPose, rightGripPose;
    xrStringToPath(inst,
        "/user/hand/left/input/grip/pose",
        &leftGripPose);
    xrStringToPath(inst,
        "/user/hand/right/input/grip/pose",
        &rightGripPose);

    XrActionSuggestedBinding bindings[] = {
        { handPoseAction, leftGripPose },
        { handPoseAction, rightGripPose }
    };

    XrInteractionProfileSuggestedBinding suggested{
        XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING
    };
    suggested.interactionProfile = profile;
    suggested.suggestedBindings = bindings;
    suggested.countSuggestedBindings = 2;

    xrSuggestInteractionProfileBindings(inst, &suggested);

    // Required GL version query
    PFN_xrGetOpenGLGraphicsRequirementsKHR reqFn;
    xrGetInstanceProcAddr(inst,
        "xrGetOpenGLGraphicsRequirementsKHR",
        (PFN_xrVoidFunction*)&reqFn);
    XrGraphicsRequirementsOpenGLKHR req{ XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
    reqFn(inst, sysId, &req);

    // Bind current GL context
    XrGraphicsBindingOpenGLWin32KHR gb{ XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR };
    gb.hDC = wglGetCurrentDC();
    gb.hGLRC = wglGetCurrentContext();

    // Session
    XrSession sess;
    XrSessionCreateInfo sci{ XR_TYPE_SESSION_CREATE_INFO };
    sci.next = &gb;
    sci.systemId = sysId;
    xrCreateSession(inst, &sci, &sess);
    std::cout << "XR session OK\n";

    XrSessionActionSetsAttachInfo attach{
    XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO
    };
    attach.countActionSets = 1;
    attach.actionSets = &actionSet;
    xrAttachSessionActionSets(sess, &attach);

    // Reference space
    XrSpace space;
    XrReferenceSpaceCreateInfo rci{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
    rci.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    rci.poseInReferenceSpace.position = { 0, 0, 0 };
    rci.poseInReferenceSpace.orientation = { 0,0,0,1 };
    xrCreateReferenceSpace(sess, &rci, &space);

    XrActionSpaceCreateInfo asci2{ XR_TYPE_ACTION_SPACE_CREATE_INFO };
    asci2.action = handPoseAction;
    asci2.poseInActionSpace.orientation = { 0,0,0,1 };

    asci2.subactionPath = handSubactionPaths[0];
    xrCreateActionSpace(sess, &asci2, &leftHandSpace);

    asci2.subactionPath = handSubactionPaths[1];
    xrCreateActionSpace(sess, &asci2, &rightHandSpace);

    // Begin
    XrSessionBeginInfo bi{ XR_TYPE_SESSION_BEGIN_INFO };
    bi.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    xrBeginSession(sess, &bi);

    // Views config
    uint32_t viewCount;
    xrEnumerateViewConfigurationViews(inst, sysId,
        XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
        0, &viewCount, nullptr);

    std::vector<XrViewConfigurationView> vcfg(
        viewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
    xrEnumerateViewConfigurationViews(inst, sysId,
        XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
        viewCount, &viewCount, vcfg.data());

    int W = vcfg[0].recommendedImageRectWidth;
    int H = vcfg[0].recommendedImageRectHeight;

    const GLFWvidmode* vidmode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    // Bearable size for monitors
	float aspectRatio = (float)W / (float)H;
    int mirrorH = (int)std::floor(vidmode->height * 0.75f);
    int mirrorW = (int)std::floor(mirrorH * aspectRatio);

    // Helpful info
    std::cout << "\nPER EYE PROPERTIES\nDecimal Aspect Ratio: " << aspectRatio << "\n";
    std::cout << "Simplied Aspect Ratio: " << aspectRatio * 9 << ":9" << "\n";
	std::cout << "VR Resolution: " << W << "x" << H << "\n";
    std::cout << "Window Resolution: " << mirrorW << "x" << mirrorH << "\n\n";

    // Resize to actual resolution
    glfwSetWindowSize(win, mirrorW, mirrorH);

    // 1 swapchain, 2 array layers
    XrSwapchain sc;
    XrSwapchainCreateInfo sci2{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
    sci2.arraySize = 2;
    sci2.format = GL_SRGB8_ALPHA8;
    sci2.width = W; sci2.height = H;
    sci2.mipCount = 1;
    sci2.faceCount = 1;
    sci2.sampleCount = 1;
    sci2.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT |
        XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
    xrCreateSwapchain(sess, &sci2, &sc);

    uint32_t scImgCount;
    xrEnumerateSwapchainImages(sc, 0, &scImgCount, nullptr);
    std::vector<XrSwapchainImageOpenGLKHR> scImgs(
        scImgCount, { XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR });
    xrEnumerateSwapchainImages(sc, scImgCount, &scImgCount,
        (XrSwapchainImageBaseHeader*)scImgs.data());

    // FBO + depth
    GLuint fbo, depth;
    glGenFramebuffers(1, &fbo);
    glGenRenderbuffers(1, &depth);
    glBindRenderbuffer(GL_RENDERBUFFER, depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, W, H);

	// Multisampled FBO (4x MSAA)
    GLuint msaaFBO, msaaColor, msaaDepth;
    glGenFramebuffers(1, &msaaFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, msaaFBO);

    // Color Buffer (4x Samples)
    glGenRenderbuffers(1, &msaaColor);
    glBindRenderbuffer(GL_RENDERBUFFER, msaaColor);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_SRGB8_ALPHA8, W, H);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaaColor);

    // Depth Buffer (4x Samples)
    glGenRenderbuffers(1, &msaaDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, msaaDepth);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, W, H);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, msaaDepth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "MSAA FBO Failed!\n";

    // View + Layer views
    std::vector<XrView> views(viewCount, { XR_TYPE_VIEW });
    std::vector<XrCompositionLayerProjectionView> lviews(
        viewCount, { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW });

    // main rendering loop
    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();

        // Start frame
        XrFrameWaitInfo fwi{ XR_TYPE_FRAME_WAIT_INFO };
        XrFrameState fs{ XR_TYPE_FRAME_STATE };
        xrWaitFrame(sess, &fwi, &fs);
        XrFrameBeginInfo fbi{ XR_TYPE_FRAME_BEGIN_INFO };
        xrBeginFrame(sess, &fbi);

        XrActiveActionSet activeSet{ actionSet, XR_NULL_PATH };
        XrActionsSyncInfo sync{ XR_TYPE_ACTIONS_SYNC_INFO };
        sync.countActiveActionSets = 1;
        sync.activeActionSets = &activeSet;
        xrSyncActions(sess, &sync);

        // Locate views
        uint32_t outCount;
        XrViewLocateInfo vli{ XR_TYPE_VIEW_LOCATE_INFO };
        vli.space = space;
        vli.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        vli.displayTime = fs.predictedDisplayTime;
        XrViewState vs{ XR_TYPE_VIEW_STATE };
        xrLocateViews(sess, &vli, &vs, viewCount, &outCount, views.data());

        XrSpaceLocation leftHandLoc{ XR_TYPE_SPACE_LOCATION };
        XrSpaceLocation rightHandLoc{ XR_TYPE_SPACE_LOCATION };

        xrLocateSpace(leftHandSpace, space,
            fs.predictedDisplayTime, &leftHandLoc);
        xrLocateSpace(rightHandSpace, space,
            fs.predictedDisplayTime, &rightHandLoc);

        bool leftValid =
            (leftHandLoc.locationFlags &
                XR_SPACE_LOCATION_POSITION_VALID_BIT) &&
            (leftHandLoc.locationFlags &
                XR_SPACE_LOCATION_ORIENTATION_VALID_BIT);

        bool rightValid =
            (rightHandLoc.locationFlags &
                XR_SPACE_LOCATION_POSITION_VALID_BIT) &&
            (rightHandLoc.locationFlags &
                XR_SPACE_LOCATION_ORIENTATION_VALID_BIT);

        GLuint sunDirLoc = glGetUniformLocation(shp, "sunDir");
        GLuint modelLoc = glGetUniformLocation(shp, "model");

        // Render eyes and also all objects in scene
        for (uint32_t eye = 0;eye < outCount;eye++) {
            uint32_t idx;
            XrSwapchainImageAcquireInfo ac{ XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
            xrAcquireSwapchainImage(sc, &ac, &idx);
            XrSwapchainImageWaitInfo wi{ XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
            wi.timeout = XR_INFINITE_DURATION;
            xrWaitSwapchainImage(sc, &wi);

            glBindFramebuffer(GL_FRAMEBUFFER, msaaFBO);
            glViewport(0, 0, W, H);
            glClearColor(0.02f, 0.02f, 0.03f, 1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glm::quat q(views[eye].pose.orientation.w,
                views[eye].pose.orientation.x,
                views[eye].pose.orientation.y,
                views[eye].pose.orientation.z);
            glm::vec3 p(views[eye].pose.position.x,
                views[eye].pose.position.y,
                views[eye].pose.position.z);

            glm::mat4 V = glm::inverse(
                glm::translate(glm::mat4(1), p) * glm::mat4_cast(q));
            glm::mat4 P = xrProj(views[eye].fov);

            glUseProgram(shp);
            glUniform3f(glGetUniformLocation(shp, "sunColor"), 1.0f, 0.95f, 0.8f);

            // Draw floor
            glm::mat4 floorM = glm::translate(glm::mat4(1), glm::vec3(0, 0.01f, 0));
            glm::mat4 floorMVP = P * V * floorM;

            glUniform3f(colLoc, 0.1f, 0.1f, 0.1f);
            glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(floorMVP));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(floorM));
            glUniform3f(sunDirLoc, -0.5f, -1.0f, -0.3f);

            glBindVertexArray(fvao);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

            auto drawHand = [&](const XrSpaceLocation& loc, glm::vec3 color)
                {
                    glm::quat q(
                        loc.pose.orientation.w,
                        loc.pose.orientation.x,
                        loc.pose.orientation.y,
                        loc.pose.orientation.z
                    );

                    glm::vec3 p(
                        loc.pose.position.x,
                        loc.pose.position.y,
                        loc.pose.position.z
                    );

                    glm::mat4 M = glm::translate(glm::mat4(1), p) * glm::mat4_cast(q);
                    M = glm::scale(M, glm::vec3(0.04f, 0.08f, 0.18f));
                    glm::mat4 MVP = P * V * M;

                    glUniform3fv(colLoc, 1, &color.x);
                    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(MVP));

                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(M));

                    glUniform3f(sunDirLoc, -0.5f, -1.0f, -0.3f);

                    glBindVertexArray(vao);
                    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)meshData.size());
                };

            if (leftValid)
                drawHand(leftHandLoc, { 0.2f, 0.9f, 0.2f });

            if (rightValid)
                drawHand(rightHandLoc, { 0.9f, 0.2f, 0.2f });

			// Draw cube 1
            glm::mat4 M = glm::translate(glm::mat4(1), glm::vec3(0.4f, 1.45f, -1.3f));
            M = glm::scale(M, glm::vec3(0.2f));
            M = glm::rotate(M, (float)glfwGetTime() * 1.15f, glm::vec3(0.3, 1, 0.5));
            glm::mat4 MVP = P * V * M;

            glUniform3f(colLoc, 0.1f, 0.7f, 1.0f);
            glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(MVP));
            glUniform3f(sunDirLoc, -0.5f, -1.0f, -0.3f);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(M));
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)meshData.size());

            // Draw cube 2
            M = glm::translate(glm::mat4(1), glm::vec3(0, 1.75f, -0.7f));
            M = glm::scale(M, glm::vec3(0.2f));
            M = glm::rotate(M, (float)glfwGetTime() * 1.15f, glm::vec3(0.3, 1, 0.5));
            MVP = P * V * M;

            glUniform3f(colLoc, 0.1f, 0.7f, 1.0f);
            glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(MVP));
            glUniform3f(sunDirLoc, -0.5f, -1.0f, -0.3f);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(M));
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)meshData.size());

            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                scImgs[idx].image, 0, eye);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);

            glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaFBO);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
            glBlitFramebuffer(
                0, 0, W, H,
                0, 0, W, H,
                GL_COLOR_BUFFER_BIT,
                GL_NEAREST
            );

            XrSwapchainImageReleaseInfo r{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
            xrReleaseSwapchainImage(sc, &r);

            lviews[eye].pose = views[eye].pose;
            lviews[eye].fov = views[eye].fov;
            lviews[eye].subImage.swapchain = sc;
            lviews[eye].subImage.imageRect.offset = { 0,0 };
            lviews[eye].subImage.imageRect.extent = { W,H };
            lviews[eye].subImage.imageArrayIndex = eye;
        }

        // Submit
        XrCompositionLayerProjection layer{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };
        layer.space = space;
        layer.viewCount = outCount;
        layer.views = lviews.data();
        const XrCompositionLayerBaseHeader* arr[] =
        { (const XrCompositionLayerBaseHeader*)&layer };

        XrFrameEndInfo fei{ XR_TYPE_FRAME_END_INFO };
        fei.displayTime = fs.predictedDisplayTime;
        fei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        fei.layerCount = 1;
        fei.layers = arr;
        xrEndFrame(sess, &fei);

        // Mirror left eye
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, mirrorW, mirrorH);
        glDisable(GL_DEPTH_TEST);
        glUseProgram(mirror);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, scImgs[0].image);
        glUniform1i(glGetUniformLocation(mirror, "texArr"), 0);
        glBindVertexArray(mirrorVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(win);
    }

    glfwTerminate();
    return 0;
}