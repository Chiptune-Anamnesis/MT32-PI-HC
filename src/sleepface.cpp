// src/sleepface.cpp

#include <circle/timer.h>
#include <circle/sched/scheduler.h>
#include <cstdlib>
#include <cstdint>

#include "utility.h"
#include "mt32pi.h"
#include "anim0.h"
#include "anim1.h"
#include "anim2.h"
#include "anim3.h"
#include "anim4.h"
#include "anim5.h"
#include "anim6.h"
#include "anim7.h"
#include "anim8.h"
#include "anim9.h"
#include "anim10.h"
#include "anim11.h"
#include "anim12.h"

//-----------------------------------------------------------------------------
// Animation storage
//-----------------------------------------------------------------------------

struct Animation {
    const uint8_t (*frames)[32][128];
    size_t        frameCount;
};

static const Animation animations[] = {
    { anim0,  7  },
    { anim1,  7  },
    { anim2, 12  },
    { anim3, 10  },
    { anim4, 19  },
    { anim6, 10  },
    { anim7, 16  },
    { anim8, 10  },
    { anim9, 20  },
    { anim10, 16  },
    { anim11, 16  },
    { anim12, 16  },
};

constexpr size_t   kNumAnimations   = sizeof(animations) / sizeof(animations[0]);
constexpr uint32_t kMinPeriodMs     = 30  * 1000;   // 30 seconds
constexpr uint32_t kMaxPeriodMs     = 180 * 1000;   // 180 seconds
constexpr uint32_t kDefaultFrameMs  = 200;          // 200 ms per frame

//-----------------------------------------------------------------------------
// Draw a single 128×32 frame via SetPixel + Flip()
//-----------------------------------------------------------------------------
static void DrawFrame(CLCD *lcd, const uint8_t frame[32][128], int8_t dx, int8_t dy)
{
    lcd->Clear(false);

    for (int y = 0; y < 32; ++y)
    {
        for (int x = 0; x < 128; ++x)
        {
            // inverted pixel: original is 0
            if (!frame[y][x])
            {
                // check 8 neighbors for at least one other zero
                int zeros = 0;
                for (int yy = y - 1; yy <= y + 1; ++yy)
                {
                    for (int xx = x - 1; xx <= x + 1; ++xx)
                    {
                        if (yy == y && xx == x) 
                            continue;
                        if (yy < 0 || yy >= 32 || xx < 0 || xx >= 128) 
                            continue;
                        if (!frame[yy][xx]) 
                        {
                            ++zeros;
                            yy = y + 1; // break outer
                            break;
                        }
                    }
                }
                if (zeros == 0)
                    continue;   // no neighbor zeros → skip this isolated speck

                // draw the “cluster” pixel
                lcd->SetPixel(x + dx, y + dy);
            }
        }
    }

    lcd->Flip();
}



//-----------------------------------------------------------------------------
// Play one animation on the given LCD with jitter, neutral pause, breathing
//-----------------------------------------------------------------------------

static void ShowCuteFaceAnimation(CLCD *lcd,
                                  size_t animIndex,
                                  uint32_t totalDurationMs,
                                  uint32_t frameMs = kDefaultFrameMs)
{
    const Animation &A       = animations[animIndex];
    size_t    startOffset    = std::rand() % A.frameCount;
    uint32_t  startTick      = CTimer::GetClockTicks();
    uint32_t  totalTicks     = Utility::MillisToTicks(totalDurationMs);

    // precompute base ticks for frameMs
    uint32_t baseFrameTicks  = Utility::MillisToTicks(frameMs);

    while (!CMT32Pi::s_pThis->m_bAbortSleepAnimation
           && (CTimer::GetClockTicks() - startTick) < totalTicks)
    {
        uint32_t elapsed    = CTimer::GetClockTicks() - startTick;
        size_t   rawFrame   = (elapsed / baseFrameTicks) % A.frameCount;
        size_t   frameIndex = (rawFrame + startOffset) % A.frameCount;

        // jitter ±20ms
        int32_t jitterMs = (std::rand() % 41) - 20;
        uint32_t frameDelay = Utility::MillisToTicks(int32_t(frameMs) + jitterMs);

        // extra pause on neutral (frame 0)
        if (frameIndex == 0)
            frameDelay += Utility::MillisToTicks(200);

        // breathing shift −1..+1
        //int8_t dx = (std::rand() % 3) - 1;
        //int8_t dy = (std::rand() % 3) - 1;
        int8_t dx = 0;
        int8_t dy = 0;

        DrawFrame(lcd, A.frames[frameIndex], dx, dy);

        // wait for this (jittered + neutral) interval
        CTimer::SimpleMsDelay(Utility::TicksToMillis(frameDelay));
        CScheduler::Get()->Yield();
    }
}

//-----------------------------------------------------------------------------
// Thread entry point – declared in mt32pi.h as:
//    static int SleepFaceThread(void*);
//-----------------------------------------------------------------------------

int CMT32Pi::SleepFaceThread(void *)
{
    CLCD *lcd = s_pThis->m_pLCD;

    // small initial delay to seed more varied
    CTimer::SimpleMsDelay(5);
    uint32_t seed = CTimer::GetClockTicks() ^ reinterpret_cast<uintptr_t>(&animations);
    std::srand(seed);

    // clear any previous abort
    s_pThis->m_bAbortSleepAnimation = false;

    while (s_pThis->m_bShowSleepAnimation)
    {
        if (s_pThis->m_bAbortSleepAnimation)
            break;

        // pick random animation and duration
        size_t   animIndex = std::rand() % kNumAnimations;
        uint32_t span      = kMaxPeriodMs - kMinPeriodMs + 1;
        uint32_t totalMs   = kMinPeriodMs + (std::rand() % span);

        ShowCuteFaceAnimation(lcd, animIndex, totalMs, kDefaultFrameMs);

        if (s_pThis->m_bAbortSleepAnimation)
            break;

        // clear abort for next run
        s_pThis->m_bAbortSleepAnimation = false;
    }

    s_pThis->m_bShowSleepAnimation = false;
    return 0;
}
