#ifndef _control_h
#define _control_h

#include <circle/gpiopin.h>
#include <circle/types.h>
#include <circle/usertimer.h>

#include "control/rotaryencoder.h"
#include "event.h"
#include "optional.h"
#include "utility.h"

class CControl
{
public:
	CControl(TEventQueue& pEventQueue);
	virtual ~CControl() = default;

	bool Initialize();
	virtual void Update();
	u8 GetButtonState() const { return m_nButtonState; }

protected:
	virtual void ReadGPIOPins() = 0;

	void DebounceButtonState(u8 nState, u8 nMask);

	static constexpr size_t ButtonStateHistoryLength = 16;
	static constexpr size_t ButtonStateHistoryMask   = ButtonStateHistoryLength - 1;

	static constexpr u32 RepeatDelayMicros 	   = 500000;
	static constexpr u32 RepeatAccelTimeMicros = 3000000;
	static constexpr u32 MaxRepeatPeriodMicros = 100000;
	static constexpr u32 MinRepeatPeriodMicros = 20000;

	TEventQueue* m_pEventQueue;
	CUserTimer m_Timer;

	u8 m_ButtonStateHistory[ButtonStateHistoryLength];
	size_t m_nButtonStateHistoryIndex;

	u8 m_nButtonState;
	u8 m_nLastButtonState;

	TOptional<u8> m_RepeatButton;
	u32 m_PressedTime;
	u32 m_RepeatTime;

	static inline u32 RepeatPeriod(u32 nPressedDuration)
	{
		return Utility::Lerp(
			nPressedDuration,
			0,
			RepeatAccelTimeMicros,
			MaxRepeatPeriodMicros,
			MinRepeatPeriodMicros
		);
	}

	static void InterruptHandler(CUserTimer* pUserTimer, void* pParam);
};

class CControlSimpleEncoder : public CControl {
	public:
		CControlSimpleEncoder(TEventQueue& pEventQueue, CRotaryEncoder::TEncoderType EncoderType, bool bEncoderReversed);
		virtual void Update() override;
	
	protected:
		virtual void ReadGPIOPins() override;
	
		CGPIOPin m_GPIOEncoderButton;
		CGPIOPin m_GPIOButton1;
		CGPIOPin m_GPIOButton2;
		CRotaryEncoder m_Encoder;
	
		unsigned int m_LastEncoderClickTime = 0;
		bool m_EncoderWasPressed = false;
	};

class CControlSimpleButtons : public CControl
{
public:
	CControlSimpleButtons(TEventQueue& pEventQueue);

protected:
	virtual void ReadGPIOPins() override;
	unsigned int m_LastEncoderClickTime;
	u8 m_EncoderClickCount;
	bool m_EncoderWasPressed;
	CGPIOPin m_GPIOButton1;
	CGPIOPin m_GPIOButton2;
	CGPIOPin m_GPIOButton3;
	CGPIOPin m_GPIOButton4;
};

#endif