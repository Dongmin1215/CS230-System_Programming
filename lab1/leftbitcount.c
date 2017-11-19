#include <stdio.h>

int leftBitCount(int x) {
    int mask1, mask2, mask4, mask8, mask16, origX, shift16, shift8, shift4, shift2, shift1;
    int res = 0;
    mask1 = ~(1 << 31);
    mask2 = mask1 >> 1;
    mask4 = mask2 >> 2;
    mask8 = mask4 >> 4;
    mask16 = mask8 >> 8;

    origX = x;
    printf("x = %x\n",x);

    printf("x | mask16 = %x\n", x|mask16);
    shift16 = (!(~(x | mask16))) << 4;
    printf("shift16 = %x\n", shift16);
    res = res + shift16;
    printf("res = %d\n",res);
    x = (x << shift16) | mask16;
    printf("x = %x\n",x);

    printf("x | mask8 = %x\n", x|mask8);
    shift8 = (!(~(x | mask8))) << 3;
    printf("shift8 = %x\n", shift8);
    res = res + shift8;
    printf("res = %d\n",res);
    x = (x << shift8) | mask8;
    printf("x = %x\n",x);

    printf("x | mask4 = %x\n", x|mask4);
    shift4 = (!(~(x | mask4))) << 2;
    printf("shift4 = %x\n", shift4);
    res = res + shift4;
    printf("res = %d\n",res);
    x = (x << shift4) | mask4;
    printf("x = %x\n",x);

    printf("x | mask2 = %x\n", x|mask2);
    shift2 = (!(~(x | mask2))) << 1;
    printf("shift2 = %x\n", shift2);
    res = res + shift2;
    printf("res = %d\n",res);
    x = (x << shift2) | mask2;
    printf("x = %x\n",x);

    printf("x | mask1 = %x\n", x|mask1);
    shift1 = (!(~(x | mask1)));
    printf("shift1 = %x\n", shift1);
    res = res + shift1;
    printf("res = %d\n",res);
    x = (x << shift1) | mask1;
    printf("x = %x\n",x);

    res = res + !(~x);
    printf("final result = %d\n",res);
    return res;
}

int main(){
    leftBitCount(0xfff00f0f);
    printf("\n\n");
    leftBitCount(0xfffff000);
}

