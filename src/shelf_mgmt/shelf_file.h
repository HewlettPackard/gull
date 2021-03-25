/*
 *  (c) Copyright 2016-2021 Hewlett Packard Enterprise Development Company LP.
 *
 *  This software is available to you under a choice of one of two
 *  licenses. You may choose to be licensed under the terms of the 
 *  GNU Lesser General Public License Version 3, or (at your option)  
 *  later with exceptions included below, or under the terms of the  
 *  MIT license (Expat) available in COPYING file in the source tree.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  As an exception, the copyright holders of this Library grant you permission
 *  to (i) compile an Application with the Library, and (ii) distribute the
 *  Application containing code generated by the Library and added to the
 *  Application during this compilation process under terms of your choice,
 *  provided you also meet the terms and conditions of the Application license.
 *
 */

#ifndef _NVMM_SHELF_FILE_H_
#define _NVMM_SHELF_FILE_H_

#include <string>
#include <unistd.h>

#include "nvmm/error_code.h"
#include "nvmm/shelf_id.h"

namespace nvmm {

// a generic abstraction of shelf files on tmpfs
class ShelfFile {
  public:
    ShelfFile() = delete; // no default
    ShelfFile(std::string pathname);

    // shelf_id is needed to register/unregister a shelf with the ShelfManager
    // TODO: have two kinds of ShelfFile? public (registered) and private
    // (unregistered)?
    ShelfFile(std::string pathname, ShelfId shelf_id);
    ~ShelfFile();

    bool IsOpen() { return fd_ != -1; }

    std::string GetPath() { return path_; }

    ShelfId GetShelfId() { return shelf_id_; }

    // file must not be opened
    ErrorCode Create(mode_t mode, size_t size = 0);
    ErrorCode Destroy();
    ErrorCode Open(int flags);

    // file must be opened
    ErrorCode Close();
    // map a specific range
    ErrorCode Map(void *addr_hint, size_t length, int prot, int flags,
                  loff_t offset, void **mapped_addr,
                  bool register_fam_atomic = true);
    // map the entire file and register itself with ShelfManager
    // requires a valid shelf_id_
    ErrorCode Map(void *addr_hint, void **mapped_addr);

    // work in both cases (opened or not opened)
    // unmap a specific range (used by the memory manager)
    static ErrorCode Unmap(void *mapped_addr, size_t length,
                           bool unoregister_fam_atomic = true);

    // unmap the entire file and unregister itself with ShelfManager
    // requires a valid shelf_id_
    ErrorCode Unmap(void *mapped_addr, bool unregister);
    ErrorCode Truncate(off_t length);
    ErrorCode Rename(char const *new_pathname);
    bool Exist();
    size_t Size();
    ErrorCode GetPermission(mode_t *mode);
    ErrorCode SetPermission(mode_t mode);

    ErrorCode MarkInvalid();
    bool IsInvalid();

  private:
    int fd_;
    std::string path_;
    ShelfId shelf_id_;

    // DEBUG
    void *addr_;
    size_t length_;
};

} // namespace nvmm
#endif
