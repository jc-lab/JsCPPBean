# JsCPPBean
JsCPPBean은 spring framework의 Bean과 Autowired처럼 클래스를 주입받아 사용할 수 있게 해주는 라이브러리 입니다.

예제소스(https://github.com/jc-lab/JsCPPBeanTest)

main.cpp

```C++
#include <stdio.h>

#include <JsCPPBean/BeanFactory.h>

#include "MainProcess.h"

int main() {
	MainProcess *mainProcess;
	JsCPPBean::BeanFactory *beanFactory = JsCPPBean::BeanFactory::getInstance();
	beanFactory->start();

	mainProcess = (MainProcess *)beanFactory->resolveBeanByName("MainProcess");
	mainProcess->run();

	return 0;
}
```

parent.h
```C++
#pragma once

#include <JsCPPBean/Bean.h>

class ChildService;

class ParentService
{
	JSCPPBEAN_OBJECT_DECL();

public:
	ChildService *m_child;

	ParentService();
	~ParentService();
};
```

parent.cpp
```C++
#include "ParentService.h"

JSCPPBEAN_BEAN_BEGIN(ParentService, "ParentService")
JSCPPBEAN_BEAN_AUTOWIRED_LAZY(ParentService, ChildService, m_child)
JSCPPBEAN_BEAN_END()

ParentService::ParentService()
{
}


ParentService::~ParentService()
{
}
```

child.h
```C++
#pragma once

#include <JsCPPBean\Bean.h>

class ParentService;
class ChildService : public JsCPPBean::BeanInitializer
{
	JSCPPBEAN_OBJECT_DECL();

private:
	ParentService *m_parent;

public:
	ChildService();
	~ChildService();

	void jcbPostConstruct() override;
	void jcbPreDestroy() override;
};
```

child.cpp
```C++
#include "ChildService.h"

JSCPPBEAN_BEAN_BEGIN(ChildService, "ChildService")
JSCPPBEAN_BEAN_AUTOWIRED(ChildService, ParentService, m_parent, "ParentService")
JSCPPBEAN_BEAN_END()

ChildService::ChildService()
{
}

ChildService::~ChildService()
{
}

void ChildService::jcbPostConstruct()
{
	printf("ChildService::jcbPostConstruct\n");
	printf("m_parent : %p\n", m_parent);
}

void ChildService::jcbPreDestroy()
{
	printf("ChildService::jcbPreDestroy\n");
	printf("m_parent : %p\n", m_parent);
}
```

MainProcess.h
```C++
#pragma once

#include <JsCPPBean\Bean.h>

class ParentService;
class ChildService;

class MainProcess {
	JSCPPBEAN_OBJECT_DECL();

private:
	ParentService *m_parent;
	ChildService *m_child;

public:
	void run();
};
```

MainProcess.cpp
```C++
#include "MainProcess.h"

#include <stdio.h>

JSCPPBEAN_BEAN_BEGIN(MainProcess, "MainProcess")
JSCPPBEAN_BEAN_AUTOWIRED(MainProcess, ChildService, m_child)
JSCPPBEAN_BEAN_AUTOWIRED(MainProcess, ParentService, m_parent, "ParentService")
JSCPPBEAN_BEAN_END()

void MainProcess::run()
{
	printf("MainProcess::run\n");
}
```
