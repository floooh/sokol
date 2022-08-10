if not exist ext/fips-cimgui/ (
    git clone --depth 1 --recursive https://github.com/fips-libs/fips-cimgui ext/fips-cimgui
)

md build\win_gl_analyze
cd build\win_gl_analyze
cmake -GNinja -DSOKOL_BACKEND=SOKOL_GLCORE33 -DUSE_ANALYZER=ON -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ ../.. || exit /b 10
cmake --build . || exit /b 10
cd ..\..

md build\win_d3d11_analyze
cd build\win_d3d11_analyze
cmake -GNinja -DSOKOL_BACKEND=SOKOL_D3D11 -DUSE_ANALYZER=ON -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ ../.. || exit /b 10
cmake --build . || exit /b 10
cd ..\..

