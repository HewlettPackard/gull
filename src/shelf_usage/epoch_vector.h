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

#ifndef _NVMM_EPOCH_VECTOR_H_
#define _NVMM_EPOCH_VECTOR_H_

#include <time.h>

#include "nvmm/epoch_manager.h"
#include "shelf_usage/participant_manager.h"


namespace nvmm {
namespace internal {


// forward declaration
struct _EpochVector;

/**
 *
 * \brief Vector comprising frontier epoch and participant reported epochs
 *
 * \details
 * Per-process wrapper around the FAM structure _EpochVector. 
 * This class maintains useful volatile state such as a cached version of 
 * the vector and timestamps of last seen updates.
 */
class EpochVector {
public: 
    class Iterator;

    class Participant;

private:
    struct Element {
        bool            valid_;
        ParticipantID   pid_;
        EpochCounter    reported_;
        struct timespec last_modified_;
    };

public:
    EpochVector(_EpochVector* vec, bool may_create);

    /** Return the frontier epoch */
    EpochCounter frontier();

    /** Atomically compare-and-swap frontier with a new value \a new_epoch */
    EpochCounter cas_frontier(EpochCounter old_epoch, EpochCounter new_epoch);

    /** Register participant identified by \a pid */
    int register_participant(ParticipantID pid, Participant* participant); 

    /** Unregister participant */   
    void unregister_participant(Participant& participant);

    /** Invalidate cached version of the participant epoch vector */
    void invalidate_cache();

    /** Update modified time to current time */
    void refresh_modified_time();

    /** Iterator to the first participant */
    Iterator begin();

    /** Iterator to the last participant */
    Iterator end();

    std::string to_string();

    /** Reset epoch vector */
    void reset();

private:
    ParticipantID pid(int slot);
    EpochCounter reported(int slot);
    void set_reported(int slot, EpochCounter epoch);
    struct timespec last_modified(int slot);

private:
    _EpochVector* ev_;    /** the global vector stored in FAM */
    Element*      cache_; /** a local cached version of the vector */
};



class EpochVector::Iterator {
public:
    Iterator();
    Iterator(EpochVector* ev, int slot);

    Participant operator*() const;
    bool operator==(const Iterator& other) const;
    bool operator!=(const Iterator& other) const;
    Iterator& operator++();
    Iterator operator++(int);

private:
    EpochVector* ev_;
    int          slot_;
};



class EpochVector::Participant {
public:
    friend EpochVector;

public:
    Participant();
    Participant(EpochVector* ev, int slot);

    /**
     * \brief Returns the last time we saw a modification of the reported 
     * epoch from this participant
     */
    struct timespec last_modified();

    /**
     * \brief Activates this participant
     *
     * \details
     * Activates participant by freezing the frontier, that is preventing the 
     * frontier from advancing, so that we can safely jump onto the wagon.
     */
    void activate();

    /**
     * \brief Unregister this participant
     */
    void unregister();

    /*
     * \brief Update our reported view of the frontier
     *
     * \details
     * Before we can update our local view of the frontier we have to 
     * become an active participant first by calling activate(). 
     */
    void update_reported(EpochCounter epoch);

    /** Return the last reported epoch by this participant */
    EpochCounter reported();

    /** Return participant identifier */
    ParticipantID id();

//private:
    EpochVector*  ev_;
    int           slot_;
};


} // end namespace internal
} // end namespace nvmm

#endif // _NVMM_EPOCH_VECTOR_H_
