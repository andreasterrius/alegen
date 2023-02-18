#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <tuple>
#include <filesystem>
#include <time.h>
#include <stdlib.h>

#include "timer.h"
#include "sprite_renderer.h"
#include "tetris.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

using namespace std;
using namespace glm;

const int windowWidth = 800;
const int windowHeight = 800;
const float moveTickTime = 0.08f;
const float moveTickFirstSticky = 0.2f;

std::filesystem::path getExeParentDirectory()
{
#ifdef _WIN32
    // Windows specific
    wchar_t szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, MAX_PATH);
#else
    // Linux specific
    char szPath[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", szPath, PATH_MAX);
    if (count < 0 || count >= PATH_MAX)
        return {}; // some error
    szPath[count] = '\0';
#endif
    return std::filesystem::path{szPath}.parent_path().parent_path() / ""; // to finish the folder path with (back)slash
}

static void printGLFWInfo(GLFWwindow *window)
{
    int profile = glfwGetWindowAttrib(window, GLFW_OPENGL_PROFILE);
    const char *profileStr = "";
    if (profile == GLFW_OPENGL_COMPAT_PROFILE)
        profileStr = "OpenGL Compatibility Profile";
    else if (profile == GLFW_OPENGL_CORE_PROFILE)
        profileStr = "OpenGL Core Profile";

    printf("GLFW %s %s\n", glfwGetVersionString(), profileStr);
}

static bool setupOpenGL()
{
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    printf("OpenGL %d.%d\n", GLVersion.major, GLVersion.minor);

#define C(x) (x ? (const char *)x : "")
    std::cout << "GL_VERSION..........: " << C(glGetString(GL_VERSION)) << '\n';
    std::cout << "GL_RENDERER.........: " << C(glGetString(GL_RENDERER)) << '\n';
    std::cout << "GL_VENDOR...........: " << C(glGetString(GL_VENDOR)) << '\n';
    std::cout << "GLSL_VERSION........: " << C(glGetString(GL_SHADING_LANGUAGE_VERSION)) << '\n';
    std::cout << "-----------------------\n";

    // setupStderrDebugCallback();

    // glEnable(GL_DEPTH_TEST);
    // glEnable(GL_BLEND);
    // glDepthFunc(GL_LEQUAL);
    // glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    return true;
}

GLFWwindow *createWindow()
{
    filesystem::current_path(getExeParentDirectory());

    glfwSetErrorCallback([](int error, const char *description)
                         { fprintf(stderr, "GLFW Error %d: %s\n", error, description); });
    if (!glfwInit())
        return nullptr;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    GLFWwindow *window = glfwCreateWindow((int)windowWidth, (int)windowHeight, "Main Window", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);
    printGLFWInfo(window);
    if (!setupOpenGL())
        return nullptr;

    return window;
}

class Input
{
public:
    Input() : moveLeftTimer(moveTickTime, moveTickFirstSticky), moveRightTimer(moveTickTime, moveTickFirstSticky)
    {
    }

    Timer moveLeftTimer;
    Timer moveRightTimer;
    Arena *arena;
    bool shouldMoveFaster;

    void timerTicks(float deltaTime)
    {
        moveLeftTimer.tick(deltaTime);
        moveRightTimer.tick(deltaTime);
    }
};

void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    Input *input = static_cast<Input *>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            input->moveLeftTimer.addExec(1);
            input->moveLeftTimer.start();
        }
        else if (action == GLFW_RELEASE)
        {
            input->moveLeftTimer.stop();
        }
    }
    else if (key == GLFW_KEY_RIGHT)
    {
        if (action == GLFW_PRESS)
        {
            input->moveRightTimer.addExec(1);
            input->moveRightTimer.start();
        }
        else if (action == GLFW_RELEASE)
        {
            input->moveRightTimer.stop();
        }
    }
    else if (key == GLFW_KEY_DOWN)
    {
        if (action == GLFW_PRESS)
        {
            input->shouldMoveFaster = true;
        }
        else if (action == GLFW_RELEASE)
        {
            input->shouldMoveFaster = false;
        }
    }
    else if (key == GLFW_KEY_SPACE)
    {
        if (action == GLFW_PRESS)
        {
            input->arena->rotate();
        }
    }
}

int main()
{
    srand(time(NULL));

    GLFWwindow *window = createWindow();
    if (window == nullptr)
        return -1;
    SpriteRenderer spriteRenderer;
    Arena arena(vec2(100, 0), 300);
    arena.moveDown(); // force to spawn

    Input input;
    input.arena = &arena;
    glfwSetWindowUserPointer(window, &input);
    glfwSetKeyCallback(window, keyCallback);

    mat4 ortho = glm::ortho(0.0f, (float)windowWidth, (float)windowHeight, 0.0f, 0.1f, 100.0f);
    mat4 view = mat4(1.0);
    view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));

    float moveDownMin = 0.04f;
    float moveDownMax = 0.5f;
    float moveDownThershold = moveDownMax; // 1 second then tick down
    float moveDownTime = 0.0f;
    float lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window))
    {
        float currTime = glfwGetTime();
        float deltaTime = currTime - lastTime;
        lastTime = currTime;

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        input.timerTicks(deltaTime);
        if (input.shouldMoveFaster)
            moveDownThershold = moveDownMin;
        else
            moveDownThershold = moveDownMax;

        auto previewSprites = arena.renderPreview();
        spriteRenderer.render(previewSprites, view, ortho);
        auto arenaSprites = arena.render();
        spriteRenderer.render(arenaSprites, view, ortho);
        auto arenaBoundarySprites = arena.renderBoundary();
        spriteRenderer.render(arenaBoundarySprites, view, ortho);

        moveDownTime += deltaTime;
        if (moveDownTime >= moveDownThershold)
        {
            moveDownTime = 0;
            arena.moveDown();
        }
        for (int i = 0; i < input.moveLeftTimer.consumeExec(); ++i)
        {
            arena.moveHorizontal(true, false);
        }
        for (int i = 0; i < input.moveRightTimer.consumeExec(); ++i)
        {
            arena.moveHorizontal(false, true);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}