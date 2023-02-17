#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <deque>
#include <limits>
#include <memory>

#include <time.h>
#include <stdlib.h>

#include "sprite_renderer.h"

using namespace std;
using namespace glm;

#define ARENA_SIZE_X 20
#define ARENA_SIZE_Y 80
#define BLOCKS_IN_QUEUE 3

//[TYPE][ROTATION][I][J]
static const vector<vector<vector<vector<int>>>> templates{
#define TYPE_Z 0
    {
        {
            {1, 1, 0},
            {0, 1, 1},
        },
        {
            {0, 1},
            {1, 1},
            {1, 0},
        },
    }};

class Block
{
public:
    int type;
    int rotation;

    static Block randomOne()
    {
        Block b;
        b.type = rand() % templates.size();
        b.rotation = rand() % templates[b.type].size();
        return b;
    }

    vector<Sprite> render(vec2 position, vec2 blockSize)
    {
        vector<Sprite> sprites;
        vector<vector<int>> blocks = templates[type][rotation];
        for (int i = 0; i < blocks.size(); ++i)
        {
            for (int j = 0; j < blocks[i].size(); ++j)
            {
                if (blocks[i][j] == 1)
                {
                    vec2 pos = vec2(j, i) * blockSize + position;
                    sprites.push_back({vec3(pos, 0.0), blockSize, vec4(vec3(1.0), 0.0)});
                }
            }
        }
        return sprites;
    }
};

struct PlacedBlock
{
public:
    vec3 color;
};

class Arena
{
public:
    vec3 position;
    vec2 size;
    deque<Block> next;

    vec2 selectedIndex;
    unique_ptr<Block> selected;

    PlacedBlock blocks[ARENA_SIZE_Y][ARENA_SIZE_X];

    Arena()
    {
        fillNext();
    }

    void fillNext()
    {
        while (next.size() < BLOCKS_IN_QUEUE)
        {
            next.push_back(Block::randomOne());
        }
    }

    void selectNext()
    {
        auto block = next.front();
        next.pop_front();
        selected = make_unique<Block>(Block(block)); //copy & own
        selectedIndex = vec2(ARENA_SIZE_X/2-templates[selected->type][selected->rotation][0].size()/2, 0);
        fillNext();
    }

    vec2 getBlockSize()
    {
        auto kx = size.x / ARENA_SIZE_X;
        return vec2(kx, kx);
    }

    void moveDown()
    {
        selectedIndex.y += 1;
        //if placed
        if(selected == nullptr){
            selectNext();
        }  
    }

    vector<Sprite> renderPreview()
    {
        vec2 blockSize = getBlockSize();
        vec2 startPos = vec2(position.x + size.x, position.y);

        vector<Sprite> sprites;
        for (int i = 0; i < next.size(); ++i)
        {
            auto n = next[i].render(startPos, blockSize);

            float lowestY = -numeric_limits<float>::infinity();
            for (int j = 0; j < n.size(); ++j)
            {
                sprites.push_back(n[j]);
                lowestY = std::max(lowestY, n[j].position.y + blockSize.y);
            }

            startPos.y = lowestY + blockSize.y;
        }

        // for(auto &d: sprites) {
        //     cout << d.position.x << "," << d.position.y << " "
        //         << d.size.x << "," << d.size.y << endl;
        // }
        // cout << sprites.size() << endl;

        return sprites;
    }

    vector<Sprite> render()
    {
        vec2 blockSize = getBlockSize();
        vec2 startPos = vec2(position.x, position.y);

        vector<Sprite> sprites;
        return sprites;
    }
};