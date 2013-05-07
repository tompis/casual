//!
//! casual_utility_signal.h
//!
//! Created on: May 6, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_UTILITY_SIGNAL_H_
#define CASUAL_UTILITY_SIGNAL_H_


#include <cstddef>

#include "common/platform.h"


namespace casual
{

	namespace common
	{
		namespace signal
		{




			//!
			//! Throws if there has been a signal received.
			//!
			//! @throw subtype to exception::signal::Base
			//!
			void handle();


			namespace alarm
			{
				class Scoped
				{
				public:
					typedef std::size_t Seconds;

					Scoped( Seconds timeout);
					~Scoped();

				};

				void set( common::platform::seconds_type timeout);
			}
		}
	}
}



#endif /* CASUAL_UTILITY_SIGNAL_H_ */