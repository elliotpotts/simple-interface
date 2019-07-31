#ifndef WL_BUFFER_HPP_INCLUDED
#define WL_BUFFER_HPP_INCLUDED

#include <wayland-client.h>
#include <memory>

namespace wl {
    struct buffer_deleter {
        void operator()(wl_buffer* b) const;
    };
    class buffer {
        std::unique_ptr<wl_buffer, buffer_deleter> const hnd;
    public:
        explicit buffer(wl_buffer*);
        buffer(const buffer&) = delete;
        explicit operator wl_buffer*() const;
    };
}

#endif
