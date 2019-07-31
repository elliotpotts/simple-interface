#include <si/wl/buffer.hpp>

void wl::buffer_deleter::operator()(wl_buffer* b) const {
    wl_buffer_destroy(b);
}

wl::buffer::buffer(wl_buffer* buffer) : hnd(buffer) {
    if (!hnd) {
        throw std::runtime_error("Can't create buffer from nullptr");
    }
}

wl::buffer::operator wl_buffer*() const {
    return hnd.get();
}
