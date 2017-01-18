#include <iomanip>
#include <fstream>

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/utility/empty_deleter.hpp>

#include <boost/log/core.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/attributes/current_process_id.hpp>
#include <boost/log/attributes/current_thread_id.hpp>

#include "nvmm/log.h"

namespace nvmm {
    
namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;
namespace keywords = boost::log::keywords;

// the global logger
boost::log::sources::severity_logger_mt<SeverityLevel> logger(off);

bool log_initialized = false;

// Defining attribute placeholders
BOOST_LOG_ATTRIBUTE_KEYWORD(process_id, "ProcessID", attrs::current_process_id::value_type)
BOOST_LOG_ATTRIBUTE_KEYWORD(thread_id, "ThreadID", attrs::current_thread_id::value_type)
BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", SeverityLevel)

void init_log(SeverityLevel level) {
  init_log(level, "output.log");
}

void init_log(SeverityLevel level, std::string file_name)
{
    if (log_initialized == true)
        return;

    log_initialized = true;

    // where to write the log
    // Construct the sink
    using text_sink = sinks::synchronous_sink< sinks::text_ostream_backend >;
    boost::shared_ptr< text_sink > sink = boost::make_shared< text_sink >();

    // Add file output
    if (file_name.empty()) {
      // Add console output
      // We have to provide an empty deleter to avoid destroying the global stream object
      boost::shared_ptr< std::ostream > stream(&std::clog, boost::empty_deleter());
      sink->locked_backend()->add_stream(stream);
    } else {
      sink->locked_backend()->add_stream(boost::make_shared< std::ofstream >(file_name));
    }


    // add log attributes
    logger.add_attribute("ProcessID", attrs::current_process_id());
    logger.add_attribute("ThreadID", attrs::current_thread_id());
    logger.add_attribute("LineID", attrs::counter< unsigned int >(1));
    
        
    // format log records
    logging::formatter fmt = expr::stream
        // // line id
        // << std::setw(8) << std::setfill('0') << line_id << " "
        // // severity
        // << ": <" << severity
        // << "> "
        // process id
        // << "[" << process_id
        // // thread id
        // << "," << thread_id
        //<< "] "
        // message
        << expr::smessage;

    sink->set_formatter(fmt);

    // set filter
    sink->set_filter(severity >= level);

    // all done
    // Register the sink in the logging core
    logging::core::get()->add_sink(sink);
}

} // namespace nvmm
