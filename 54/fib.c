#include <stdio.h>
#include <stdlib.h>

long long fib(int n) {
    if (n <= 1)
        return n;
    return fib(n-1) + fib(n-2);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    printf("Fibonacci number %d is %lld\n", n, fib(n));
    return 0;
}
