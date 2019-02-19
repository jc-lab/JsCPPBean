#pragma once

#include <map>
#include <list>

#include <JsCPPUtils/SmartPointer.h>
#include <JsCPPUtils/Lockable.h>

#include "BeanInitializer.h"

#include "exceptions/BeanCreationException.hpp"
#include "exceptions/NoSuchBeanDefinitionException.hpp"

namespace JsCPPBean {

	class BeanFactory
	{
	private:
		class BeanObjectContextBase;

	public:
		class BeanBuilder {
		private:
			friend class BeanFactory;
			BeanObjectContextBase *beanObjectContext;
			JsCPPUtils::SmartPointer<BeanObjectContextBase> beanCtx;

			JsCPPUtils::SmartPointer<BeanObjectContextBase> swapBeanCtx() {
				JsCPPUtils::SmartPointer<BeanObjectContextBase> temp = beanCtx;
				beanCtx = NULL;
				return temp;
			}

		public:
			BeanBuilder(BeanObjectContextBase *beanObjectContext) {
				this->beanObjectContext = beanObjectContext;
			}

		public:
			template<typename T, typename U>
			void addAutowiredObject(U *T::*member, bool isLazy, const char *beanName = NULL) {
				std::string mapName;
				size_t varOffset = ((char*)&((T*)nullptr->*member) - (char*)nullptr);
				if (beanName)
				{
					mapName = "B";
					mapName.append(beanName);
				} else {
					mapName = "T";
					mapName.append(typeid(U).name());
				}

				this->beanObjectContext->autowirings.push_back(AutowiringObjectContext(mapName, varOffset, isLazy));
			}
		};

	private:
		JsCPPUtils::LockableEx m_lock;

		class AutowiringObjectContext {
		public:
			std::string mapName;
			BeanObjectContextBase *beanCtx;

			size_t varOffset;
			bool isLazy;

			AutowiringObjectContext(const std::string& mapName, size_t varOffset, bool isLazy) {
				this->mapName = mapName;
				this->beanCtx = NULL;
				this->varOffset = varOffset;
				this->isLazy = isLazy;
			}
		};

		class BeanObjectContextBase {
		public:
			int inited;
			std::string beanName;
			std::string className;

			BeanBuilder beanBuilder;
			std::list<AutowiringObjectContext> autowirings; // dependencies

			BeanObjectContextBase() : beanBuilder(this) {
				this->inited = 0;
			}

			virtual void *getPtr() = 0;
			virtual void callPostConstruct() = 0;
			virtual void callPreDestroy() = 0;
		};

		template<class T>
		class BeanObjectContextImpl : public BeanObjectContextBase {
		public:
			void *ptr;
			JsCPPUtils::SmartPointer<T> object;
			virtual ~BeanObjectContextImpl() {}
			BeanObjectContextImpl(T *existingObject) : ptr(existingObject) { }
			BeanObjectContextImpl(JsCPPUtils::SmartPointer<T> &existingObject) : object(existingObject) { ptr = object.getPtr(); }
			void *getPtr() override { return ptr;  };
			void callPostConstruct() override {
				if (::_JsCPPUtils_private::Loki::SuperSubclassStrict< BeanInitializer, T>::value)
				{
					BeanInitializer *beanInitializer = (BeanInitializer*)object.getPtr();
					if (beanInitializer)
						beanInitializer->jcbPostConstruct();
				}
			}
			void callPreDestroy() override {
				if (::_JsCPPUtils_private::Loki::SuperSubclassStrict< BeanInitializer, T>::value)
				{
					BeanInitializer *beanInitializer = (BeanInitializer*)object.getPtr();
					if (beanInitializer)
						beanInitializer->jcbPreDestroy();
				}
			}
		};

		static BeanFactory *m_instance;

		// Prefix
		// T{name} : typeid(class).name()
		// B{name} : User Bean Name
		std::map<std::string, JsCPPUtils::SmartPointer<BeanObjectContextBase> > m_beanObjects;

		void initializeBeanImpl(BeanObjectContextBase *beanObjectCtx, std::list<BeanObjectContextBase *> &callBeanCtx);

	public:
		BeanFactory();
		~BeanFactory();

		static BeanFactory *getInstance();

		void start() throw(exceptions::NoSuchBeanDefinitionException);

		template<class T>
		BeanBuilder *beanBuilder(JsCPPUtils::SmartPointer<T> existingBean)
		{
			JsCPPUtils::SmartPointer<BeanObjectContextBase> beanCtx = new BeanObjectContextImpl<T>(existingBean);
			beanCtx->className = typeid(T).name();
			beanCtx->beanBuilder.beanCtx = beanCtx;
			return &(beanCtx->beanBuilder);
		}

		template<class T>
		BeanBuilder *beanBuilder(T* existingBean)
		{
			JsCPPUtils::SmartPointer<BeanObjectContextBase> beanCtx = new BeanObjectContextImpl<T>(existingBean);
			beanCtx->className = typeid(T).name();
			beanCtx->beanBuilder.beanCtx = beanCtx;
			return &(beanCtx->beanBuilder);
		}

		// Bean 초기화와 함께 Bean등록
		void initializeBean(BeanBuilder *beanBuilder, const char *beanName);
		// Bean 등록하지 않고 Autowired와 BeanInitialize 핸들러만 작동
		void autowireBean(BeanBuilder *beanBuilder);

		template<class T>
		BeanBuilder *_beginRegisterBean(JsCPPUtils::SmartPointer<T> existingBean, const char *beanName = NULL)
		{
			JsCPPUtils::SmartPointer<BeanObjectContextBase> beanCtx = new BeanObjectContextImpl<T>(existingBean);
			beanCtx->className = typeid(T).name();
			m_beanObjects["T" + beanCtx->className] = beanCtx;
			if (beanName)
			{
				beanCtx->beanName = beanName;
				m_beanObjects["B" + beanCtx->beanName] = beanCtx;
			}
			return &(beanCtx->beanBuilder);
		}

		template<class T>
		T *autowire()
		{
			T* object = NULL;
			std::string className("T");
			className.append(typeid(T).name());
			m_lock.lock();
			std::map<std::string, JsCPPUtils::SmartPointer<BeanObjectContextBase> >::iterator foundIter = m_beanObjects.find(className);
			if (foundIter != m_beanObjects.end())
			{
				object = (T*)foundIter->second->getPtr();
			}
			m_lock.unlock();
			return object;
		}

		template<class T>
		T *autowire(const std::string& beanName)
		{
			T* object = NULL;
			m_lock.lock();
			std::map<std::string, JsCPPUtils::SmartPointer<BeanObjectContextBase> >::iterator foundIter = m_beanObjects.find("B" + beanName);
			if (foundIter != m_beanObjects.end())
			{
				object = (T*)foundIter->second->getPtr();
			}
			m_lock.unlock();
			return object;
		}
	};

}
