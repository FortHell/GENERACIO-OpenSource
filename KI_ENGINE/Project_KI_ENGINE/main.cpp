#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_OPENGL

#include <Windows.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <iostream>
#include <vector>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

static const float nearZ = 0.1f;
static const float farZ = 50.f;

// cube vertex + index data
static const float cubeVerts[] = {
    -0.5f,-0.5f,-0.5f,
     0.5f,-0.5f,-0.5f,
     0.5f, 0.5f,-0.5f,
    -0.5f, 0.5f,-0.5f,
    -0.5f,-0.5f, 0.5f,
     0.5f,-0.5f, 0.5f,
     0.5f, 0.5f, 0.5f,
    -0.5f, 0.5f, 0.5f
};
static const uint16_t cubeIdx[] = {
    0,1,2, 2,3,0, // back
    4,5,6, 6,7,4, // front
    3,2,6, 6,7,3, // top
    0,1,5, 5,4,0, // bottom
    1,2,6, 6,5,1, // right
    0,3,7, 7,4,0  // left
};

// very simple vertex+frag shader
static const char* VERT = R"(
#version 460 core
layout(location=0) in vec3 pos;
uniform mat4 mvp;
void main() { gl_Position = mvp * vec4(pos,1.0); }
)";
static const char* FRAG = R"(
#version 460 core
out vec4 col;
void main() { col = vec4(0.1,0.7,1.0,1.0); }
)";

// fullscreen blit shader
static const char* MIRROR_VS = R"(
#version 460 core
const vec2 v[3] = vec2[]( vec2(-1,-1), vec2(3,-1), vec2(-1,3));
out vec2 uv;
void main(){
    uv = (v[gl_VertexID] + 1.0) * 0.5;
    gl_Position = vec4(v[gl_VertexID],0,1);
})";
static const char* MIRROR_FS = R"(
#version 460 core
in vec2 uv;
out vec4 col;
uniform sampler2DArray texArr;
void main(){ col = texture(texArr, vec3(uv,0)); }
)";

// make GL shader
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

int main() {
    // FPS counter init
    using clock = std::chrono::steady_clock;
    int frames = 0;
    auto lastTime = clock::now();

    // GL window
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* win = glfwCreateWindow(400, 200, "KI ENGINE VR View", nullptr, nullptr); // Needs fixing, using SteamVR's VR View for now.
    glfwMakeContextCurrent(win);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glEnable(GL_DEPTH_TEST);
    /*glEnable(GL_CULL_FACE); // Will fix later. Or not.
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);*/
    glfwSwapInterval(0);

    // Shaders
    GLuint shp = makeShader(VERT, FRAG);
    GLuint mvpLoc = glGetUniformLocation(shp, "mvp");

    GLuint mirror = makeShader(MIRROR_VS, MIRROR_FS);
    GLuint mirrorVAO; glGenVertexArrays(1, &mirrorVAO);

    // Cube Mesh
    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIdx), cubeIdx, GL_STATIC_DRAW);

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

    // XR system
    XrSystemId sysId;
    XrSystemGetInfo sgi{ XR_TYPE_SYSTEM_GET_INFO };
    sgi.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    xrGetSystem(inst, &sgi, &sysId);

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

    // Reference space
    XrSpace space;
    XrReferenceSpaceCreateInfo rci{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
    rci.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    rci.poseInReferenceSpace.position = { 0, 0, 0 };
    rci.poseInReferenceSpace.orientation = { 0,0,0,1 };
    xrCreateReferenceSpace(sess, &rci, &space);

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

        // Locate views
        uint32_t outCount;
        XrViewLocateInfo vli{ XR_TYPE_VIEW_LOCATE_INFO };
        vli.space = space;
        vli.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        vli.displayTime = fs.predictedDisplayTime;
        XrViewState vs{ XR_TYPE_VIEW_STATE };
        xrLocateViews(sess, &vli, &vs, viewCount, &outCount, views.data());

        // Render eyes
        for (uint32_t eye = 0;eye < outCount;eye++) {
            uint32_t idx;
            XrSwapchainImageAcquireInfo ac{ XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
            xrAcquireSwapchainImage(sc, &ac, &idx);
            XrSwapchainImageWaitInfo wi{ XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
            wi.timeout = XR_INFINITE_DURATION;
            xrWaitSwapchainImage(sc, &wi);

            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                scImgs[idx].image, 0, eye);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);

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

            glm::mat4 M = glm::translate(glm::mat4(1), glm::vec3(0, 0, -2.0f));
            M = glm::scale(M, glm::vec3(0.2f));
            M = glm::rotate(M, (float)glfwGetTime(), glm::vec3(0.3, 1, 0.5));
            glm::mat4 MVP = P * V * M;

            glUseProgram(shp);
            glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(MVP));
            glBindVertexArray(vao);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);

            XrSwapchainImageReleaseInfo r{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
            xrReleaseSwapchainImage(sc, &r);

            lviews[eye].pose = views[eye].pose;
            lviews[eye].fov = views[eye].fov;
            lviews[eye].subImage.swapchain = sc;
            lviews[eye].subImage.imageRect.offset = { 0,0 };
            lviews[eye].subImage.imageRect.extent = { W,H };
            lviews[eye].subImage.imageArrayIndex = eye;
        }

        //printf("Eye 0: %g\n", views[0].pose.position.x); // Only for debugging!
        //printf("Eye 1: %g\n", views[1].pose.position.x);

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
        glViewport(0, 0, 1920, 1080);
        glDisable(GL_DEPTH_TEST);
        glUseProgram(mirror);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, scImgs[0].image);
        glUniform1i(glGetUniformLocation(mirror, "texArr"), 0);
        glBindVertexArray(mirrorVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(win);

		// FPS counter. Don't need it? Comment it out.
        // p.s. this is not the headset refresh rate!
        frames++;

        auto now = clock::now();
        std::chrono::duration<float> delta = now - lastTime;
        if (delta.count() >= 1.0f) {
            std::cout << "FPS: " << frames << "\n";
            frames = 0;
            lastTime = now;
        }
    }

    glfwTerminate();
    return 0;
}