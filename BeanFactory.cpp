#include "BeanFactory.h"

#include "exceptions/NoSuchBeanDefinitionException.hpp"
#include "exceptions/BeanCreationException.hpp"

namespace JsCPPBean {
	BeanFactory *BeanFactory::m_instance = BeanFactory::getInstance();

	BeanFactory::BeanFactory()
	{
	}

	BeanFactory::~BeanFactory()
	{
	}

	BeanFactory *BeanFactory::getInstance()
	{
		if (!m_instance) {
			m_instance = new BeanFactory();
		}
		return m_instance;
	}

	void BeanFactory::start()
	{
		// Find all dependencies
		for (std::map<std::string, JsCPPUtils::SmartPointer<BeanObjectContextBase> >::iterator beanIter = m_beanObjects.begin(); beanIter != m_beanObjects.end(); beanIter++)
		{
			BeanObjectContextBase *beanCtx = beanIter->second.getPtr();
			for (std::list<AutowiringObjectContext>::iterator awIter = beanCtx->autowirings.begin(); awIter != beanCtx->autowirings.end(); awIter++)
			{
				std::map<std::string, JsCPPUtils::SmartPointer<BeanObjectContextBase> >::iterator foundBean = m_beanObjects.find(awIter->mapName);
				if (foundBean == m_beanObjects.end()) {
					// could not find bean
					throw exceptions::NoSuchBeanDefinitionException("Could not find bean: " + awIter->mapName);
				}
				awIter->beanCtx = foundBean->second.getPtr();
			}
		}

		// initialize beans
		for (std::map<std::string, JsCPPUtils::SmartPointer<BeanObjectContextBase> >::iterator beanIter = m_beanObjects.begin(); beanIter != m_beanObjects.end(); beanIter++)
		{
			BeanObjectContextBase *beanCtx = beanIter->second.getPtr();
			std::list<BeanObjectContextBase *> callCtxStack;
			initializeBeanImpl(beanCtx, callCtxStack);
		}

		// Lazy initialize beans
		for (std::map<std::string, JsCPPUtils::SmartPointer<BeanObjectContextBase> >::iterator beanIter = m_beanObjects.begin(); beanIter != m_beanObjects.end(); beanIter++)
		{
			BeanObjectContextBase *beanCtx = beanIter->second.getPtr();
			void *clsPtr = (char*)beanCtx->getPtr();
			for (std::list<AutowiringObjectContext>::iterator iter = beanCtx->autowirings.begin(); iter != beanCtx->autowirings.end(); iter++)
			{
				*((void**)((char*)clsPtr + iter->varOffset)) = (iter->beanCtx->getPtr());
			}
			beanCtx->callPostConstruct();
			beanCtx->inited = 2;
		}
	}

	// Bean 초기화와 함께 Bean등록
	void BeanFactory::initializeBean(BeanBuilder *beanBuilder, const char *beanName)
	{
		std::list<BeanObjectContextBase *> callBeanStack;
		m_lock.lock();
		initializeBeanImpl(beanBuilder->beanCtx.getPtr(), callBeanStack);
		m_beanObjects["T" + beanBuilder->beanCtx->className] = beanBuilder->beanCtx;
		if (beanName)
		{
			beanBuilder->beanCtx->beanName = beanName;
			m_beanObjects["B" + beanBuilder->beanCtx->beanName] = beanBuilder->beanCtx;
		}
		m_lock.unlock();
		beanBuilder->beanCtx = NULL;
	}

	// Bean 등록하지 않고 Autowired와 BeanInitialize 핸들러만 작동
	void BeanFactory::autowireBean(BeanBuilder *beanBuilder)
	{
		std::list<BeanObjectContextBase *> callBeanStack;
		m_lock.lock();
		initializeBeanImpl(beanBuilder->beanCtx.getPtr(), callBeanStack);
		m_lock.unlock();
		beanBuilder->beanCtx = NULL;
	}

	void BeanFactory::initializeBeanImpl(BeanObjectContextBase *beanObjectCtx, std::list<BeanObjectContextBase *> &callBeanStack)
	{
		if (beanObjectCtx->inited == 0)
		{
			void *clsPtr = (char*)beanObjectCtx->getPtr();
			beanObjectCtx->inited = 1;
			callBeanStack.push_back(beanObjectCtx);
			for (std::list<AutowiringObjectContext>::iterator iter = beanObjectCtx->autowirings.begin(); iter != beanObjectCtx->autowirings.end(); iter++)
			{
				bool alreadyResolved = false;
				for (std::list<BeanObjectContextBase *>::iterator iterStack = callBeanStack.begin(); iterStack != callBeanStack.end(); iterStack++)
				{
					if (*iterStack == iter->beanCtx)
					{
						alreadyResolved = true;
						break;
					}
				}
				if (!iter->isLazy)
				{
					if(alreadyResolved)
						throw exceptions::BeanCreationException("Circular reference detected: " + iter->mapName);
					else
						initializeBeanImpl(iter->beanCtx, callBeanStack);
				}
			}
			callBeanStack.pop_back();
		}
	}
}

