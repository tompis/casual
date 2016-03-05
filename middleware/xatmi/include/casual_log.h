//!
//! vlog.h
//!
//! Created on: Mar 28, 2012
//!     Author: Lazan
//!
//! "stolen" from BlackTie for the time being..
//!


#ifndef CASUAL_LOG_H_
#define CASUAL_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { c_log_error, c_log_warning, c_log_information, c_log_debug } casual_log_category_t;
extern int casual_log( casual_log_category_t category, const char* const format, ...);
extern int casual_vlog( casual_log_category_t category, const char* const format, va_list ap);

extern int casual_user_vlog( const char* category, const char* const format, va_list ap);

extern int casual_user_log( const char* category, const char* const message);



#ifdef __cplusplus
}
#endif


#endif /* CASUAL_LOG_H_ */
