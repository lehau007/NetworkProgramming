#include <stdio.h>
#include <sys/select.h>

int main() {
    fd_set fdset; 
    FD_ZERO(&fdset);
    FD_SET(3, &fdset);
    FD_SET(2, &fdset);

    printf("fds_bits[0] = %ld\n", fdset.__fds_bits[0]);

    for (int i = sizeof(fdset.__fds_bits[0])*8 - 1; i >= 0; i--) {
        printf("%ld", (fdset.__fds_bits[0] >> i) & 1);
    }
    return 1;
}
