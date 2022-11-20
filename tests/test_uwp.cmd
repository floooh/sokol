cmake --preset win_uwp || exit /b 10
cmake --build --preset win_uwp_debug || exit /b 10
cmake --build --preset win_uwp_release || exit /b 10
