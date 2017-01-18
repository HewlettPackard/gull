#include <errno.h>
#include <signal.h> // kill()
#include <fstream>
#include <sys/types.h>
#include <unistd.h>

#include "nvmm/log.h"

#include "common/process_id.h"


namespace nvmm{
   
ProcessID::ProcessID()
    : pid_{0}, btime_{0}
{
};

bool ProcessID::IsValid()
{
    return pid_!=0 && btime_!=0;
}

void ProcessID::SetPid()
{
    SetPid(getpid());
}

void ProcessID::SetPid(uint64_t pid)
{
    btime_ = GetBtime(pid);
    if (btime_!=0)
        pid_ = pid;
    else
        pid_ = 0;
}

bool ProcessID::IsAlive()
{
    assert (IsValid() == true);
    
    if (kill((pid_t)pid_,0) == 0)
    {
        // seems to be alive
        // alive
        //LOG(fatal) << "DistHeap: process " << pid_ << " is alive";
        
        uint64_t btime = GetBtime(pid_);
        if (btime == btime_)
            return true;
        else
            return false;        
    }
    else
    {
        if (errno == ESRCH)
        {
            // dead
            LOG(fatal) << "DistHeap: process " << pid_ << " is gone";
            return false;
        }
        else
        {
            // other error...
            LOG(fatal) << "DistHeap: process " << pid_ << " is experiencing problems? "
                       << "errno " << strerror(errno);
            uint64_t btime = GetBtime(pid_);
            if (btime == btime_)
                return true;
            else
                return false;
        }
    }
}

uint64_t ProcessID::GetBtime(uint64_t pid)
{
    std::string path = "/proc/"+std::to_string(pid)+"/stat";
    std::ifstream file(path);
    if (file.good())
    {
        std::string item;
        int count=0;
        while(std::getline(file, item, ' '))
        {
            count++;
            if (count==22)
            {
                //std::cout << item << std::endl;
                //LOG(fatal) << "GetBtime: (" << count <<") " << item;
                return std::stol(item);
            }
        }
    }
    return 0;
}

} // namespace nvmm
