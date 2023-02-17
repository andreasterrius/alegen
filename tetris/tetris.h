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

#define ARENA_SIZE_X 10
#define ARENA_SIZE_Y 24
#define ARENA_HIDDEN_HEIGHT 4
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
    vec3 color;

    static Block randomOne()
    {
        Block b;
        b.type = rand() % templates.size();
        b.rotation = rand() % templates[b.type].size();
        b.color = vec3(
            ((float) (rand() % 256) / 255), 
            ((float) (rand() % 256) / 255), 
            ((float) (rand() % 256) / 255));
        return b;
    }

    vector<Sprite> render(vec2 blockSize)
    {
        vector<Sprite> sprites;
        vector<vector<int>> blocks = templates[type][rotation];
        for (int i = 0; i < blocks.size(); ++i)
        {
            for (int j = 0; j < blocks[i].size(); ++j)
            {
                if (blocks[i][j] == 1)
                {
                    vec2 pos = vec2(j, i) * blockSize;
                    sprites.push_back(
                        {vec3(pos, 0.0), 
                        blockSize, 
                        vec4(color, 0.0)});
                }
            }
        }
        return sprites;
    }
};

struct ArenaBlock
{
public:
    bool isFilled;
    vec3 color;
};

class Arena
{
public:
    vec2 position;
    vec2 size;
    deque<Block> next;

    vec2 selectedIndex;
    unique_ptr<Block> selected;

    ArenaBlock blocks[ARENA_SIZE_Y][ARENA_SIZE_X];

    Arena(vec2 position, int sizeX)
    {
        this->position = position;
        size.x = sizeX;
        vec2 b = getBlockSize();
        size.y = b.y * ARENA_SIZE_Y;

        fillNext();
        resetArena();
    }

    void resetArena(){
        for(int i = 0; i < ARENA_SIZE_Y; ++i){
            for(int j = 0; j < ARENA_SIZE_X; ++j){
                blocks[i][j] = ArenaBlock{false, vec3()};
            }
        }
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
        selected = make_unique<Block>(Block(block)); // copy & own
        selectedIndex = vec2(ARENA_SIZE_X / 2 - templates[selected->type][selected->rotation][0].size() / 2, 0);
        fillNext();
    }

    vec2 getBlockSize()
    {
        auto kx = size.x / ARENA_SIZE_X;
        return vec2(kx, kx);
    }

    vector<ivec2> getSelectedIndex(){
        vector<ivec2> indices;
        if (selected != nullptr)
        {
            auto block = templates[selected->type][selected->rotation];
            for (int i = 0; i < block.size(); ++i)
            {
                for (int j = 0; j < block[i].size(); ++j)
                {
                    if(block[i][j] == 0) continue;
                    int x = selectedIndex.x + j;
                    int y = selectedIndex.y + i;
                    indices.push_back(ivec2(x, y));
                }
            }
        }
        return indices;
    }

    void moveDown()
    {
        // if prev block is placed, then we get a new one
        if (selected == nullptr)
        {
            selectNext();
        }

        auto si = getSelectedIndex();
        for(auto &s: si){
            blocks[s.y][s.x] = ArenaBlock{false, vec3()};
        }
        selectedIndex.y += 1;
        si = getSelectedIndex();
        for(auto &s: si){
            vec3 color;
            if(selected != nullptr){
                color = selected->color;
            }
            blocks[s.y][s.x] = ArenaBlock{true, color};
        }
        cout << selectedIndex.y << endl;
    }

    vector<Sprite> renderPreview()
    {
        vec2 blockSize = getBlockSize();
        vec2 startPos = vec2(position.x + size.x, position.y);

        vector<Sprite> sprites;
        for (int i = 0; i < next.size(); ++i)
        {
            auto n = next[i].render(blockSize);

            float lowestY = -numeric_limits<float>::infinity();
            for (int j = 0; j < n.size(); ++j)
            {
                n[j].position += vec3(
                    startPos.x + blockSize.x * 2.0, 
                    startPos.y,
                    0.0);
                sprites.push_back(n[j]);
                lowestY = std::max(lowestY, n[j].position.y + blockSize.y);
            }

            startPos.y = lowestY + blockSize.y;
        }

        return sprites;
    }

    vector<Sprite> render()
    {
        vec2 blockSize = getBlockSize();
        vec2 startPos = vec2(position.x, position.y - ARENA_HIDDEN_HEIGHT*blockSize.y);

        vector<Sprite> sprites;
        for (int i = 0; i < ARENA_SIZE_Y; ++i)
        {
            for (int j = 0; j < ARENA_SIZE_X; ++j)
            {
                if(!blocks[i][j].isFilled){
                    continue;
                }
                sprites.push_back(Sprite{
                    vec3(startPos + vec2(j * blockSize.x, i * blockSize.y), 0.0),
                    vec2(blockSize),
                    blocks[i][j].color});
            }
        }

        return sprites;
    }

    vector<Sprite> renderBoundary() {
        //render the boundarys of the tetris arena
        vec2 blockSize = getBlockSize();
        vec2 halfBlockSize = blockSize/vec2(4.0);

        //top left
        vec2 topLeft = vec2(position.x - halfBlockSize.x, position.y);
        vec2 bottomLeft = vec2(topLeft.x, size.y - ARENA_HIDDEN_HEIGHT * blockSize.y);
        vec2 topRight = vec2(position.x + size.x, position.y);

        vec4 color = vec4(vec3(1.0), 1.0);

        return vector<Sprite>{
            Sprite{vec3(topLeft, 0.0), vec2(halfBlockSize.x, size.y - ARENA_HIDDEN_HEIGHT * blockSize.y), color},
            Sprite{vec3(bottomLeft, 0.0), vec2(size.x + (blockSize.x - 2 * halfBlockSize.x), halfBlockSize.y), color},
            Sprite{vec3(topRight, 0.0), vec2(halfBlockSize.x, size.y - ARENA_HIDDEN_HEIGHT * blockSize.y), color},
        };
    }
};