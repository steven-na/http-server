# Build then run main
run: build
    ./main

# Build main.c
build:
    gcc -Wall -Wextra -o main main.c


