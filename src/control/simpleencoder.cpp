//
// simpleencoder.cpp
//
// mt32-pi - A baremetal MIDI synthesizer for Raspberry Pi
// Copyright (C) 2020-2023 Dale Whinham <daleyo@gmail.com>
//
// This file is part of mt32-pi.
//
// mt32-pi is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// mt32-pi is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// mt32-pi. 
//

#include "control/button.h"
#include "control/control.h"
#include <circle/timer.h>
#include "utility.h"  // for Utility::TicksToMillis()

constexpr u8 GPIOPinButton1         = 17;
constexpr u8 GPIOPinButton2         = 27;
constexpr u8 GPIOPinEncoderButton   = 4;
constexpr u8 GPIOPinEncoderCLK      = 22;
constexpr u8 GPIOPinEncoderDAT      = 23;

// maximum interval (ms) between two presses to count as a double‑click
constexpr u32 kDoubleClickWindowMs   = 300;
// how long to wait before confirming a single‑click
constexpr u32 kSingleClickTimeoutMs  = 300;

constexpr u8 ButtonMask = (1 << static_cast<u8>(TButton::Button1)) |
                          (1 << static_cast<u8>(TButton::Button2)) |
                          (1 << static_cast<u8>(TButton::EncoderButton));

CControlSimpleEncoder::CControlSimpleEncoder(TEventQueue& pEventQueue,
                                             CRotaryEncoder::TEncoderType EncoderType,
                                             bool bEncoderReversed)
    : CControl(pEventQueue),
      m_GPIOEncoderButton(GPIOPinEncoderButton, TGPIOMode::GPIOModeInputPullUp),
      m_GPIOButton1       (GPIOPinButton1,       TGPIOMode::GPIOModeInputPullUp),
      m_GPIOButton2       (GPIOPinButton2,       TGPIOMode::GPIOModeInputPullUp),
      m_Encoder           (EncoderType, bEncoderReversed, GPIOPinEncoderCLK, GPIOPinEncoderDAT),
      m_LastEncoderClickTime(0),
      m_EncoderWasPressed(false)
{
}

void CControlSimpleEncoder::Update()
{
    CControl::Update();

    const s8 nEncoderDelta = m_Encoder.Read();
    if (nEncoderDelta != 0)
    {
        TEvent Event;
        Event.Type          = TEventType::Encoder;
        Event.Encoder.nDelta = nEncoderDelta;
        m_pEventQueue->Enqueue(Event);
    }
}

void CControlSimpleEncoder::ReadGPIOPins()
{
    // 1) Read & debounce
    const u32 nGPIOState = CGPIOPin::ReadAll();
    const u8 nButtonState =
        (((nGPIOState >> GPIOPinButton1)       & 1) << static_cast<u8>(TButton::Button1))       |
        (((nGPIOState >> GPIOPinButton2)       & 1) << static_cast<u8>(TButton::Button2))       |
        (((nGPIOState >> GPIOPinEncoderButton) & 1) << static_cast<u8>(TButton::EncoderButton));

    DebounceButtonState(nButtonState, ButtonMask);

    const bool bPressed = !(nButtonState & (1 << static_cast<u8>(TButton::EncoderButton)));
    const u32  now      = CTimer::GetClockTicks();

    // these statics live across calls:
    static bool buttonDown    = false;

    // 2) Edge detect on encoder-button press
    if (bPressed && !buttonDown)
    {
        buttonDown = true;

        // how long since our *last* press?
        u32 deltaMs = Utility::TicksToMillis(now - m_LastEncoderClickTime);

        // if it’s too soon after the last click, ignore entirely
        if (deltaMs < 50)  // debounce / too-quick guard (tweak threshold as needed)
        {
            m_LastEncoderClickTime = now;
            return;
        }

        // record for next interval
        m_LastEncoderClickTime = now;

        // double‑click?
        if (m_EncoderWasPressed && (deltaMs <= kDoubleClickWindowMs))
        {
            // dispatch double-click event
            TEvent Event;
            Event.Type                = TEventType::Button;
            Event.Button.Button       = TButton::EncoderButton;
            Event.Button.bPressed     = true;
            Event.Button.bRepeat      = false;
            Event.Button.bDoubleClick = true;
            m_pEventQueue->Enqueue(Event);

            // clear pending flag so next press is fresh
            m_EncoderWasPressed = false;
        }
        else
        {
            // start single‑click pending
            m_EncoderWasPressed = true;
        }
    }
    else if (!bPressed && buttonDown)
    {
        // Just released
        buttonDown = false;
    }

    // 3) Single‑click timeout
    if (m_EncoderWasPressed &&
        Utility::TicksToMillis(now - m_LastEncoderClickTime) > kSingleClickTimeoutMs)
    {
        // fire single-click
        TEvent Event;
        Event.Type                = TEventType::Button;
        Event.Button.Button       = TButton::EncoderButton;
        Event.Button.bPressed     = true;
        Event.Button.bRepeat      = false;
        Event.Button.bDoubleClick = false;
        m_pEventQueue->Enqueue(Event);

        m_EncoderWasPressed = false;
    }

    // 4) Read rotary ticks
    m_Encoder.ReadGPIOPins(
        (nGPIOState >> GPIOPinEncoderCLK) & 1,
        (nGPIOState >> GPIOPinEncoderDAT) & 1
    );
}
