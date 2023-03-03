#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;

string readFile(string fileName)
{
    ifstream inFile;
    inFile.open(fileName); // open the input file

    stringstream strStream;
    strStream << inFile.rdbuf();  // read the file
    string str = strStream.str(); // str holds the content of the file

    return str;
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


struct Sprite
{
    vec3 position;
    vec2 size;
    vec3 color;

    GLuint textureId;
};

class SpriteRenderer
{
public:
    int VAO, VBO;
    int spriteShader;

    SpriteRenderer()
    {
        float vertices[] = {
            0.0f, 0.0f, // bot left
            1.0f, 0.0f,  // bot right
            0.0f, 1.0f,  // top left
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
            model = translate(model, sprite.position);
            model = scale(model, glm::vec3(sprite.size.x, sprite.size.y, 1.0f));

            int modelLoc = glGetUniformLocation(spriteShader, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);

            int colorLoc = glGetUniformLocation(spriteShader, "color");
            glUniform4fv(colorLoc, 1, &sprite.color[0]);

            // render glyph texture over quad
            glBindTexture(GL_TEXTURE_2D, sprite.textureId);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
    }
};
