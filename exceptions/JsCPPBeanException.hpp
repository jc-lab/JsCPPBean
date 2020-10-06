#pragma once

#include <exception>
#include <string>

namespace JsCPPBean {

	namespace exceptions {

		class JsCPPBeanException : public std::exception {
		private:
			std::string storedMessage;

		public:
			JsCPPBeanException(const char *message, int errCode = 0) :
				std::exception(message, errCode)
			{
			}

			JsCPPBeanException(const std::string& message, int errCode = 0) :
				storedMessage(message),
				std::exception(storedMessage.c_str(), errCode)
			{
			}
		};

	}

}
