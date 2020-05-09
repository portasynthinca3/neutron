typedef long unsigned int uint64_t;

void do_syscall(uint64_t num, uint64_t p0){
    asm("int $0x80" : :
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
    syscall_stop();
}
