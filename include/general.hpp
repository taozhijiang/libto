#ifndef _GENERAL_HPP_
#define _GENERAL_HPP_

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#include <string>
using std::string;

#include <cstdint>
using std::int64_t;
using std::uint64_t;

#include <memory>

#include <boost/log/trivial.hpp>

namespace libto {

extern char *basename(char *path);
#if 1
#define BOOST_LOG_T(x) std::cerr<<std::endl<<#x<<":"<<__LINE__<<"[@"<<__func__<<"]"<<" "
#else
#define BOOST_LOG_T(x) BOOST_LOG_TRIVIAL(x)<<::basename(__FILE__)<<":"<<__LINE__<<"[@"<<__func__<<"]"<<" "
#endif


int st_make_nonblock(int socket);

}



#endif // _GENERAL_HPP_
