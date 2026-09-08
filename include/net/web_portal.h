// HTTP dashboard + captive-portal handlers.
#pragma once

#include <Arduino.h>

namespace WebPortal
{
  void start();
  void handle();
  bool started();
}
