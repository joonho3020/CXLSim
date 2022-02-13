
# CXL Standalone Simulator

# Quick Start
```
git clone <url>
git submodule init
git submodule update
cd scripts
./statgen.pl
./knobgen.pl
cd ..
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=../bin
make -j <cores>
make install
```


# TODO
- ~~knobs : how am I going to generate knobs for standalone without conflict when integrating it with macsim?~~
- Limit RX request pull BW to add RX back pressure

