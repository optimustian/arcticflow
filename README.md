# arcticflow
A task scheduling language compiler

### How to build

For Ubuntu 22.04:
```bash
sudo apt install cmake flex bison llvm zlib1g-dev zlib1g -y

mkdir build && cd build
cmake ..
make -j4
```