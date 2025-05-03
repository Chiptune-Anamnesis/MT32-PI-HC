// src/sleepface.cpp

#include <circle/timer.h>
#include <circle/sched/scheduler.h>
#include <fatfs/ff.h>
#include <cstdlib>
#include <cstdint>
#include <cstring>

#include "utility.h"
#include "mt32pi.h"

constexpr uint32_t kMinPeriodMs     = 30  * 1000;   // 30 seconds
constexpr uint32_t kMaxPeriodMs     = 180 * 1000;   // 180 seconds
constexpr uint32_t kDefaultFrameMs  = 200;          // 200 ms per frame
constexpr size_t   kFrameSize       = 128 * 32;     // bytes per frame
constexpr size_t   kMaxFrames       = 64;           // max frames per animation
constexpr size_t   kMaxAnimations   = 16;           // max animations to load

struct Animation {
    uint8_t *frames[kMaxFrames] = {nullptr};
    size_t frameCount = 0;
};

static Animation animations[kMaxAnimations];
static size_t kNumAnimations = 0;

static bool LoadAnimation(const char *filename, Animation &anim)
{
    FIL file;
    if (f_open(&file, filename, FA_READ) != FR_OK)
        return false;

    size_t index = 0;
    while (index < kMaxFrames)
    {
        anim.frames[index] = (uint8_t *)malloc(kFrameSize);
        if (!anim.frames[index])
            break;

        UINT br;
        if (f_read(&file, anim.frames[index], kFrameSize, &br) != FR_OK || br < kFrameSize)
        {
            free(anim.frames[index]);
            anim.frames[index] = nullptr;
            break;
        }
        ++index;
    }

    anim.frameCount = index;
    f_close(&file);
    return (index > 0);
}

static void LoadAllAnimations()
{
    DIR dir;
    FILINFO fno;
    if (f_opendir(&dir, "/animations") != FR_OK)
        return;

    while (kNumAnimations < kMaxAnimations && f_readdir(&dir, &fno) == FR_OK && fno.fname[0])
    {
        if (!(fno.fattrib & AM_DIR))
        {
            char path[64];
            snprintf(path, sizeof(path), "/animations/%s", fno.fname);
            if (LoadAnimation(path, animations[kNumAnimations]))
                ++kNumAnimations;
        }
    }
    f_closedir(&dir);
}

static void DrawFrame(CLCD *lcd, const uint8_t *frame, int8_t dx, int8_t dy)
{
    lcd->Clear(false);

    for (int y = 0; y < 32; ++y)
    {
        for (int x = 0; x < 128; ++x)
        {
            if (!frame[y * 128 + x])
            {
                int zeros = 0;
                for (int yy = y - 1; yy <= y + 1; ++yy)
                {
                    for (int xx = x - 1; xx <= x + 1; ++xx)
                    {
                        if (yy == y && xx == x) continue;
                        if (yy < 0 || yy >= 32 || xx < 0 || xx >= 128) continue;
                        if (!frame[yy * 128 + xx]) {
                            ++zeros;
                            yy = y + 1;
                            break;
                        }
                    }
                }
                if (zeros == 0) continue;
                lcd->SetPixel(x + dx, y + dy);
            }
        }
    }

    lcd->Flip();
}

static void ShowCuteFaceAnimation(CLCD *lcd, size_t animIndex, uint32_t totalDurationMs, uint32_t frameMs = kDefaultFrameMs)
{
    const Animation &A = animations[animIndex];
    if (A.frameCount == 0) return;

    size_t startOffset = std::rand() % A.frameCount;
    uint32_t startTick = CTimer::GetClockTicks();
    uint32_t totalTicks = Utility::MillisToTicks(totalDurationMs);
    uint32_t baseFrameTicks = Utility::MillisToTicks(frameMs);

    while (!CMT32Pi::s_pThis->m_bAbortSleepAnimation && (CTimer::GetClockTicks() - startTick) < totalTicks)
    {
        uint32_t elapsed = CTimer::GetClockTicks() - startTick;
        size_t rawFrame = (elapsed / baseFrameTicks) % A.frameCount;
        size_t frameIndex = (rawFrame + startOffset) % A.frameCount;

        int32_t jitterMs = (std::rand() % 41) - 20;
        uint32_t frameDelay = Utility::MillisToTicks(int32_t(frameMs) + jitterMs);

        if (frameIndex == 0)
            frameDelay += Utility::MillisToTicks(200);

        DrawFrame(lcd, A.frames[frameIndex], 0, 0);
        CTimer::SimpleMsDelay(Utility::TicksToMillis(frameDelay));
        CScheduler::Get()->Yield();
    }
}

int CMT32Pi::SleepFaceThread(void *)
{
    CLCD *lcd = s_pThis->m_pLCD;
    CTimer::SimpleMsDelay(5);
    std::srand(CTimer::GetClockTicks() ^ reinterpret_cast<uintptr_t>(&animations));

    s_pThis->m_bAbortSleepAnimation = false;

    LoadAllAnimations();

    while (s_pThis->m_bShowSleepAnimation)
    {
        if (s_pThis->m_bAbortSleepAnimation || kNumAnimations == 0)
            break;

        size_t animIndex = std::rand() % kNumAnimations;
        uint32_t span = kMaxPeriodMs - kMinPeriodMs + 1;
        uint32_t totalMs = kMinPeriodMs + (std::rand() % span);

        ShowCuteFaceAnimation(lcd, animIndex, totalMs, kDefaultFrameMs);

        if (s_pThis->m_bAbortSleepAnimation)
            break;

        s_pThis->m_bAbortSleepAnimation = false;
    }

    s_pThis->m_bShowSleepAnimation = false;
    return 0;
}
