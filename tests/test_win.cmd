cmake --preset win_gl || exit /b 10
cmake --build --preset win_gl_debug || exit /b 10
cmake --build --preset win_gl_release || exit /b 10

cmake --preset win_d3d11 || exit /b 10
cmake --build --preset win_d3d11_debug || exit /b 10
cmake --build --preset win_d3d11_release || exit /b 10

cd build\win_d3d11\Debug
sokol-test.exe || exit /b 10
cd ..\..\..
