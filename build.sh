# build villainy
mkdir -p build
cmake -S . -B build
cd build
make
# build shaders
mkdir -p shaders
for f in ../src/shaders/*.vert ../src/shaders/*.frag; do
if [ -f "$f" ]; then
    glslc "$f" -o "shaders/$(basename "$f").spv"
fi
done