#pragma once

#define test(T) do {                                                    \
    bool __res = (T);                                                   \
    std::cout << (__res ? "[PASS] " : "[FAIL] ")                        \
              << __FILE__ << ":" << __LINE__ << ": " << #T << std::endl; \
    global_res &= __res;                                                \
    if (not __res and exit_on_fail) exit (1);                           \
  } while (0)

static bool global_res = true;
static bool exit_on_fail = false;
