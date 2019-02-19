#pragma once

#include "JsCPPBeanException.hpp"

namespace JsCPPBean {

	namespace exceptions {

		class BeanCreationException : public JsCPPBeanException
		{
		public:
			BeanCreationException(const char *message, int errCode = 0) :
				JsCPPBeanException(message, errCode)
			{
			}

			BeanCreationException(const std::string &message, int errCode = 0) :
				JsCPPBeanException(message, errCode)
			{
			}
		};

	}

}
