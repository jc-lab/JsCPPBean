#pragma once

#include <list>
#include <string>

#include "BeanFactory.h"
#include "BeanInitializer.h"

namespace JsCPPBean {
	namespace internal {
		class AutowiringVar {
		public:
			void *targetVar;
			std::string className;
			std::string beanName;
		};
	}
}

#define JSCPPBEAN_OBJECT_DECL() \
	public: \
		static void __jscppbean_objectInit()

#define JSCPPBEAN_BEAN_BEGIN(CLASS, ...) \
	struct _JsCPPBeanClassLoader1_ ## CLASS { \
		_JsCPPBeanClassLoader1_ ## CLASS() { CLASS::__jscppbean_objectInit(); } \
	} __t_JsCPPBeanClassLoader1_ ## CLASS; \
	void CLASS::__jscppbean_objectInit() { \
		::JsCPPBean::BeanFactory *beanFactory = ::JsCPPBean::BeanFactory::getInstance(); \
		::JsCPPUtils::SmartPointer<CLASS> object = new CLASS(); \
		::JsCPPBean::BeanFactory::BeanBuilder *beanBuilder = beanFactory->_beginRegisterBean(object, #CLASS, __VA_ARGS__);

#define JSCPPBEAN_BEAN_AUTOWIRED_LAZY(TARGETCLASS, BEANCLASS, VARNAME, ...) \
		beanBuilder->addAutowiredObject(#BEANCLASS, &TARGETCLASS::VARNAME, true, __VA_ARGS__); \

#define JSCPPBEAN_BEAN_AUTOWIRED(TARGETCLASS, BEANCLASS, VARNAME, ...) \
		beanBuilder->addAutowiredObject(#BEANCLASS, &TARGETCLASS::VARNAME, false, __VA_ARGS__); \

#define JSCPPBEAN_BEAN_END() }
