#ifndef UNIQUE_ANY_HPP_INCLUDED
#define UNIQUE_ANY_HPP_INCLUDED

class unique_any {
    struct contents {
        virtual ~contents() = default;
    };
    
    template<typename T>
    struct typed_contents : public contents {
        T storage;
        template<typename... Args>
        typed_contents(Args&&... args) : storage{ std::forward<Args>(args)... } {
        }
        virtual ~typed_contents() = default;
    };
    
    std::unique_ptr<contents> storage;
public:
    template<typename T, typename... Args>
    unique_any(std::in_place_type_t<T>, Args&&... args) :
        storage(std::make_unique<typed_contents<T>>(std::forward<Args>(args)...)) {
    }
    
    template<typename T>
    T* get() {
        auto ptr = dynamic_cast<typed_contents<T>*>(storage.get());
        if (ptr) {
            return &ptr->storage;
        } else {
            return nullptr;
        }
    }
};

#endif
