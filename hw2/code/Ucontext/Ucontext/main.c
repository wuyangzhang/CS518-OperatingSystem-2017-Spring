//
//  main.c
//  Ucontext
//
//  Created by Wuyang on 2/18/17.
//  Copyright © 2017 Wuyang. All rights reserved.
//

#include <stdio.h>
#include <ucontext.h>
#include <unistd.h>

int main(int argc, const char * argv[]) {
    
    ucontext_t context;
    getcontext(&context);
    puts("HelloWorld");
    sleep(1);
    setcontext(&context);
    return 0;
}
