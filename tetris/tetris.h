#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

#include <time.h>
#include <stdlib.h>

using namespace std;
using namespace glm;

#define ARENA_SIZE_X 80
#define ARENA_SIZE_Y 20

//[TYPE][ROTATION][I][J]
static const vector<vector<vector<vector<int>>>> templates{
#define TYPE_Z 0
    {
        {
            {0, 0, 0, 0},
            {0, 1, 1, 0},
            {0, 0, 1, 1},
            {0, 0, 0, 0},
        },
        {
            {0, 0, 0, 0},
            {0, 0, 1, 0},
            {0, 1, 1, 0},
            {0, 1, 0, 0},
        },
    }};

class Block
{
    int type;
    int rotation;

    static Block random_one()
    {
        Block b;
        b.type = rand() % templates.size();
        b.rotation = rand() % templates[b.type].size();
        return b;
    }
};

class Arena
{
    vec3 position;
    vec2 size;

    vec2 getBlockSize()
    {
        return vec2(
            size.x / ARENA_SIZE_X,
            size.y / ARENA_SIZE_Y);
    }

    void tick(float deltaTime)
    {
    }
};