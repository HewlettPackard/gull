#ifndef _NVMM_SHELF_FILE_H_
#define _NVMM_SHELF_FILE_H_

#include <unistd.h>
#include <string>

#include "nvmm/error_code.h"
#include "nvmm/shelf_id.h"

namespace nvmm {

// a generic abstraction of shelf files on tmpfs
class ShelfFile
{
public:
    ShelfFile() = delete; // no default    
    ShelfFile(std::string pathname);

    // shelf_id is needed to register/unregister a shelf with the ShelfManager
    // TODO: have two kinds of ShelfFile? public (registered) and private (unregistered)?
    ShelfFile(std::string pathname, ShelfId shelf_id); 
    ~ShelfFile();        

    bool IsOpen()
    {
        return fd_ != -1;
    }
    
    std::string GetPath()
    {
        return path_;
    }

    ShelfId GetShelfId()
    {
        return shelf_id_;
    }
    
    // file must not be opened
    ErrorCode Create(mode_t mode, size_t size=0);
    ErrorCode Destroy();
    ErrorCode Open(int flags);    

    // file must be opened
    ErrorCode Close();
    // map a specific range 
    ErrorCode Map(void *addr_hint, size_t length, int prot, int flags, loff_t offset, void **mapped_addr, bool register_fam_atomic=true);
    // map the entire file and register itself with ShelfManager
    // requires a valid shelf_id_
    ErrorCode Map(void *addr_hint, void **mapped_addr);

    // work in both cases (opened or not opened)
    // unmap a specific range (used by the memory manager)
    static ErrorCode Unmap(void *mapped_addr, size_t length, bool unoregister_fam_atomic=true);

    // unmap the entire file and unregister itself with ShelfManager
    // requires a valid shelf_id_
    ErrorCode Unmap(void *mapped_addr, bool unregister);
    ErrorCode Truncate(off_t length);
    ErrorCode Rename(char const *new_pathname);
    bool Exist();
    size_t Size();        
    
private:
    int fd_;
    std::string path_;
    ShelfId shelf_id_;

    //DEBUG
    void *addr_;
    size_t length_;
    
};

} // namespace nvmm
#endif
