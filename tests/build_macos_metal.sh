sh build_prepare.sh
mkdir -p build/macos_metal && cd build/macos_metal
cmake -G Ninja -D SOKOL_BACKEND=SOKOL_METAL ../..
cmake --build .
cd ..