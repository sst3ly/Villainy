for arg in "$@"; do
    case $arg in
        --clean|-c)
            CLEAN=true
            ;;
    esac
done

if [ "$CLEAN" = true ]; then
    rm -rf build/
fi

./build.sh
echo "\n"
cd build
./villainy