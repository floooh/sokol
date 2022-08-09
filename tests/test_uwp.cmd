if not exist ext/fips-cimgui/ (
    git clone --depth 1 --recursive https://github.com/fips-libs/fips-cimgui ext/fips-cimgui
)

md build\win_uwp_debug
cd build\win_uwp_debug
cmake -DSOKOL_BACKEND=SOKOL_D3D11 -DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=10.0.19041.0 ../.. || exit /b 10
cmake --build . || exit /b 10
cd ..\..

md build\win_uwp_release
cd build\win_uwp_release
cmake -DSOKOL_BACKEND=SOKOL_D3D11 -DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=10.0.19041.0 ../.. || exit /b 10
cmake --build . --config Release || exit /b 10
cd ..\..


