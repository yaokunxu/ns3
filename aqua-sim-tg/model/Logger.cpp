/*
 * Logger.cpp
 *
 *  Created on: 2021年9月2日
 *      Author: root
 */
#include "Logger.h"
#include <cstdlib>
#include <ctime>
#include <time.h>
#include <stdio.h>
#include <chrono>
std::ofstream Logger::m_error_log_file;
std::ofstream Logger::m_info_log_file;
std::ofstream Logger::m_warn_log_file;
/*Logger::Logger() {
	// TODO Auto-generated constructor stub

}*/
void initLogger(const std::string& info_log_filename,
    const std::string& warn_log_filename,
    const std::string& error_log_filename) {
    Logger::m_info_log_file.open(info_log_filename.c_str());
    Logger::m_warn_log_file.open(warn_log_filename.c_str());
    Logger::m_error_log_file.open(error_log_filename.c_str());
}

std::ostream& Logger::getStream(log_rank tlog_rank) {
    return (INFO == tlog_rank) ?
        (m_info_log_file.is_open() ? m_info_log_file : std::cout) :
        (WARNING == tlog_rank ?
            (m_warn_log_file.is_open() ? m_warn_log_file : std::cerr) :
            (m_error_log_file.is_open() ? m_error_log_file : std::cerr));
}

std::ostream& Logger::start(log_rank tlog_rank,
    const int line,
    const std::string& function,
	const std::string& fileName) {
    time_t rawtime;
    struct tm info;
    char buffer[128];
    time(&rawtime);
    //localtime_s(&info,&rawtime);//windows
    localtime_r(&rawtime,&info);//Linux
    strftime(buffer, 128, "%Y-%m-%d %H:%M:%S", &info);
    return getStream(tlog_rank) << /*time_string*/buffer<<"   "
        <<"fileName ("<<fileName<<") "
        << "function (" << function << ")  "
        << "line: " << line<<"  "
        << std::flush;
}

Logger::~Logger() {
    getStream(m_log_rank) << std::endl << std::flush;

    if (FATAL == m_log_rank) {
        m_info_log_file.close();
        m_info_log_file.close();
        m_info_log_file.close();
        abort();
    }
}
void PRINT(){
	std::cout<<" printlll ";
}
