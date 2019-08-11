//
// ProgressBar
// MQSim
//
// Created by 文盛業 on 2019-08-11.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__Progress__
#define __MQSim__Progress__

#include <iostream>
#include <string>

#include "InlineTools.h"

namespace Utils {
  class ProgressBar {
  private:
    constexpr static int STEP = 5;
    constexpr static int BAR_WIDTH = 100;
    constexpr static int BAR_SIZE = BAR_WIDTH / STEP + 3;

  private:
    const std::string __name;

    int __announcing_at;

  public:
    explicit ProgressBar(std::string name);

    void update(int progress);
  };

  force_inline
  ProgressBar::ProgressBar(std::string name)
    : __name(std::move(name)),
      __announcing_at(0)
  { }

  force_inline void
  ProgressBar::update(int progress)
  {
    if (progress >= __announcing_at) {
      char bar[BAR_SIZE];
      char* pos = bar;

      *pos = '[';

      for (int i = 0; i < BAR_WIDTH; i += STEP) {
        if (i < progress) *(++pos) = '=';
        else if (i == progress) *(++pos) = '>';
        else *(++pos) = ' ';
      }

      *(++pos) = ']';
      *(++pos) = '\0';

      std::cout << bar << " "
                << progress << "% progress in " << __name << std::endl;
      __announcing_at += STEP;
    }
  }
}

#endif /* Predefined include guard __MQSim__Progress__ */
