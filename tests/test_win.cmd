if not exist ext/fips-cimgui/ (
    git clone --depth 1 --recursive https://github.com/fips-libs/fips-cimgui ext/fips-cimgui
)

md build\win_gl_debug
cd build\win_gl_debug
cmake -DSOKOL_BACKEND=SOKOL_GLCORE33 ../.. || exit /b 10
cmake --build . || exit /b 10
cd ..\..

md build\win_gl_release
cd build\win_gl_release
cmake -DSOKOL_BACKEND=SOKOL_GLCORE33 ../.. || exit /b 10
cmake --build . --config Release || exit /b 10
cd ..\..

md build\win_d3d11_debug
cd build\win_d3d11_debug
cmake -DSOKOL_BACKEND=SOKOL_D3D11 ../.. || exit /b 10
cmake --build . || exit /b 10
cd ..\..

md build\win_d3d11_debug
cd build\win_d3d11_debug
cmake -DSOKOL_BACKEND=SOKOL_D3D11 ../.. || exit /b 10
cmake --build . --config Release || exit /b 10
cd ..\..

cd build\win_d3d11_debug\Debug
sokol-test.exe || exit /b 10
cd ..\..\..
