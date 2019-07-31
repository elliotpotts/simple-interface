#ifndef SI_SHM_BACKED_BUFFER_HPP_INCLUDED
#define SI_SHM_BACKED_BUFFER_HPP_INCLUDED

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <si/wl/shm_pool.hpp>
#include <si/wl/shm.hpp>
#include <si/wl/buffer.hpp>

inline constexpr int bytes_per_pixel = 4;
inline constexpr int win_width = 480;
inline constexpr int win_height = 360;

class shm_backed_buffer {
    static boost::interprocess::shared_memory_object make_shm_obj(int width, int height);
public:
    boost::interprocess::shared_memory_object shm_obj;
    boost::interprocess::mapped_region pixels;
    wl::shm_pool pool;
    wl::buffer buffer;
    shm_backed_buffer(wl::shm&, int width, int height);
};

#endif
