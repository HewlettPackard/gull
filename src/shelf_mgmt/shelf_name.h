#ifndef _NVMM_SHELF_NAME_H_
#define _NVMM_SHELF_NAME_H_

//#include <sys/stat.h> // for S_IRUSR and S_IWUSR
#include <string>
#include <sstream>

#include "nvmm/shelf_id.h"

namespace nvmm {

// convert shelf id to full pathname

// right now SHELF_BASE_DIR is defined in the root CMakeLists.txt
// shelf name = prefix + "_" + shelf_id + "_" + suffix
// all shelf names share a common prefix: BASE_DIR/file_prefix. e.g. /lfs/Shelf
// suffix is optional
class ShelfName
{
public:
    ShelfName(std::string base_dir = SHELF_BASE_DIR, std::string file_prefix = "Shelf")
    { 
        prefix_ = std::string(base_dir) + "/" + std::string(SHELF_USER) + "_" + file_prefix;
    }

    ~ShelfName()
    {
    }

    // generate the full path name for the shelf, with one suffix
    std::string Path(ShelfId shelf_id, std::string suffix1="", std::string suffix2="")
    {
        std::string pathname = prefix_ + "_" +
            std::to_string(shelf_id.GetPoolId()) + "_" +
            std::to_string(shelf_id.GetShelfIndex());
        if (suffix1 != "")
        {
            pathname = pathname + "_" + suffix1;
        }
        if (suffix2 != "")
        {
            pathname = pathname + "_" + suffix2;
        }
        //assert(shelf_id == ToShelfId(pathname));
        return pathname;
    }
    
    std::string prefix_; // all shelves share a common prefix
};

} // namespace nvmm
#endif
