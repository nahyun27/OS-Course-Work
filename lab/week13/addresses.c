#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <virtual_address>\n", argv[0]);
        return 1;
    }

    unsigned int address = atoi(argv[1]);
    unsigned int page_size = 4096; // 4KB 페이지 크기
    unsigned int page_number = address / page_size;
    unsigned int offset = address % page_size;

    printf("The address %u contains:\n", address);
    printf("page number = %u\n", page_number);
    printf("offset = %u\n", offset);

    return 0;
}
