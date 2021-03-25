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

#ifndef _NVMM_SHELF_REGION_H_
#define _NVMM_SHELF_REGION_H_

#include <stddef.h>
#include <stdint.h>
#include <string>

#include "nvmm/error_code.h"
#include "shelf_mgmt/shelf_file.h"

namespace nvmm {

class ShelfRegion
{
public:
    ShelfRegion() = delete;
    ShelfRegion(std::string pathname);
    ~ShelfRegion();

    ErrorCode Create(size_t size);
    ErrorCode Destroy();
    ErrorCode Verify();    
    ErrorCode Resize(size_t);

    bool IsOpen() const
    {
        return is_open_;
    }
    
    ErrorCode Open(int flags);
    ErrorCode Close();
    size_t Size();
    
    ErrorCode Map(void *addr_hint, size_t length, int prot, int flags, loff_t offset, void **mapped_addr);
    ErrorCode Unmap(void *mapped_addr, size_t length);

    ErrorCode GetPermission(mode_t *mode);   
    ErrorCode SetPermission(mode_t mode);
private:
    bool is_open_;    
    ShelfFile shelf_;
};

} // namespace nvmm
#endif
