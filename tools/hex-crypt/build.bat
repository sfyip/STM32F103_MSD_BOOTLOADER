gcc -c -o crypt.o -O3 crypt.c
gcc -c -o aes.o -O3 aes.c
gcc -c -o ihex_parser.o -O3 ihex_parser.c
g++ -o hex_crypt -O3 hex_crypt.cpp crypt.o aes.o ihex_parser.o