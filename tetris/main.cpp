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

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

using namespace std;
using namespace glm;

const int windowWidth = 400;
const int windowHeight = 400;

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

unsigned int createShader(string &vertexShaderSource, string &fragmentShaderSource)
{
    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char *vss = vertexShaderSource.c_str();
    glShaderSource(vertexShader, 1, &vss, NULL);
    glCompileShader(vertexShader);

    int success = 0;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[1024];
        glGetShaderInfoLog(vertexShader, 1024, NULL, infoLog);
        cout << "vertex compile error:\n"
             << infoLog << "\n";
    }

    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char *fss = fragmentShaderSource.c_str();
    glShaderSource(fragmentShader, 1, &fss, NULL);
    glCompileShader(fragmentShader);
    success = 0;
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[1024];
        glGetShaderInfoLog(fragmentShader, 1024, NULL, infoLog);
        cout << "fragment compile error:\n"
             << infoLog << "\n";
    }

    unsigned int shaderProgram;
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    success = 0;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[1024];
        glGetShaderInfoLog(shaderProgram, 1024, NULL, infoLog);
        cout << "shader link error:\n"
             << infoLog << "\n";
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

string readFile(string fileName)
{
    ifstream inFile;
    inFile.open(fileName); // open the input file

    stringstream strStream;
    strStream << inFile.rdbuf();  // read the file
    string str = strStream.str(); // str holds the content of the file

    return str;
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

    int windowWidth = 300;
    int windowHeight = 300;
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

struct Sprite
{
    vec3 position;
    vec3 color;
};

class SpriteRenderer
{
public:
    int VAO, VBO;
    int spriteShader;

    SpriteRenderer()
    {
        float vertices[] = {
            -1.0f, -1.0f, // bot left
            1.0f, -1.0f,  // bot right
            -1.0f, 1.0f,  // top left
            1.0f, 1.0f,   // top right
        };

        unsigned int VAO;
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        unsigned int VBO;
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        string vertexShaderSource = readFile("resources/shader/sprite.vert");
        string fragmentShaderSource = readFile("resources/shader/sprite.frag");

        this->spriteShader = createShader(vertexShaderSource, fragmentShaderSource);
        this->VAO = VAO;
        this->VBO = VBO;
    }

    void render(vector<Sprite> sprites, mat4 view, mat4 proj)
    {
        glUseProgram(spriteShader);
        glBindVertexArray(VAO);

        int viewLoc = glGetUniformLocation(spriteShader, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);

        int projLoc = glGetUniformLocation(spriteShader, "projection");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, &proj[0][0]);

        for (auto &sprite : sprites)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = scale(model, glm::vec3(200.0f, 200.0f, 1.0f));

            int modelLoc = glGetUniformLocation(spriteShader, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);

            int colorLoc = glGetUniformLocation(spriteShader, "color");
            glUniform4fv(colorLoc, 1, &sprite.color[0]);
        }
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
};

int main()
{
    GLFWwindow *window = createWindow();
    if (window == nullptr)
        return -1;
    SpriteRenderer spriteRenderer;

    mat4 ortho = glm::ortho(0.0f, (float)windowWidth, 0.0f, (float)windowHeight, 0.1f, 100.0f);
    mat4 view = mat4(1.0);
    view  = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));

    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        spriteRenderer.render(vector<Sprite>{
                                  Sprite{vec3(0.0f), vec4(1.0f, 0.0f, 0.0f, 0.0f)}},
                              view, ortho);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}