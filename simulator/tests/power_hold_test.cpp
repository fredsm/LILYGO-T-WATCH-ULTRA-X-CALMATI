#include "../../src/power_hold.h"
#include <cstdio>
#define CHECK(expr) do { if (!(expr)) { std::fprintf(stderr,"failure at %d: %s\n", __LINE__, #expr); return 1; } } while (0)
int main() {
    PowerHold hold;
    CHECK(!hold.release(1));
    hold.press(10); CHECK(!hold.update(5009)); CHECK(!hold.release(5009));
    CHECK(!hold.armed());
    hold.press(10000); hold.press(10100); // repeated down cannot restart timer
    CHECK(!hold.update(14999)); CHECK(hold.update(15000));
    CHECK(!hold.update(15001)); CHECK(hold.release(15100)); CHECK(!hold.down());
    hold.reset(); hold.press(0xfffffff0u); CHECK(hold.update(4984));
    CHECK(hold.release(4985)); hold.reset();
    hold.press(100); CHECK(hold.release(5100)); // release before next tick
    return 0;
}
