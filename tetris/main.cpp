#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <enet/enet.h>
#include <cxxopts.hpp>

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
#include <atomic>

#include "timer.h"
#include "text_renderer.h"
#include "sprite_renderer.h"
#include "tetris.h"
#include "util.h"

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

    string hostIp;
    int hostPort;
};

bool handleArgs(Args *args, int argc, char *argv[])
{
    using namespace cxxopts;
    Options options("Tetris", "a heartpounding versus tetris game");
    options.add_options()
        ("s,server", "Enable dedicated server", value<bool>()->default_value("false"))
        ("c,client", "Connect to a given <ip>:<port> server", value<string>()->default_value("")),
        ("h,help", "Print usage");
    auto result = options.parse(argc, argv);
    if (result.count("help"))
    {
      cout << options.help() << endl;
      exit(0);
    }


    args->dedicatedServer = result["server"].as<bool>();
    vector<string> host = stringSplit(result["client"].as<string>(), ":");
    args->hostIp = host.size() >= 1 ? host[0] : "none";
    try
    {
        args->hostPort = stoi(host.size() >= 2 ? host[1] : "");
    }
    catch (invalid_argument &e)
    {
        args->hostPort = 0;
    }

    cout << "client: " << args->hostIp << ":"  << args->hostPort << " " << endl;

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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

    return true;
}

GLFWwindow *createWindow()
{
    filesystem::current_path(getExeParentDirectory());

    // glfwSetErrorCallback([](int error, const char *description)
    //                      { fprintf(stderr, "GLFW Error %d: %s\n", error, description); });
    if (!glfwInit())
        return nullptr;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_FALSE);

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

class BlockChangeReplicator : public SelectedBlockChangeListener
{
public:
    BlockChangeReplicator()
    {
    }

    void onChange(Block *b) override
    {

        ENetPacket *packet = enet_packet_create(b, sizeof(Block), 0);
        // enet_peer_send();

        // cout << "on change" << endl;
        // should send message to server here
    }

    void onPlace(Block *b, unordered_set<int> *checkY) override
    {
        //cout << "on place" << endl;
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
    TextRenderer textRenderer;
    Arena arena;
    Input input;

    mat4 ortho;
    mat4 view;

    Tetris() : arena(vec2(100, 0), 300), window(createWindow()), spriteRenderer(), 
        textRenderer("resources/font/Roboto/Roboto-Regular.ttf")
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

            auto textSprites = textRenderer.layoutText(vec3(300.0f, 300.0f, 0.0f), "abcdefghijk");
            spriteRenderer.render(textSprites, view, ortho);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }
};

struct ClientData
{
};

struct GameEvent
{
};

class Server
{
    mutex mtx;
    queue<GameEvent> gameEventQueue;

public:
    Server()
    {
    }

    void run()
    {
        thread checkConnThread(&Server::checkConnection, this);
        cout << "Suspending until server is done\n";
        checkConnThread.join();
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
        cout << "Starting a server..." << endl;

        ENetEvent event;
        while (true)
        {
            int code = enet_host_service(server, &event, serverTickMs);
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

class Client
{
public:
    string hostIp;
    int hostPort;
    ENetHost *client;
    atomic_bool shouldQuit;

    Client(string hostIp, int hostPort) : hostIp(hostIp), hostPort(hostPort), shouldQuit(false)
    {
    }

    void run()
    {
        thread checkConnThread;
        bool checkConnRunning = false;
        if (hostIp != "" && hostPort != 0)
        {
            client = enet_host_create(NULL, 1, 2, 0, 0);
            ENetAddress address;
            enet_address_set_host(&address, hostIp.c_str());
            address.port = hostPort;
            ENetPeer *serverPeer = enet_host_connect(client, &address, 2, 0);
            checkConnThread = thread(&Client::checkConnection, this);
            checkConnRunning = true;
        }

        Tetris tetris;
        tetris.run();

        shouldQuit = true;

        if(checkConnRunning) {
            checkConnThread.join();
        }
    }

    void checkConnection()
    {
        ENetEvent event;
        while (!shouldQuit.load())
        {
            enet_host_service(client, &event, serverTickMs);
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                printf("A new client connected from %x:%u.\n", event.peer->address.host, event.peer->address.port);
                /* Store any relevant client information here. */
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
        Client client(args.hostIp, args.hostPort);
        client.run();
    }
}
