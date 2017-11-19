#include <stdio.h>
long absdiff(long x, long y){
    long result;
    if (x>y)
        result = x-y;
    else
        result = y-x;
    return result;
}
int main(){
    printf("%d, %x\n",355,355);
    printf("%d, %x\n",364,364);
    printf("%d, %x\n",373,373);
    printf("%d, %x\n",355,355);

    printf("6:%d\n",0x336);
    printf("5:%d\n",0xaa);
    printf("4:%d\n",0x249);
    printf("3:%d\n",0xd8);
    printf("2:%d\n",0xed);
    printf("1:%d\n",0xf0);
    printf("%x",0x7fffffffe4e8+0x14);
}


