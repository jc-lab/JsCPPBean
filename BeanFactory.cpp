#include "BeanFactory.h"

#include "exceptions/NoSuchBeanDefinitionException.hpp"
#include "exceptions/BeanCreationException.hpp"

namespace JsCPPBean {
	BeanFactory::BeanFactory()
	{
	}

	BeanFactory::~BeanFactory()
	{
	}

	BeanFactory *BeanFactory::getInstance()
	{
		static JsCPPUtils::SmartPointer<BeanFactory> instance = new BeanFactory();
		return instance.getPtr();
	}

	void BeanFactory::findAllDependency(BeanObjectContextBase *beanCtx)
	{
		// Find all dependencies
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

	void BeanFactory::start()
	{
		// Find all dependencies
		for (std::map<std::string, JsCPPUtils::SmartPointer<BeanObjectContextBase> >::iterator beanIter = m_beanObjects.begin(); beanIter != m_beanObjects.end(); beanIter++)
		{
			findAllDependency(beanIter->second.getPtr());
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

	void BeanFactory::stop()
	{
		for (std::map<std::string, JsCPPUtils::SmartPointer<BeanObjectContextBase> >::iterator beanIter = m_beanObjects.begin(); beanIter != m_beanObjects.end(); )
		{
			beanIter->second->callPreDestroy();
			beanIter = m_beanObjects.erase(beanIter);
		}
	}

	void BeanFactory::autowireBean(JsCPPUtils::SmartPointer<BeanBuilder> beanBuilder)
	{
		std::list<BeanObjectContextBase *> callBeanStack;
		m_lock.lock();
		findAllDependency(beanBuilder->beanCtx.getPtr());
		initializeBeanImpl(beanBuilder->beanCtx.getPtr(), callBeanStack);
		beanBuilder->beanCtx->callPostConstruct();
		m_lock.unlock();
	}

	void BeanFactory::initializeBean(JsCPPUtils::SmartPointer<BeanBuilder> beanBuilder, const char *beanName)
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
					*((void**)((char*)clsPtr + iter->varOffset)) = (iter->beanCtx->getPtr());
				}
			}
			callBeanStack.pop_back();
		}
	}
}

