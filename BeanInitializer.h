#pragma once

namespace JsCPPBean {
	class BeanInitializer {
	public:
		virtual ~BeanInitializer() {}
		virtual void jcbPostConstruct() {}
		virtual void jcbPreDestroy() {}
	};
}
