//
// ServiceHandler
// MQSim
//
// Created by 文盛業 on 2019-06-11.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__ServiceHandler__
#define __MQSim__ServiceHandler__

namespace Utils {
    template <typename PARAM_TYPE>
    class ServiceHandlerBase {
    public:
        virtual ~ServiceHandlerBase() = default;
        virtual void operator()(PARAM_TYPE param) = 0;
    };

    template <typename T, typename PARAM_TYPE>
    class ServiceHandler : public ServiceHandlerBase<PARAM_TYPE> {
        typedef void(T::*Handler)(PARAM_TYPE param);

    private:
        T* __callee;
        Handler  __handler;

    public:
        ServiceHandler(T* callee, Handler handler)
          : ServiceHandlerBase<PARAM_TYPE>(),
            __callee(callee),
            __handler(handler)
        { }

        ~ServiceHandler() final = default;

        void operator()(PARAM_TYPE param) final
        { (__callee->*__handler)(param); }
    };
}

#endif /* Predefined include guard __MQSim__ServiceHandler__ */
