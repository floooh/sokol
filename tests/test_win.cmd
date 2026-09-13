rem LLM maintained.
cmake --preset win_vk || exit /b 10
cmake --build --preset win_vk_debug || exit /b 10
cmake --build --preset win_vk_release || exit /b 10

cmake --preset win_gl || exit /b 10
cmake --build --preset win_gl_debug || exit /b 10
cmake --build --preset win_gl_release || exit /b 10

cmake --preset win_d3d11 || exit /b 10
cmake --build --preset win_d3d11_debug || exit /b 10
cmake --build --preset win_d3d11_release || exit /b 10

rem GL 4.x and GLES 3.x backends against the mock GL library in tests/mocks/gl
cmake --preset win_glcore_mock || exit /b 10
cmake --build --preset win_glcore_mock_debug || exit /b 10
cmake --build --preset win_glcore_mock_release || exit /b 10

cmake --preset win_gles3_mock || exit /b 10
cmake --build --preset win_gles3_mock_debug || exit /b 10
cmake --build --preset win_gles3_mock_release || exit /b 10

cd build\win_d3d11\Debug
sokol-test.exe || exit /b 10
cd ..\..\..

cd build\win_glcore_mock\Debug
sokol-gl41-test.exe || exit /b 10
sokol-gl43-test.exe || exit /b 10
cd ..\..\..

cd build\win_gles3_mock\Debug
sokol-gles30-test.exe || exit /b 10
sokol-gles31-test.exe || exit /b 10
sokol-gles32-test.exe || exit /b 10
cd ..\..\..
