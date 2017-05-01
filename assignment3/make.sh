cd example
fusermount -u mountdir
cd ..
./configure
make
touch disk-zz
cd example
../src/sfs ../disk-zz mountdir
