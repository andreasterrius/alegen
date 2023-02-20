#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <enet/enet.h>
#include <argparse/argparse.hpp>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <tuple>
#include <filesystem>
#include <time.h>
#include <stdlib.h>
#include <thread>
#include <mutex>
#include <queue>

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

const int serverTickMs = 1000;
const int windowWidth = 800;
const int windowHeight = 800;
const float moveTickTime = 0.08f;
const float moveTickFirstSticky = 0.2f;

struct Args
{
    bool dedicatedServer;
};

bool handleArgs(Args *args, int argc, char *argv[])
{
    argparse::ArgumentParser program("tetris");

    program.add_argument("--server")
        .help("run as dedicated server")
        .default_value(false)
        .implicit_value(true);

    try
    {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error &err)
    {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return false;
    }

    args->dedicatedServer = program.get<bool>("server");
    return true;
}

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

    GLFWwindow *window = glfwCreateWindow((int)windowWidth, (int)windowHeight, "AleTetris", nullptr, nullptr);
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

class BlockChangeReplicator : public SelectedBlockChangeListener {
    void onChange(Block *b) override {
        cout << "on change" << endl;
        // should send message to server here
    }

    void onPlace(Block *b, unordered_set<int> *checkY) override {
        cout << "on place" << endl;
    }
};

class Input
{
public:
    Input() : moveLeftTimer(moveTickTime, moveTickFirstSticky), moveRightTimer(moveTickTime, moveTickFirstSticky)
    {
        moveDownMin = 0.04f;
        moveDownMax = 0.5f;
        moveDownThershold = moveDownMax; // 1 second then tick down
        moveDownTime = 0.0f;
    }

    Timer moveLeftTimer;
    Timer moveRightTimer;
    Arena *arena;
    bool shouldMoveFaster;

    float moveDownMin;
    float moveDownMax;
    float moveDownThershold;
    float moveDownTime;

    void timerTicks(float deltaTime)
    {
        moveLeftTimer.tick(deltaTime);
        moveRightTimer.tick(deltaTime);
    }

    void handleMoveDown(float deltaTime)
    {
        if (shouldMoveFaster)
            moveDownThershold = moveDownMin;
        else
            moveDownThershold = moveDownMax;
        moveDownTime += deltaTime;

        if (moveDownTime >= moveDownThershold)
        {
            moveDownTime = 0;
            arena->moveDown();
        }
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

class Tetris
{
public:
    GLFWwindow *window;
    SpriteRenderer spriteRenderer;
    Arena arena;
    Input input;

    mat4 ortho;
    mat4 view;

    Tetris() : arena(vec2(100, 0), 300), window(createWindow()), spriteRenderer()
    {
        if (window == nullptr)
        {
            cout << "window creation failed" << endl;
            return;
        }

        arena.moveDown(); // force to spawn
        input.arena = &arena;

        glfwSetWindowUserPointer(window, &input);
        glfwSetKeyCallback(window, keyCallback);

        ortho = glm::ortho(0.0f, (float)windowWidth, (float)windowHeight, 0.0f, 0.1f, 100.0f);
        view = mat4(1.0);
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));

        arena.sbcl = new BlockChangeReplicator();
    }

    ~Tetris()
    {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void run()
    {
        float lastTime = glfwGetTime();
        while (!glfwWindowShouldClose(window))
        {
            // Delta time
            float currTime = glfwGetTime();
            float deltaTime = currTime - lastTime;
            lastTime = currTime;

            // Input
            input.timerTicks(deltaTime);
            input.handleMoveDown(deltaTime);
            for (int i = 0; i < input.moveLeftTimer.consumeExec(); ++i)
            {
                arena.moveHorizontal(true, false);
            }
            for (int i = 0; i < input.moveRightTimer.consumeExec(); ++i)
            {
                arena.moveHorizontal(false, true);
            }

            // Render
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            auto previewSprites = arena.renderPreview();
            spriteRenderer.render(previewSprites, view, ortho);
            auto arenaSprites = arena.render();
            spriteRenderer.render(arenaSprites, view, ortho);
            auto arenaBoundarySprites = arena.renderBoundary();
            spriteRenderer.render(arenaBoundarySprites, view, ortho);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }
};

struct ClientData
{
};

struct ClientToServerData {
    // Frequent update
    vec3 selectedBlockPosition;
    Block selectedBlock;

    // Sometimes update
    vector<vector<ArenaBlock>> blocks; 
};

struct GameEvent
{
};

class Server
{
    mutex mtx;
    queue<GameEvent> gameEventQueue;

public:
    Server(){
    }
    
    void run()
    {
        thread checkConnThread(&Server::checkConnection, this);
        thread gameTickThread(&Server::gameTick, this);

        cout << "suspending until server is done";
        checkConnThread.join();
        gameTickThread.join();
    }

    void gameTick()
    {
        
    }

    void checkConnection()
    {
        ENetAddress address = {0};
        address.host = ENET_HOST_ANY;
        address.port = 7777;

        const int maxClients = 5;
        ENetHost *server = enet_host_create(&address, maxClients, 2, 0, 0);
        if (server == NULL)
        {
            cout << "Error occurred while trying to create an ENet server host" << endl;
            return;
        }
        cout << "Starting a server..."  << endl;

        ENetEvent event;
        while (enet_host_service(server, &event, serverTickMs))
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                printf("A new client connected from %x:%u.\n", event.peer->address.host, event.peer->address.port);
                /* Store any relevant client information here. */
                ClientData clientData;
                event.peer->data = &clientData;
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                printf("A packet of length %lu containing %s was received from %s on channel %u.\n",
                       event.packet->dataLength,
                       event.packet->data,
                       event.peer->data,
                       event.channelID);
                /* Clean up the packet now that we're done using it. */
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                printf("%s disconnected.\n", event.peer->data);
                /* Reset the peer's client information. */
                event.peer->data = NULL;
                break;

            case ENET_EVENT_TYPE_NONE:
                break;
            }
        }
    }
};

int main(int argc, char *argv[])
{
    srand(time(NULL));

    Args args;
    if (!handleArgs(&args, argc, argv))
    {
        return -1;
    }

    if (enet_initialize() != 0)
    {
        return -1;
    }

    if (args.dedicatedServer)
    {
        Server server;
        server.run();
    }
    else
    {
        Tetris tetris;
        tetris.run();
    }

    enet_deinitialize();
    return 0;
}