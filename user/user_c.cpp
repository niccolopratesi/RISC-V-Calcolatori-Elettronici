char c = 'c';

extern "C" void func();

int funzione() {
    func();
    return 1;
}

int __attribute__((constructor)) funzione2() {
    return 2;
}

extern "C" void __attribute__((section(".main"))) main() {
    int a;
    int b = 13;
    c = '\0';
    a = funzione();
    func();
}

