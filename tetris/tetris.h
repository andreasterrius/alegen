#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <deque>
#include <limits>
#include <memory>
#include <unordered_set>
#include <algorithm>

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
    {{
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
     }},
#define TYPE_Z_ALT 1
    {{
         {0, 0, 0, 0},
         {0, 0, 1, 1},
         {0, 1, 1, 0},
         {0, 0, 0, 0},
     },
     {
         {0, 0, 0, 0},
         {0, 1, 0, 0},
         {0, 1, 1, 0},
         {0, 0, 1, 0},
     }},
#define TYPE_I 2
    {{
         {0, 0, 1, 0},
         {0, 0, 1, 0},
         {0, 0, 1, 0},
         {0, 0, 1, 0},
     },
     {
         {0, 0, 0, 0},
         {0, 0, 0, 0},
         {1, 1, 1, 1},
         {0, 0, 0, 0},
     }},
#define TYPE_BOX 3
    {{
        {0, 0, 0, 0},
        {0, 1, 1, 0},
        {0, 1, 1, 0},
        {0, 0, 0, 0},
    }},
#define TYPE_L 4
    {{
         {0, 0, 0, 0},
         {0, 1, 0, 0},
         {0, 1, 0, 0},
         {0, 1, 1, 0},
     },
     {
         {0, 0, 0, 0},
         {0, 0, 0, 0},
         {0, 1, 1, 1},
         {0, 1, 0, 0},
     },
     {
         {0, 0, 0, 0},
         {0, 1, 1, 0},
         {0, 0, 1, 0},
         {0, 0, 1, 0},
     },
     {
         {0, 0, 0, 0},
         {0, 0, 0, 0},
         {0, 0, 1, 0},
         {1, 1, 1, 0},
     }},
#define TYPE_L_ALT 5
    {{
         {0, 0, 0, 0},
         {0, 0, 1, 0},
         {0, 0, 1, 0},
         {0, 1, 1, 0},
     },
     {
         {0, 0, 0, 0},
         {0, 0, 0, 0},
         {1, 1, 1, 0},
         {0, 0, 1, 0},
     },
     {
         {0, 0, 0, 0},
         {0, 1, 1, 0},
         {0, 1, 0, 0},
         {0, 1, 0, 0},
     },
     {
         {0, 0, 0, 0},
         {0, 0, 0, 0},
         {0, 1, 0, 0},
         {0, 1, 1, 1},
     }},
