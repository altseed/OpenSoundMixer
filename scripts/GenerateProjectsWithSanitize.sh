cd `dirname $0`

mkdir ../build

cd ../build

cmake -D BUILD_TEST=ON -D SANITIZE_ENABLED=ON ..
