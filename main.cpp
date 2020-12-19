#include <stdio.h>

#define CALL_FUNC_LOG printf("func: %s, line: %d\n", __FUNCTION__, __LINE__);

#include "sigslot.h"

class Switch
{
public:
    sigslot::signal0<> Clicked;
    sigslot::signal1<int> Clicked1;
    sigslot::signal2<int, int> Clicked2;
    sigslot::signal3<int, int, int> Clicked3;
};

class Light : public sigslot::has_slots<>
{
public:
    void ToggleState()
    {
        CALL_FUNC_LOG;
    }
    void ToggleState1(int)
    {
        CALL_FUNC_LOG;
    }
    void ToggleState2(int, int)
    {
        CALL_FUNC_LOG;
    }
    void ToggleState3(int, int, int)
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
    Switch sw3, all_on, all_off;
    Light lp1, lp2, lp3, lp4;
//    Switch sw3, all_on, all_off;

    sw3.Clicked.connect(&lp3, &Light::ToggleState);
    sw3.Clicked1.connect(&lp3, &Light::ToggleState1);
    sw3.Clicked2.connect(&lp3, &Light::ToggleState2);
    sw3.Clicked3.connect(&lp3, &Light::ToggleState3);
    all_on.Clicked.connect(&lp1, &Light::TurnOn);
    all_on.Clicked.connect(&lp2, &Light::TurnOn);
    all_on.Clicked.connect(&lp3, &Light::TurnOn);
    all_on.Clicked.connect(&lp4, &Light::TurnOn);
    all_off.Clicked.connect(&lp1, &Light::TurnOff);
    all_off.Clicked.connect(&lp2, &Light::TurnOff);
    all_off.Clicked.connect(&lp3, &Light::TurnOff);
    all_off.Clicked.connect(&lp4, &Light::TurnOff);

    sw3.Clicked.disconnect(&lp3);
    all_on.Clicked.disconnect();
    all_off.Clicked.disconnect(&lp1);

    sw3.Clicked();
    sw3.Clicked1(1);
    sw3.Clicked2(1,2);
    sw3.Clicked3(1,2,3);
    all_on.Clicked.emit();
    all_off.Clicked();
    return 0;
}
