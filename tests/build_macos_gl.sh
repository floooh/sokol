sh build_prepare.sh
mkdir -p build/macos_gl && cd build/macos_gl
cmake -G Ninja -D SOKOL_BACKEND=SOKOL_GLCORE33 ../..
cmake --build .
cd ..
