#include <si/shm_buffer.hpp>

namespace ipc = boost::interprocess;

ipc::shared_memory_object shm_buffer::make_shm_obj(int width, int height) {
    ipc::shared_memory_object shm_obj {
        ipc::open_or_create,
        "foobar",
        ipc::read_write
    };
    shm_obj.truncate(width * height * bytes_per_pixel);
    return shm_obj;
}
shm_buffer::shm_buffer(wl::shm& shm_iface, int width, int height) :
    shm_obj{make_shm_obj(width, height)},
    pixels{shm_obj, ipc::read_write},
    pool{shm_iface.make_pool(shm_obj.get_mapping_handle().handle, pixels.get_size())},
    buffer(pool.make_buffer(0, width, height, width * bytes_per_pixel, WL_SHM_FORMAT_ARGB8888))
    {
}
