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

#ifndef _NVMM_TEST_H_
#define _NVMM_TEST_H_

#include "nvmm/log.h"
#include <gtest/gtest.h>

namespace nvmm {

class Environment : public ::testing::Environment {
  public:
    Environment(boost::log::trivial::severity_level level =
                    boost::log::trivial::severity_level::fatal,
                bool to_console = false) {
        level_ = level;
        to_console_ = to_console;
    }
    ~Environment() override {}

    // Override this to define how to set up the environment.
    void SetUp() override;

    // Override this to define how to tear down the environment.
    void TearDown() override {}

  private:
    boost::log::trivial::severity_level level_;
    bool to_console_;
};

} // namespace nvmm

#endif
