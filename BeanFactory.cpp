#include "BeanFactory.h"

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
		for (std::map<std::string, JsCPPUtils::SmartPointer<BeanObjectContextBase> >::iterator iter = m_beanObjects.begin(); iter != m_beanObjects.end(); iter++)
		{
			if (!iter->second->inited) {
				initializeBeanImpl(iter->second.getPtr(), false);
			}
		}
		for (std::map<std::string, JsCPPUtils::SmartPointer<BeanObjectContextBase> >::iterator iter = m_beanObjects.begin(); iter != m_beanObjects.end(); iter++)
		{
			if (!iter->second->inited) {
				initializeBeanImpl(iter->second.getPtr(), true);
			}
		}
	}

	void BeanFactory::initializeBeanImpl(BeanObjectContextBase *beanObjectCtx, bool isLazy)
	{
		void *clsPtr = (char*)beanObjectCtx->getPtr();
		for (std::list<AutowiringObjectContext>::iterator iter = beanObjectCtx->autowirings.begin(); iter != beanObjectCtx->autowirings.end(); iter++)
		{
			if (iter->isLazy == isLazy) {
				if (iter->beanName.empty())
					*((void**)((char*)clsPtr + iter->varOffset)) = resolveBeanByClass(iter->beanClass);
				else
					*((void**)((char*)clsPtr + iter->varOffset)) = resolveBeanByName(iter->beanName);
				beanObjectCtx->loadedAutowires++;
			}
		}

		if (beanObjectCtx->loadedAutowires == beanObjectCtx->autowirings.size()) {
			beanObjectCtx->callPostConstruct();
			beanObjectCtx->inited = true;
		}
	}

	void BeanFactory::BeanBuilder::addAutowiredObject(const char *beanClass, size_t varOffset, bool isLazy, const char *beanName)
	{
		this->beanObjectContext->autowirings.push_back(AutowiringObjectContext(beanClass, varOffset, beanName, isLazy));
	}

	void *BeanFactory::resolveBeanByName(const std::string& name) {
		m_lock.lock();
		std::map<std::string, JsCPPUtils::SmartPointer<BeanObjectContextBase> >::iterator iter = m_beanObjects.find(name);
		if (iter == m_beanObjects.end()) {
			m_lock.unlock();
			return NULL;
		}
		if (!iter->second->inited) {
			initializeBeanImpl(iter->second.getPtr(), false);
		}
		m_lock.unlock();
		return iter->second->getPtr();
	}
	void *BeanFactory::resolveBeanByClass(const std::string& name) {
		m_lock.lock();
		std::map<std::string, JsCPPUtils::SmartPointer<BeanObjectContextBase> >::iterator iter = m_beanClasses.find(name);
		if (iter == m_beanClasses.end()) {
			m_lock.unlock();
			return NULL;
		}
		if (!iter->second->inited) {
			initializeBeanImpl(iter->second.getPtr(), false);
		}
		m_lock.unlock();
		return iter->second->getPtr();
	}
}

