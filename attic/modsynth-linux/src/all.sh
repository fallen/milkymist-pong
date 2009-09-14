./a.out $1 > /tmp/o 2>&1
mv output.{raw,s16}
./sox.sh output.s16
aplay output.wav
