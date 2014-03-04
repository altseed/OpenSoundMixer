rm -rf cmake

mkdir cmake

(cd cmake;
cmake -G "Unix Makefiles" ../;
make;)

cp cmake/OpenSoundMixerTest bin/
