typedef long unsigned int uint64_t;

void do_syscall(uint64_t num, uint64_t p0){
    asm("syscall" : :
        "rdi"(num), "rsi"(p0) : "rdi", "rsi");
}

void syscall_print(char* str){
    do_syscall(0, (uint64_t)str);
}

void syscall_stop(void){
    do_syscall(1, 0);
}

void syscall_run(char* path){
    do_syscall(2, (uint64_t)path);
}

void main(void* args){
    syscall_print("Hello, World (from test2.elf)!");
    
    syscall_print("Let's try to acces a kernel memory location (0xFFFF800000000000)");
    volatile uint64_t malicious_af = *(uint64_t*)0xFFFF800000000000ULL;
    
    syscall_print("Let's try to divide by 0");
    volatile int a = 123 / 0;
    
    syscall_stop();
}
