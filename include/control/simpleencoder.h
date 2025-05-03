#pragma once

#include "control.h"
#include "button.h"           // for TButton
#include "rotaryencoder.h"    // for CRotaryEncoder
#include <cstdint>

// you’ll need these forward‑declares if your control.h doesn’t already pull them in:
class CGPIOPin;

class CControlSimpleEncoder : public CControl
{
public:
    CControlSimpleEncoder(TEventQueue& pEventQueue,
                          CRotaryEncoder::TEncoderType EncoderType,
                          bool bEncoderReversed);

    void Update() override;
    void ReadGPIOPins() override;

private:
    CGPIOPin       m_GPIOEncoderButton;
    CGPIOPin       m_GPIOButton1;
    CGPIOPin       m_GPIOButton2;
    CRotaryEncoder m_Encoder;

    //–– click‑detection state ––
    bool  m_PendingSingleClick;
    u32   m_SingleClickTime;
    u32   m_LastEncoderClickTime;
};
