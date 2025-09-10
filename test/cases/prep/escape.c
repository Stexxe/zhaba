#define SOME (3 + \
    4)

void func(char *fmt, ...);

int main() {
    func("%d", SOME);
}
