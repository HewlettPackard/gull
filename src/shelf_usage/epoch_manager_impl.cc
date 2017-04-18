#include <assert.h>
#include <chrono>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <thread>
#include <unistd.h>

#include "nvmm/nvmm_fam_atomic.h"
#include "nvmm/epoch_manager.h"

#include "shelf_usage/participant_manager.h"
#include "shelf_usage/epoch_vector_internal.h"
#include "shelf_usage/hrtime.h"
#include "shelf_usage/epoch_manager_impl.h"

using nvmm::internal::EpochVector;

namespace nvmm {

class HeartBeat {
public:
    HeartBeat()
        : epoch(0)
    { }

    EpochCounter    epoch;
    struct timespec last_beat_time;
};

EpochManagerImpl::EpochManagerImpl(void *addr, bool may_create) :
    metadata_pool_(addr, MAX_POOL_SIZE),
    active_epoch_count_(0),
    terminate_monitor_(false),
    terminate_heartbeat_(false),
    debug_level_(0)
{
    epoch_vec_ = new EpochVector(&*metadata_pool_, may_create);

    pid_ = ParticipantManager::get_self_id();

    if (epoch_vec_->register_participant(pid_, &epoch_participant_) < 0) {
        std::ostringstream message;
        message << "EpochManager::EpochManager: attempt to register participant"
                << " in epoch vector failed.";
        throw std::runtime_error(message.str());
    }

    epoch_participant_.activate();

    if (debug_level_) {
        std::cerr << "Epoch Manager registered as participant: " << epoch_participant_.id() << std::endl;
    }

    pthread_mutex_init(&active_epoch_mutex_, NULL);

    // Attempt to advance frontier to ensure frontier advances at least once in
    // the lifetime of the process in case the process doesn't run long enough
    advance_frontier();

    // Spawn monitor threads
    std::thread mt(&EpochManagerImpl::monitor_thread_entry, this);
    monitor_thread_ = std::move(mt);
    std::thread hbt(&EpochManagerImpl::heartbeat_thread_entry, this);
    heartbeat_thread_ = std::move(hbt);
}


EpochManagerImpl::~EpochManagerImpl() {
    terminate_monitor_ = true;
    terminate_heartbeat_ = true;
    monitor_thread_.join();
    heartbeat_thread_.join();
    epoch_participant_.unregister();
    delete epoch_vec_;
}


EpochCounter EpochManagerImpl::reported_epoch() {
    return epoch_participant_.reported();
}


EpochCounter EpochManagerImpl::frontier_epoch() {
    return epoch_vec_->frontier();
}


void EpochManagerImpl::report_frontier() {
    epoch_participant_.update_reported(epoch_vec_->frontier());
}


void EpochManagerImpl::enter_critical() {
    epoch_lock_.sharedLock();
    pthread_mutex_lock(&active_epoch_mutex_);
    if (active_epoch_count_++ == 0) {
        report_frontier();
    }
    pthread_mutex_unlock(&active_epoch_mutex_);
}


void EpochManagerImpl::exit_critical() {
    pthread_mutex_lock(&active_epoch_mutex_);
    active_epoch_count_--;
    pthread_mutex_unlock(&active_epoch_mutex_);
    epoch_lock_.sharedUnlock();
}


bool EpochManagerImpl::exists_active_critical() {
    pthread_mutex_lock(&active_epoch_mutex_);
    bool active = active_epoch_count_ > 0;
    pthread_mutex_unlock(&active_epoch_mutex_);
    return active;
}


bool EpochManagerImpl::advance_frontier()
{
    bool success = false;
    bool all_in_frontier = true;
    std::vector<EpochVector::Participant> likely_dead;

    // For the current implementation of epochs based on S/X lock, 
    // we should avoid holding the X lock too long, as holding the 
    // X lock prevents epochs from progressing, thus facing the 
    // danger of being thought for dead.
    // We only need to hold the exclusiveLock to protect ourselves 
    // from other threads in this process, such as a thread
    // that might concurrently update cached state.
    epoch_lock_.exclusiveLock();
    epoch_vec_->invalidate_cache();
    epoch_lock_.exclusiveUnlock();

    internal::HRTime current_time = internal::get_hrtime();

    // If our last scan was too far in the past then our observed modified times are 
    // probably stale, so we should refresh to avoid accidentally killing processes 
    size_t time_since_last_scan_us = internal::diff_hrtime_us(last_scan_time_, current_time);
    if (time_since_last_scan_us > TIMEOUT_US) {
        epoch_vec_->refresh_modified_time();
    }
    
    last_scan_time_ = internal::get_hrtime();

    EpochCounter frontier = epoch_vec_->frontier();
    bool have_x_lock = false;
    for (EpochVector::Iterator it = epoch_vec_->begin();
         it != epoch_vec_->end(); 
         it++) 
    { 
        EpochVector::Participant ptc = *it;
        if (pid_ == ptc.id()) {
            epoch_lock_.exclusiveLock();
            have_x_lock = true;
        }
        EpochCounter reported = ptc.reported();
        if (reported >= internal::EPOCH_MIN_ACTIVE && reported != frontier) {
            all_in_frontier = false;
        }

        // If participant is lacking behind the frontier, then check for timeout 
        // suggesting possible failure 
        if (reported != internal::EPOCH_NO_PARTICIPANT && reported != frontier) {
            internal::HRTime current_time = internal::get_hrtime();
            if (internal::diff_hrtime_us(ptc.last_modified(), current_time) > TIMEOUT_US) {
                likely_dead.push_back(ptc);
            }
        }
        if (have_x_lock) {
            epoch_lock_.exclusiveUnlock();
            have_x_lock = false;
        }
    }

    if (all_in_frontier) {
        EpochCounter old_frontier = epoch_vec_->cas_frontier(frontier, frontier+1);
        if (old_frontier == frontier) {
            success = true;
        }
    }

    if (likely_dead.size() > 0) {
        for (std::vector<EpochVector::Participant>::iterator it = likely_dead.begin();
             it != likely_dead.end();
             it++) 
        { 
            EpochVector::Participant ptc = *it;
            if (debug_level_) {
                std::cerr << "Killing likely dead participant: " << ptc.id() << std::endl;
                std::cerr << "Waiting..." << std::endl;
            }
            try {
                if (debug_level_) {
                    // introduce some artificial delay (probably this should happen in the terminate call)
                    std::this_thread::sleep_for(std::chrono::seconds(3));
                }
                if (ptc.id() != 0) {
                    ParticipantManager::terminate(ptc.id());
                }
            } catch (std::exception& e) {
                std::cerr << e.what() << std::endl;
            }
            ptc.unregister();
        }
    }

    return success;
}


/**
 * A monitor thread that periodically wakes up and attempts to advance 
 * the frontier 
 */
void EpochManagerImpl::monitor_thread_entry() { 
    internal::HRTime last_debug_output = internal::get_hrtime();
    while (!terminate_monitor_) {
        usleep(MONITOR_INTERVAL_US);
        advance_frontier();

        if (debug_level_) {
            internal::HRTime current_time = internal::get_hrtime();
            if (internal::diff_hrtime_us(last_debug_output, current_time) > DEBUG_INTERVAL_US) {
                std::cerr << epoch_vec_->to_string() << std::endl;
                last_debug_output = internal::get_hrtime();
            }
        }
    }
}


/** 
 * A heartbeat thread that periodically wakes up and attempts to advance this
 * process' reported epoch to the frontier to signal process is alive and makes 
 * progress. Progress here just means completion of epoch operations. 
 * Thus, it doesn't necessarily imply application-level progress and liveness. 
 * For example, the application can be livelock but still complete epochs. 
 */
void EpochManagerImpl::heartbeat_thread_entry() {
    while (!terminate_heartbeat_) {
        assert(usleep(HEARTBEAT_INTERVAL_US)==0);
        // Grab exclusive lock to effectively drain active epochs and prevent
        // new epoch operations from occuring so that we can update and report 
        // our local view of the frontier
        epoch_lock_.exclusiveLock();
        pthread_mutex_lock(&active_epoch_mutex_);
        assert(active_epoch_count_ == 0);
        report_frontier();
        pthread_mutex_unlock(&active_epoch_mutex_);
        epoch_lock_.exclusiveUnlock();
    }
}

void EpochManagerImpl::set_debug_level(int level) {
    debug_level_ = level;
}

} // end namespace nvmm
