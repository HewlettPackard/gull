#ifndef _NVMM_ROOT_SHELF_H_
#define _NVMM_ROOT_SHELF_H_

#include <string>

#include "nvmm/error_code.h"

namespace nvmm {

class RootShelf
{
public:
    RootShelf() = delete; // no default    
    RootShelf(std::string pathname);
    ~RootShelf();

    ErrorCode Create();
    ErrorCode Destroy();
    ErrorCode Open();        
    ErrorCode Close();
    bool Exist();
    bool IsOpen();
    void *Addr();


private:
    static uint64_t const kMagicNum = 766874353; // root shelf
    static size_t const kShelfSize = 128*1024*1024; // 128MB
    std::string path_;
    int fd_;
    void *addr_;
};

} // namespace nvmm
        
#endif
