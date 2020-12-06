#ifndef EVENTBUS_H
#define EVENTBUS_H

#include <map>
#include <typeindex>
#include "MemberFunctionWrapper.h"

typedef std::vector<HandlerFunctionBase*> HandlerList;

class EventBus 
{
public:
	template<typename EventType>
	void Publish(EventType* event);

	template<class T, class EventType>
	void Subscribe(T* classInstance, void (T::* memberFunction)(EventType*));

	//Singleton of the eventbus
	static EventBus& GetInstance();

	template<class T, class EventType>
	void Unsubscribe(T* instance, void (T::* memberFunction)(EventType*));

	void UnsubscribeAll();

	// Only called when program exits
	~EventBus();
private:
	template<class T, class EventType>
	static unsigned int getID(T* instance);
	std::map<std::type_index, HandlerList*> m_Subscribers;
};

// Publish to the EventBus using an event from Events.h
template<typename EventType>
inline void EventBus::Publish(EventType* event)
{
	//Get all subscribed handlers associated with the current event
	HandlerList* handlers = m_Subscribers[typeid(EventType)];

	//No handler exists, exit
	if (handlers == nullptr)
	{
		return;
	}

	//Loop through each handler and execute events
	for (auto& handler : *handlers) 
	{
		if (handler != nullptr) 
		{
			handler->ExecuteMemberFunction(event);
		}
	}
}

// Subscribe to the EventBus
template<class T, class EventType>
inline void EventBus::Subscribe(T* classInstance, void(T::* memberFunction)(EventType*))
{
	//Get all subscribed handlers associated with the current event
	HandlerList* handlers = m_Subscribers[typeid(EventType)];

	//If there are no handlers for the current EventType, create one
	if (handlers == nullptr)
	{
		handlers = new HandlerList();
		m_Subscribers[typeid(EventType)] = handlers;
	}

	HandlerFunctionBase* temp = new MemberFunctionHandler<T, EventType>(classInstance, memberFunction);
	temp->SetId(getID<T, EventType>(classInstance));
	//Push function into list of handlers
	handlers->push_back(temp);
}

template<class T, class EventType>
inline void EventBus::Unsubscribe(T* classInstance, void(T::* memberFunction)(EventType*))
{
	if (m_Subscribers.empty())
	{
		return;
	}

	if (m_Subscribers.find(typeid(EventType)) != m_Subscribers.end())
	{
		HandlerList* handlers = m_Subscribers[typeid(EventType)];

		// Handlers could be null when closing down the program
		// This could probably be improved.
		if (handlers == nullptr)
		{
			return;
		}

		unsigned int id = getID<T, EventType>(classInstance);
		std::vector<HandlerFunctionBase*>::iterator it;
		for (it = handlers->begin(); it != handlers->end(); ++it)
		{
			if ((*it)->GetId() == id)
			{
				delete* it;
				handlers->erase(it);
				break;
			}
		}
	}
}

template<class T, class EventType>
inline unsigned int EventBus::getID(T* instance)
{
	unsigned int index = static_cast<unsigned int>(typeid(EventType).hash_code());
	unsigned int a = (index - static_cast<unsigned int>((size_t)instance));
	return a;
}

// Get the single instance of the EventBus in order to subscribe/publish
inline EventBus& EventBus::GetInstance()
{
	static EventBus instance;
	return instance;
}

inline void EventBus::UnsubscribeAll()
{
	for (auto const& i : m_Subscribers)
	{
		HandlerList* handlers = i.second;
		if (handlers != nullptr)
		{
			std::vector<HandlerFunctionBase*>::iterator iter;
			for (iter = handlers->begin(); iter != handlers->end(); iter++)
			{
				delete *iter;
			}
			delete handlers;
		}
	}
	m_Subscribers.clear();
}

inline EventBus::~EventBus()
{
	UnsubscribeAll();
}

#endif
