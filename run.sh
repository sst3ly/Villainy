while getopts "clean" opt; do
    case $opt in
        clean) rm -rf build ;;
        \?) echo "Invalid option: -$OPTARG" >&2 ; exit 1;;
    esac
done
./build.sh
echo "\n"
cd build
./villainy