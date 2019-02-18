#pragma once

#include <map>
#include <list>

#include <JsCPPUtils/SmartPointer.h>
#include <JsCPPUtils/Lockable.h>

#include "BeanInitializer.h"

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

		public:
			BeanBuilder(BeanObjectContextBase *beanObjectContext) {
				this->beanObjectContext = beanObjectContext;
			}

			JsCPPUtils::SmartPointer<BeanObjectContextBase> swapBeanCtx() {
				JsCPPUtils::SmartPointer<BeanObjectContextBase> temp = beanCtx;
				beanCtx = NULL;
				return temp;
			}

		public:
			void addAutowiredObject(const char *beanClass, size_t varOffset, bool isLazy, const char *beanName = NULL);

			template<typename T, typename U>
			void addAutowiredObject(const char *beanClass, U T::*member, bool isLazy, const char *beanName = NULL) {
				this->addAutowiredObject(beanClass, ((char*)&((T*)nullptr->*member) - (char*)nullptr), isLazy, beanName);
			}
		};

	private:
		JsCPPUtils::LockableEx m_lock;

		class AutowiringObjectContext {
		public:
			std::string beanClass;
			std::string beanName;
			size_t varOffset;
			bool isLazy;

			AutowiringObjectContext(const char *beanClass, size_t varOffset, const char *beanName, bool isLazy) {
				this->beanClass = beanClass;
				this->varOffset = varOffset;
				if (beanName)
					this->beanName = beanName;
				this->isLazy = isLazy;
			}
		};

		class BeanObjectContextBase {
		public:
			bool inited;
			std::string beanName;
			std::string className;

			BeanBuilder beanBuilder;
			std::list<AutowiringObjectContext> autowirings;
			int loadedAutowires;

			BeanObjectContextBase() : beanBuilder(this) {
				this->inited = false;
				this->loadedAutowires = 0;
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

		std::map<std::string, JsCPPUtils::SmartPointer<BeanObjectContextBase> > m_beanClasses;
		std::map<std::string, JsCPPUtils::SmartPointer<BeanObjectContextBase> > m_beanObjects;

		void initializeBeanImpl(BeanObjectContextBase *beanObjectCtx, bool isLazy);

	public:
		BeanFactory();
		~BeanFactory();

		static BeanFactory *getInstance();

		void start();

		void *resolveBeanByName(const std::string& name);
		void *resolveBeanByClass(const std::string& name);

		template<class T>
		BeanBuilder *beanBuilder(JsCPPUtils::SmartPointer<T> existingBean, const char *className, const char *beanName = NULL)
		{
			JsCPPUtils::SmartPointer<BeanObjectContextBase> beanCtx = new BeanObjectContextImpl<T>(existingBean);
			const char *name = beanName ? beanName : className;
			beanCtx->className = className;
			beanCtx->beanName = beanName;
			beanCtx->beanBuilder.beanCtx = beanCtx;
			return &(beanCtx->beanBuilder);
		}

		template<class T>
		BeanBuilder *beanBuilder(T* existingBean, const char *className, const char *beanName = NULL)
		{
			JsCPPUtils::SmartPointer<BeanObjectContextBase> beanCtx = new BeanObjectContextImpl<T>(existingBean);
			const char *name = beanName ? beanName : className;
			beanCtx->className = className;
			beanCtx->beanName = beanName;
			beanCtx->beanBuilder.beanCtx = beanCtx;
			return &(beanCtx->beanBuilder);
		}

		void autowireBean(BeanBuilder *beanBuilder) {
			m_lock.lock();
			m_beanClasses[beanBuilder->beanCtx->className] = beanBuilder->beanCtx;
			m_beanObjects[beanBuilder->beanCtx->beanName] = beanBuilder->beanCtx;
			m_lock.unlock();
			beanBuilder->beanCtx = NULL;
		}

		template<class T>
		BeanBuilder *_beginRegisterBean(JsCPPUtils::SmartPointer<T> existingBean, const char *className, const char *beanName = NULL)
		{
			JsCPPUtils::SmartPointer<BeanObjectContextBase> beanCtx = new BeanObjectContextImpl<T>(existingBean);
			const char *name = beanName ? beanName : className;
			beanCtx->className = className;
			beanCtx->beanName = beanName;
			m_beanClasses[className] = beanCtx;
			m_beanObjects[name] = beanCtx;
			return &(beanCtx->beanBuilder);
		}
	};

}
