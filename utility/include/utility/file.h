//!
//! casual_utility_file.h
//!
//! Created on: May 5, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_UTILITY_FILE_H_
#define CASUAL_UTILITY_FILE_H_

#include <string>

#include <regex>


namespace casual
{

	namespace utility
	{
		namespace file
		{
			void remove( const std::string& path);

			class RemoveGuard
			{
			public:
				RemoveGuard( const std::string& path);
				~RemoveGuard();

				const std::string& path() const;

			private:
				RemoveGuard( const RemoveGuard&);
				RemoveGuard& operator = ( const RemoveGuard&);

				const std::string m_path;
			};

			class ScopedPath : public RemoveGuard
			{
			public:
				ScopedPath( const std::string& path);

				operator const std::string& ();
			};

			//!
			//! Find the first file that matches search
			//!
			//! @param path The path to search
			//! @param search regexp to match file names
			//!
			std::string find( const std::string& path, const std::regex& search);


			//!
			//! @return filename or directory portion of pathname
			//!
			std::string basename( const std::string& path);

			//!
			//! @return the extension of the file. ex. yaml for file configuration.yaml
			//!
			std::string extension( const std::string& file);


		}

	}


}




#endif /* CASUAL_UTILITY_FILE_H_ */
