#ifndef MEMBER_FUNCTION_WRAPPER_H
#define MEMBER_FUNCTION_WRAPPER_H

#include "Events.h"

// This is the interface for MemberFunctionWrapper that each specialization will use
class HandlerFunctionBase 
{
public:
    // Call the member function
    void ExecuteMemberFunction(Event* event) 
    {
        call(event);
    }

    void SetId(unsigned int id) { m_Id = id; }
    const unsigned int GetId() const { return m_Id; }
private:
    // Implemented by MemberFunctionHandler
    virtual void call(Event* event) = 0;
    unsigned int m_Id = 0;
};

template<class T, class EventType>
class MemberFunctionHandler : public HandlerFunctionBase
{
public:
    typedef void (T::* MemberFunction)(EventType*);

    MemberFunctionHandler(T* classInstance, MemberFunction memberFunction) 
        : m_pInstance{ classInstance }, 
        m_MemberFunction{ memberFunction } {};

private:
    void call(Event* event) override
    {
        // Cast event to the correct type and call member function
        (m_pInstance->*m_MemberFunction)(static_cast<EventType*>(event));
    }

    // Pointer to class instance
    T* m_pInstance = nullptr;

    // Pointer to member function
    MemberFunction m_MemberFunction;
};

#endif
