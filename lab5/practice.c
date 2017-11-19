#include <stdio.h>

int main(){

    int x = 1;
    int y1 = ~(1<<31);
    int y2 = y1>>1;
    int y4 = y2>>2;
    int y8 = y4>>4;
    int y16 = y8>>8;
    printf("y1 = %x \n", y1);
    printf("y2 = %x \n", y2);
    printf("y4 = %x \n", y4);
    printf("y8 = %x \n", y8);
    printf("y16 = %x \n", y16);
    printf("x>>31 = %x  \n", x>>31);
    printf("x<<16 = %x \n", x<<16);
    printf("x<< 8= %x \n", x<<8);
    printf("x>>8= %x \n", x>>8);

}
