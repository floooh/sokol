@echo off
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

rem D3D11 backend against the mock d3d11.h in tests/mocks/d3d11
cmake --preset win_d3d11_mock || exit /b 10
cmake --build --preset win_d3d11_mock_debug || exit /b 10

rem GL 4.x and GLES 3.x backends against the mock GL library in tests/mocks/gl
cmake --preset win_glcore_mock || exit /b 10
cmake --build --preset win_glcore_mock_debug || exit /b 10

cmake --preset win_gles3_mock || exit /b 10
cmake --build --preset win_gles3_mock_debug || exit /b 10

call :runtest win_d3d11 sokol-test || exit /b 10

call :runtest win_d3d11_mock sokol-d3d11-test || exit /b 10

call :runtest win_glcore_mock sokol-gl41-test || exit /b 10
call :runtest win_glcore_mock sokol-gl43-test || exit /b 10

call :runtest win_gles3_mock sokol-gles30-test || exit /b 10
call :runtest win_gles3_mock sokol-gles31-test || exit /b 10
call :runtest win_gles3_mock sokol-gles32-test || exit /b 10

goto :eof

rem runtest <preset> <binary> -- mirrors the runtest function in test_common.sh
:runtest
echo ===============================================================
echo === RUNNING: %2 [%1]
echo ===============================================================
cd build\%1\Debug
%2.exe || exit /b 10
cd ..\..\..
goto :eof
