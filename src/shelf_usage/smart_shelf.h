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

/*
  modified from the shelf implementation from Mark
 */

#ifndef _NVMM_SMART_SHELF_H_
#define _NVMM_SMART_SHELF_H_

#include <stddef.h>
#include <stdexcept>
#include <stdint.h>

#include "common/common.h"
#include "nvmm/global_ptr.h"

namespace nvmm {

class IOError : public std::runtime_error {
  public:
    IOError(int error_no);
    int error_no; // errno value
};

class SmartShelf_ {
  public:
    // may throw IOError, (on bad parameters) std::logic_error, or
    // std::runtime error (existing file incompatible with passed parameters)
    SmartShelf_(void *addr, size_t fixed_section_size, size_t max_shelf_size);
    ~SmartShelf_();

    /*
     * Valid Offset's for the variable section of this Shelf are 0
     * (NULL equivalent) and start_ptr() .. size()-1.
     */
    Offset start_ptr(); // cacheline aligned
    size_t size();

    // get start of the fixed section
    void *fixed_section();

    // converting to and from local-address-space pointers
    void *from_Offset(Offset p);
    Offset to_Offset(void *p);

    // shortcut for from_Offset; does not work well on (Shelf)*:
    //   need (*shelf)[ptr] for that case
    void *operator[](Offset p) { return from_Offset(p); }

    // Shelf_'s are not copyable or assignable:
    SmartShelf_(const SmartShelf_ &) = delete;
    SmartShelf_ &operator=(const SmartShelf_ &) = delete;

  private:
    size_t start;
    void *shelf_location;
    size_t mapped_size;
};

template <typename Fixed> class SmartShelf : public SmartShelf_ {
  public:
    // throws same types as SmartShelf_
    SmartShelf(void *addr, size_t max_shelf_size)
        : SmartShelf_(addr, sizeof(Fixed), max_shelf_size) {}

    // provide easy access to fixed section by pretending we are a
    // pointer to it:
    Fixed &operator*() { return *(Fixed *)fixed_section(); }
    Fixed *operator->() { return (Fixed *)fixed_section(); }
};

// Specialize with type void if you want no fixed section:
template <> class SmartShelf<void> : public SmartShelf_ {
  public:
    SmartShelf(void *addr, size_t max_shelf_size)
        : SmartShelf_(addr, 0, max_shelf_size) {}
};

/***************************************************************************/
/*                                                                         */
/* Implementation-only details follow                                      */
/*                                                                         */
/***************************************************************************/

inline void *SmartShelf_::fixed_section() {
    return (char *)shelf_location + kCacheLineSize;
}

inline void *SmartShelf_::from_Offset(Offset p) {
    if (p == 0)
        return NULL;
    else
        return (char *)shelf_location + p;
}

inline Offset SmartShelf_::to_Offset(void *p) {
    if (p == NULL)
        return 0;
    else
        return (char *)p - (char *)shelf_location;
}

} // namespace nvmm

#endif
