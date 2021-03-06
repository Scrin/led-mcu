/////////////////////////////////////////////////
// Generic light effects for some fancy stuffs //
/////////////////////////////////////////////////

#include "common.h"

long long effectCounter = 0;
int effectTimerID = -1;

const uint8_t lights[360] = {0, 0, 0, 0, 0, 1, 1, 2, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13, 15, 17, 18, 20, 22, 24, 26, 28, 30, 32, 35, 37, 39, 42, 44, 47, 49, 52, 55, 58, 60, 63, 66, 69, 72, 75, 78, 81, 85, 88, 91, 94, 97, 101, 104, 107, 111, 114, 117, 121, 124, 127, 131, 134, 137, 141, 144, 147, 150, 154, 157, 160, 163, 167, 170, 173, 176, 179, 182, 185, 188, 191, 194, 197, 200, 202, 205, 208, 210, 213, 215, 217, 220, 222, 224, 226, 229, 231, 232, 234, 236, 238, 239, 241, 242, 244, 245, 246, 248, 249, 250, 251, 251, 252, 253, 253, 254, 254, 255, 255, 255, 255, 255, 255, 255, 254, 254, 253, 253, 252, 251, 251, 250, 249, 248, 246, 245, 244, 242, 241, 239, 238, 236, 234, 232, 231, 229, 226, 224, 222, 220, 217, 215, 213, 210, 208, 205, 202, 200, 197, 194, 191, 188, 185, 182, 179, 176, 173, 170, 167, 163, 160, 157, 154, 150, 147, 144, 141, 137, 134, 131, 127, 124, 121, 117, 114, 111, 107, 104, 101, 97, 94, 91, 88, 85, 81, 78, 75, 72, 69, 66, 63, 60, 58, 55, 52, 49, 47, 44, 42, 39, 37, 35, 32, 30, 28, 26, 24, 22, 20, 18, 17, 15, 13, 12, 11, 9, 8, 7, 6, 5, 4, 3, 2, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void gradient()
{
    float x, stepSize = 1.f / (gradientExtent / 100.f) / NUM_LEDS;
    switch (gradientMode)
    {
    case 'N':
        x = 1.f;
        for (int i = 0; i < NUM_LEDS; i++)
        {
            x = max(x - stepSize, 0.f);
            stripLeds[i] = RgbwColor(red * x, green * x, blue * x, white * x);
        }
        break;
    case 'F':
        x = 1.f;
        for (int i = NUM_LEDS - 1; i >= 0; i--)
        {
            x = max(x - stepSize, 0.f);
            stripLeds[i] = RgbwColor(red * x, green * x, blue * x, white * x);
        }
        break;
        break;
    case 'C':
        x = 1.f;
        for (int i = NUM_LEDS / 2; i < NUM_LEDS; i++)
        {
            x = max(x - stepSize, 0.f);
            stripLeds[i] = RgbwColor(red * x, green * x, blue * x, white * x);
        }
        x = 1.f;
        for (int i = NUM_LEDS / 2 - 1; i >= 0; i--)
        {
            x = max(x - stepSize, 0.f);
            stripLeds[i] = RgbwColor(red * x, green * x, blue * x, white * x);
        }
        break;
    case 'E':
        x = 1.f;
        for (int i = 0; i < NUM_LEDS / 2; i++)
        {
            x = max(x - stepSize, 0.f);
            stripLeds[i] = RgbwColor(red * x, green * x, blue * x, white * x);
        }
        x = 1.f;
        for (int i = NUM_LEDS - 1; i >= NUM_LEDS / 2; i--)
        {
            x = max(x - stepSize, 0.f);
            stripLeds[i] = RgbwColor(red * x, green * x, blue * x, white * x);
        }
        break;
    }
}

void runEffect()
{
    switch (effect)
    {
    case eStable:
        for (int i = 0; i < NUM_LEDS; i++)
        {
            stripLeds[i] = RgbwColor(red, green, blue, white);
        }
        break;
    case eGradient:
        gradient();
        break;
    case eCustom:
        for (int i = 0; i < NUM_LEDS; i++)
        {
            stripLeds[i] = customLeds[i];
        }
        break;
    case eSunrise:
        effectTimerID = timer.setTimeout(10, runEffect);
        sunrise();
        break;
    case eColorLoop:
        effectCounter++;
        effectTimerID = timer.setTimeout(10, runEffect);
        for (int i = 0; i < NUM_LEDS; i++)
        {
            int angle = (effectCounter / 10 + i) % 360;
            stripLeds[i] = RgbwColor(lights[(angle + 120) % 360], lights[angle], lights[(angle + 240) % 360], 0);
        }
        break;
    default:
        Serial.println("Unknown effect?");
    }
}

void startEffect(Effect e)
{
    stopEffect();
    effect = e;
    effectCounter = 0;
    if (effect == eSunrise)
    {
        // Sunrise is its own self-contained spaghetti with its own timers that need to be started as well
        startSunrise(sunriseDuration);
    }
    runEffect();
}

void stopEffect()
{
    if (effectTimerID != -1)
    {
        timer.deleteTimer(effectTimerID);
    }
    effectTimerID = -1;
}
