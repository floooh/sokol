cmake --preset win_gl_analyze || exit /b 10
cmake --build --preset win_gl_analyze || exit /b 10
cmake --preset win_d3d11_analyze || exit /b 10
cmake --build --preset win_d3d11_analyze || exit /b 10
