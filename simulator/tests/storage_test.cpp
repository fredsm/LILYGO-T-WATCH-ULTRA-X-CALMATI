#include "mock/storage.hpp"
#include <cstdio>
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"line %d: %s\n",__LINE__,#x); return 1; } } while(0)
int main() {
    using R=sim::StorageResult;
    sim::Media media;
    sim::Storage sd(media);
    sd.capacity=5;
    CHECK(sd.write("/a","12345")==R::ok);
    CHECK(sd.write("/b","x")==R::full);
    CHECK(sd.write("/a","12")==R::ok);
    CHECK(sd.write("/b","345")==R::ok);
    CHECK(sd.write("/a","123")==R::full);
    std::string out;
    CHECK(sd.read("/a",out)==R::ok && out=="12");
    CHECK(sd.write("/../x","")==R::invalid_path);
    CHECK(sd.write("/a//b","")==R::invalid_path);
    sd.usb_owned=true;
    CHECK(sd.read("/a",out)==R::unavailable);
    CHECK(sd.remove("/a")==R::unavailable);
    sd.usb_owned=false; sd.read_only=true;
    CHECK(sd.write("/a","")==R::read_only);
    CHECK(sd.read("/a",out)==R::ok);
    sd.inserted=false;
    CHECK(sd.read("/a",out)==R::unavailable);
    CHECK(sd.put("settings","face","digital"));
    CHECK(sd.put("other","face","analog"));
    sd.nvs_available=false;
    CHECK(!sd.get("settings","face"));
    CHECK(!sd.put("settings","face","bad"));
    sim::Storage reboot(media);
    CHECK(reboot.get("settings","face").value_or("")=="digital");
    CHECK(reboot.get("other","face").value_or("")=="analog");
    CHECK(reboot.read("/b",out)==R::ok && out=="345");
    CHECK(reboot.remove("/b")==R::ok);
    CHECK(reboot.read("/b",out)==R::not_found);
    return 0;
}
