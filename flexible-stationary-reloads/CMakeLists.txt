include($ENV{HELLFORGE}/app/sdk/cmake/HellForgeMod.cmake)
hellforge_add_mod(flexible-stationary-reloads
        "src/main.cpp"
        "src/patches.cpp"
        "src/diagnostics.cpp"
        "src/config.cpp")
