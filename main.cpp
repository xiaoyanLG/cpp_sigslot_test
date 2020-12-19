#include <stdio.h>
#include "sigslot.h"

#define CALL_FUNC_LOG printf("func: %s, line: %d\n", __FUNCTION__, __LINE__)

class Switch
{
public:
    sigslot::signal0<> Clicked;
};

class Light : public sigslot::has_slots<>
{
public:
    void ToggleState()
    {
        CALL_FUNC_LOG;
    }
    void TurnOn()
    {
        CALL_FUNC_LOG;
    }
    void TurnOff()
    {
        CALL_FUNC_LOG;
    }
};

int main()
{
    printf("sigslot.h usage show:\n");
    Switch sw3, sw4, all_on, all_off;
    Light lp1, lp2, lp3, lp4;
    sw3.Clicked.connect(&lp3, &Light::ToggleState);
    sw4.Clicked.connect(&lp4, &Light::ToggleState);
    all_on.Clicked.connect(&lp1, &Light::TurnOn);
    all_on.Clicked.connect(&lp2, &Light::TurnOn);
    all_on.Clicked.connect(&lp3, &Light::TurnOn);
    all_on.Clicked.connect(&lp4, &Light::TurnOn);
    all_off.Clicked.connect(&lp1, &Light::TurnOff);
    all_off.Clicked.connect(&lp2, &Light::TurnOff);
    all_off.Clicked.connect(&lp3, &Light::TurnOff);
    all_off.Clicked.connect(&lp4, &Light::TurnOff);

    sw3.Clicked();
    all_on.Clicked.emit();
    all_off.Clicked();
    return 0;
}