#define TYPE_T 6
    {{
         {0, 0, 0, 0},
         {0, 1, 0, 0},
         {0, 1, 1, 0},
         {0, 1, 0, 0},
     },
     {
         {0, 0, 0, 0},
         {0, 0, 0, 0},
         {0, 1, 1, 1},
         {0, 0, 1, 0},
     },
     {
         {0, 0, 0, 0},
         {0, 0, 1, 0},
         {0, 1, 1, 0},
         {0, 0, 1, 0},
     },
     {
         {0, 0, 0, 0},
         {0, 0, 1, 0},
         {0, 1, 1, 1},
         {0, 0, 0, 0},
     }}
    /***/};

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
            ((float)(rand() % 256) / 255),
            ((float)(rand() % 256) / 255),
            ((float)(rand() % 256) / 255));
        return b;
    }

    Block rotateCopy()
    {
        Block b;
        b.type = type;
        b.rotation = (rotation + 1) % templates[type].size();
        b.color = color;
        return b;
    }

    void rotate()
    {
        rotation = (rotation + 1) % templates[type].size();
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
    bool isPlaced;
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

    void resetArena()
    {
        for (int i = 0; i < ARENA_SIZE_Y; ++i)
        {
            for (int j = 0; j < ARENA_SIZE_X; ++j)
            {
                blocks[i][j] = ArenaBlock{false, false, vec3()};
            }
        }
        selected = nullptr;
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

    vector<ivec2> getSelectedIndex(Block *b, vec2 index)
    {
        vector<ivec2> indices;
        if (b != nullptr)
        {
            auto block = templates[b->type][b->rotation];
            for (int i = 0; i < block.size(); ++i)
            {
                for (int j = 0; j < block[i].size(); ++j)
                {
                    if (block[i][j] == 0)
                        continue;
                    int x = index.x + j;
                    int y = index.y + i;
                    indices.push_back(ivec2(x, y));
                }
            }
        }
        return indices;
    }

    void moveHorizontal(bool isLeft, bool isRight)
    {
        if (isLeft == isRight)
        {
            return;
        }
        auto si = getSelectedIndex(selected.get(), selectedIndex);

        if (isLeft)
        {
            bool canMoveLeft = true;
            for (auto &s : si)
            {
                if (s.x - 1 < 0 || blocks[s.y][s.x - 1].isPlaced)
                {
                    canMoveLeft = false;
                }
            }
            if (canMoveLeft)
            {
                clearCurrentBlock();
                selectedIndex.x -= 1;
                placeCurrentBlock();
            }
        }
        if (isRight)
        {
            bool canMoveRight = true;
            for (auto &s : si)
            {
                if (s.x + 1 >= ARENA_SIZE_X || blocks[s.y][s.x + 1].isPlaced)
                {
                    canMoveRight = false;
                }
            }
            if (canMoveRight)
            {
                clearCurrentBlock();
                selectedIndex.x += 1;
                placeCurrentBlock();
            }
        }
    }

    void rotate()
    {
        if (selected == nullptr)
            return;
        Block rotated = selected->rotateCopy();
        vector<ivec2> indices = getSelectedIndex(&rotated, selectedIndex);

        for (auto &i : indices)
        {
            if (blocks[i.y][i.x].isPlaced ||
                i.y < 0 || i.y >= ARENA_SIZE_Y ||
                i.x < 0 || i.x >= ARENA_SIZE_X)
            {
                return;
            }
        }
        clearCurrentBlock();
        selected->rotate();
        placeCurrentBlock();
    }

    void moveDown()
    {
        if (selected == nullptr)
        {
            selectNext();
        }

        bool shouldPlace = false;
        auto si = getSelectedIndex(selected.get(), selectedIndex);
        for (auto &s : si)
        {
            if (s.y + 1 >= ARENA_SIZE_Y || blocks[s.y + 1][s.x].isPlaced)
            {
                shouldPlace = true;
                break;
            }
        }
        if (shouldPlace)
        {
            unordered_set<int> checkY;
            bool isDead = false;
            for (auto &s : si)
            {
                blocks[s.y][s.x] = ArenaBlock{true, true, selected->color};
                checkY.insert(s.y);
                if (s.y < ARENA_HIDDEN_HEIGHT)
                {
                    dead();
                    return;
                }
            }
            scoreCheck(checkY);
            selected = nullptr;
            return;
        }
        else
        {
            // move everything down once
            clearCurrentBlock();
            selectedIndex.y += 1;
            placeCurrentBlock();
        }
    }

    void clearCurrentBlock()
    {
        auto si = getSelectedIndex(selected.get(), selectedIndex);
        for (auto &s : si)
        {
            blocks[s.y][s.x] = ArenaBlock{false, false, selected->color};
        }
    }

    void placeCurrentBlock()
    {
        auto si = getSelectedIndex(selected.get(), selectedIndex);
        for (auto &s : si)
        {
            blocks[s.y][s.x] = ArenaBlock{false, true, selected->color};
        }
    }

    void dead()
    {
        resetArena();
    }

    void scoreCheck(unordered_set<int> &checkY)
    {
        deque<int> lineYIndex;
        unordered_set<int> ignorePullDownY;

        // find complete lines
        vector<int> sortedY(checkY.begin(), checkY.end());
        sort(sortedY.begin(), sortedY.end(), greater<int>());
        for (auto &y : sortedY)
        {
            bool hasALine = true;
            for (int x = 0; x < ARENA_SIZE_X; ++x)
            {
                if (!blocks[y][x].isFilled)
                {
                    hasALine = false;
                    break;
                }
            }

            if (hasALine)
            {
                ignorePullDownY.insert(y);
                lineYIndex.push_back(y);
            }
        }

        // pull down
        bool hasFoundEmptyRow = false;
        while (!lineYIndex.empty())
        {
            int currY = lineYIndex.front();
            lineYIndex.pop_front();

            if (hasFoundEmptyRow)
            {
                for (int j = 0; j < ARENA_SIZE_X; ++j)
                {
                    blocks[currY][j] = ArenaBlock{false, false, vec3()};
                }
                continue;
            }

            for (int i = currY - 1; i >= 0; --i)
            {
                if (ignorePullDownY.find(i) != ignorePullDownY.end())
                {
                    continue;
                }
                int ec = 0;
                for (int j = 0; j < ARENA_SIZE_X; ++j)
                {
                    if (!blocks[i][j].isFilled)
                        ec++;
                    blocks[currY][j] = blocks[i][j];
                    blocks[i][j] = ArenaBlock{false, false, vec3()};
                }
                if (ec == ARENA_SIZE_X)
                    hasFoundEmptyRow = true;
                lineYIndex.push_back(i);
                ignorePullDownY.insert(i);
                break;
            }
        }
    }

    vector<Sprite> renderPreview()
    {
        vec2 blockSize = getBlockSize();
        vec2 startPos = vec2(position.x + size.x, position.y + blockSize.y);

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
        vec2 startPos = vec2(position.x, position.y - ARENA_HIDDEN_HEIGHT * blockSize.y);

        vector<Sprite> sprites;
        for (int i = 0; i < ARENA_SIZE_Y; ++i)
        {
            for (int j = 0; j < ARENA_SIZE_X; ++j)
            {
                if (!blocks[i][j].isFilled)
                {
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

    vector<Sprite> renderBoundary()
    {
        // render the boundarys of the tetris arena
        vec2 blockSize = getBlockSize();
        vec2 halfBlockSize = blockSize / vec2(4.0);

        // top left
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