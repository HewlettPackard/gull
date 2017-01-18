#ifndef _NVMM_PROCESS_ID_H_
#define _NVMM_PROCESS_ID_H_

#include <fstream>
#include <sys/types.h>
#include <unistd.h>

namespace nvmm{

class ProcessID
{
public:
    ProcessID();

    // set current process's pid and its creation time
    void SetPid();

    // set process id's pid and its creation time
    void SetPid(uint64_t id);

    // is this process ID valid?
    bool IsValid();
    
    // check if this process is still alive
    bool IsAlive();

    friend std::ostream& operator<<(std::ostream& os, const ProcessID& pid)
    {
        os << "[" << pid.pid_ << ", " << pid.btime_ << "]";
        return os;
    }

    friend bool operator==(const ProcessID &left, const ProcessID &right)
    {
        return left.pid_ == right.pid_ && left.btime_ == right.btime_;
    }   

    friend bool operator!=(const ProcessID &left, const ProcessID &right)
    {
        return !(left == right);
    }   

private:
    uint64_t GetBtime(uint64_t pid);
    
    uint64_t pid_; 
    uint64_t btime_; // creation time (number of jiffies since the machine booted)
};


} // namespace nvmm

#endif
