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
// mt32-pi. If not, see <http://www.gnu.org/licenses/>.
//

#include "control/button.h"
#include "control/control.h"
#include <circle/timer.h>

constexpr u8 GPIOPinButton1 = 17;
constexpr u8 GPIOPinButton2 = 27;

constexpr u8 GPIOPinEncoderButton = 4;
constexpr u8 GPIOPinEncoderCLK    = 22;
constexpr u8 GPIOPinEncoderDAT    = 23;

bool m_PendingSingleClick = false;
u32 m_SingleClickTime = 0;

constexpr u8 ButtonMask = 1 << static_cast<u8>(TButton::Button1) |
			  1 << static_cast<u8>(TButton::Button2) |
			  1 << static_cast<u8>(TButton::EncoderButton);

CControlSimpleEncoder::CControlSimpleEncoder(TEventQueue& pEventQueue, CRotaryEncoder::TEncoderType EncoderType, bool bEncoderReversed)
	: CControl(pEventQueue),

	  m_GPIOEncoderButton(GPIOPinEncoderButton, TGPIOMode::GPIOModeInputPullUp),
	  m_GPIOButton1(GPIOPinButton1, TGPIOMode::GPIOModeInputPullUp),
	  m_GPIOButton2(GPIOPinButton2, TGPIOMode::GPIOModeInputPullUp),

	  m_Encoder(EncoderType, bEncoderReversed, GPIOPinEncoderCLK, GPIOPinEncoderDAT)
{
}

void CControlSimpleEncoder::Update()
{
	CControl::Update();

	const s8 nEncoderDelta = m_Encoder.Read();
	if (nEncoderDelta != 0)
	{
		TEvent Event;
		Event.Type = TEventType::Encoder;
		Event.Encoder.nDelta = nEncoderDelta;
		m_pEventQueue->Enqueue(Event);
	}
}

void CControlSimpleEncoder::ReadGPIOPins()
{
	const u32 nGPIOState = CGPIOPin::ReadAll();
	const u8 nButtonState = (((nGPIOState >> GPIOPinButton1) & 1) << static_cast<u8>(TButton::Button1)) |
	                        (((nGPIOState >> GPIOPinButton2) & 1) << static_cast<u8>(TButton::Button2)) |
	                        (((nGPIOState >> GPIOPinEncoderButton) & 1) << static_cast<u8>(TButton::EncoderButton));

	DebounceButtonState(nButtonState, ButtonMask);

	const bool bPressed = !(nButtonState & (1 << static_cast<u8>(TButton::EncoderButton)));
	const u32 now = CTimer::GetClockTicks();
	static bool buttonDown = false;
	
	if (bPressed && !buttonDown)
	{
		// Just pressed
		buttonDown = true;
		const u32 delta = Utility::TicksToMillis(now - m_LastEncoderClickTime);
		m_LastEncoderClickTime = now;
	
		if (m_PendingSingleClick && delta < 400) // Double click detected
		{
			m_PendingSingleClick = false;
	
			TEvent Event;
			Event.Type = TEventType::Button;
			Event.Button.Button = TButton::EncoderButton;
			Event.Button.bPressed = true;
			Event.Button.bRepeat = false;
			Event.Button.bDoubleClick = true;
			m_pEventQueue->Enqueue(Event);
		}
		else
		{
			// Possible single click — start timer
			m_PendingSingleClick = true;
			m_SingleClickTime = now;
		}
	}
	else if (!bPressed && buttonDown)
	{
		// Just released
		buttonDown = false;
	}
	
	// Timeout: if pending and no second press, send single click
	if (m_PendingSingleClick && Utility::TicksToMillis(now - m_SingleClickTime) > 400)
	{
		m_PendingSingleClick = false;
	
		TEvent Event;
		Event.Type = TEventType::Button;
		Event.Button.Button = TButton::EncoderButton;
		Event.Button.bPressed = true;
		Event.Button.bRepeat = false;
		Event.Button.bDoubleClick = false;
		m_pEventQueue->Enqueue(Event);
	}
	
	// Read encoder rotation pins
	m_Encoder.ReadGPIOPins((nGPIOState >> GPIOPinEncoderCLK) & 1,
						   (nGPIOState >> GPIOPinEncoderDAT) & 1);

}
