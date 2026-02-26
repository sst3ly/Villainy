if [ ! -d "build" ]; then
    echo "Please build the project first by running ./build.sh"
    exit 1
fi

rm -rf target
mkdir -p target/lib
mkdir -p target/include

cp build/libVillainyLib.a target/lib
cp src/villainy/*.hpp target/include
cp src/villainy/*.ipp target/include
zip -r Villainy.zip target