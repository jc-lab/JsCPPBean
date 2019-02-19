#pragma once

#include "JsCPPBeanException.hpp"

namespace JsCPPBean {

	namespace exceptions {

		class NoSuchBeanDefinitionException : public JsCPPBeanException
		{
		public:
			NoSuchBeanDefinitionException(const char *message, int errCode = 0) :
				JsCPPBeanException(message, errCode)
			{
			}

			NoSuchBeanDefinitionException(const std::string& message, int errCode = 0) :
				JsCPPBeanException(message, errCode)
			{
			}
		};

	}

}
