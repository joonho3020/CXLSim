
# CXL Standalone simulator

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
- add simbase
- knobs : how am I going to generate knobs for standalone without conflict when integrating it with macsim?
- what am I going to use as mem\_req\_info ?
- trace generation
