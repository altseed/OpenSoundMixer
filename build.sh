rm -rf cmake

mkdir cmake

(cd cmake;
cmake -G "Unix Makefiles" ../;
make;)

