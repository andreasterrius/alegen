#pragma once

using namespace std;

class Timer
{
public:
    float isStarted;
    float currTimer;
    float currTimerLimit;

    float execCount;

    float timerLimit;
    float firstStickyTimerLimit; // first timer used after start

    Timer(float timerLimit) : timerLimit(timerLimit), firstStickyTimerLimit(timerLimit)
    {
        stop();
    }

    Timer(float timerLimit, float firstStickyTimerLimit) : timerLimit(timerLimit),
                                                           firstStickyTimerLimit(firstStickyTimerLimit)
    {
        stop();
    }

    void addExec(int count)
    {
        execCount += count;
    }

    void start()
    {
        isStarted = true;
    }

    void stop()
    {
        isStarted = false;
        currTimer = 0.0f;
        execCount = 0.0f;
        currTimerLimit = firstStickyTimerLimit;
    }

    void tick(float deltaTime)
    {
        if (!isStarted)
            return;
        currTimer += deltaTime;
        if (currTimer >= currTimerLimit)
        {
            currTimer -= currTimerLimit;
            int overLimit = 1;
            currTimerLimit = timerLimit; // force back to non sticky after first tick;

            if(currTimer > currTimerLimit) {
                overLimit += floor(currTimer / currTimerLimit);
                float leftOver = currTimer - (overLimit * currTimerLimit);
                currTimer = leftOver;
            }

            execCount += overLimit;
        }
    }

    int consumeExec()
    {
        int e = execCount;
        execCount = 0;
        return e;
    }
};