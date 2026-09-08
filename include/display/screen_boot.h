// Power-on wake-up choreography (eyes opening, gaze, blink, greeting).
#pragma once

namespace ScreenBoot
{
  void begin();    // start the boot clock
  bool render();   // draw one frame; returns true while still animating
  bool complete(); // true once the wake-up sequence has finished
}
