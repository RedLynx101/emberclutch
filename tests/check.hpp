// Tiny test harness shared by the PC unit tests.
#pragma once

#include <cstdio>

extern int g_failures;
extern int g_checks;

#define CHECK(cond)                                                       \
    do {                                                                  \
        ++g_checks;                                                       \
        if (!(cond)) {                                                    \
            ++g_failures;                                                 \
            std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                 \
    } while (0)

#define TEST(name) static void name()
#define RUN(name)                   \
    do {                            \
        std::printf("%s\n", #name); \
        name();                     \
    } while (0)

void runModelTests();  // tests/test_model.cpp
void runAnimTests();   // tests/test_anim.cpp
void runBehaviorTests();  // tests/test_behavior.cpp
void runDenTests();       // tests/test_den.cpp
void runEggTests();       // tests/test_egg.cpp
void runCareTests();      // tests/test_care.cpp
void runValleyTests();    // tests/test_valley.cpp
void runKindTests();      // tests/test_kinds.cpp
void runWorldTests();     // tests/test_world.cpp
void runChallengeTests(); // tests/test_challenges.cpp
void runTrainerTests();   // tests/test_trainer.cpp
void runPlaceTests();     // tests/test_places.cpp
void runInterfaceTests(); // tests/test_interface.cpp (1.0 interface: the tracked goal, the tips)
