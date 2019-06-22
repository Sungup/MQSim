#ifndef FLASH_TRANSACTION_QUEUE_H
#define FLASH_TRANSACTION_QUEUE_H

#include <list>
#include <string>
#include "NVM_Transaction_Flash.h"
#include "Queue_Probe.h"
#include "../sim/Sim_Reporter.h"

#include "../utils/InlineTools.h"

namespace SSD_Components
{
  // TODO Change inheritencies because default list operators is not virtual
  //  function
  class Flash_Transaction_Queue
    : public std::list<NVM_Transaction_Flash*>,
      public MQSimEngine::Sim_Reporter
  {
  private:
    std::string __id;
    Queue_Probe __queue_probe;

  public:
    Flash_Transaction_Queue();
    explicit Flash_Transaction_Queue(std::string id);

    void Set_id(const std::string& id);

    void remove(list<NVM_Transaction_Flash*>::iterator const& itr_pos);

    void push_back(NVM_Transaction_Flash* const&);
    void push_front(NVM_Transaction_Flash* const&);
    std::list<NVM_Transaction_Flash*>::iterator insert(list<NVM_Transaction_Flash*>::iterator position, NVM_Transaction_Flash* const& transaction);
    void remove(NVM_Transaction_Flash* const& transaction);
    void pop_front();

    void Report_results_in_XML(std::string name_prefix,
                               Utils::XmlWriter& xmlwriter) final;

  };

  typedef std::vector<Flash_Transaction_Queue> FlashTransactionQueueList;

  // TODO Need to change normal pointer to shared_ptr

  force_inline
  Flash_Transaction_Queue::Flash_Transaction_Queue()
    : std::list<NVM_Transaction_Flash*>(),
      MQSimEngine::Sim_Reporter(),
      __id(),
      __queue_probe()
  { }

  force_inline
  Flash_Transaction_Queue::Flash_Transaction_Queue(std::string id)
    : std::list<NVM_Transaction_Flash*>(),
      MQSimEngine::Sim_Reporter(),
      __id(std::move(id)),
      __queue_probe()
  { }

  force_inline void
  Flash_Transaction_Queue::Set_id(const std::string& id)
  {
    __id = id;
  }

  force_inline void
  Flash_Transaction_Queue::remove(std::list<NVM_Transaction_Flash*>::iterator const& itr_pos)
  {
    __queue_probe.DequeueRequest(*itr_pos);
    list<NVM_Transaction_Flash*>::erase(itr_pos);
  }

  force_inline void
  Flash_Transaction_Queue::push_back(NVM_Transaction_Flash* const& transaction)
  {
    __queue_probe.EnqueueRequest(transaction);
    return list<NVM_Transaction_Flash*>::push_back(transaction);
  }

  force_inline void
  Flash_Transaction_Queue::push_front(NVM_Transaction_Flash* const& transaction)
  {
    __queue_probe.EnqueueRequest(transaction);
    return list<NVM_Transaction_Flash*>::push_front(transaction);
  }

  force_inline std::list<NVM_Transaction_Flash*>::iterator
  Flash_Transaction_Queue::insert(std::list<NVM_Transaction_Flash*>::iterator position, NVM_Transaction_Flash* const& transaction)
  {
    __queue_probe.EnqueueRequest(transaction);
    return list<NVM_Transaction_Flash*>::insert(position, transaction);
  }
  force_inline void
  Flash_Transaction_Queue::remove(NVM_Transaction_Flash* const& transaction)
  {
    __queue_probe.DequeueRequest(transaction);
    return list<NVM_Transaction_Flash*>::remove(transaction);
  }

  force_inline void
  Flash_Transaction_Queue::pop_front()
  {
    __queue_probe.DequeueRequest(this->front());
    list<NVM_Transaction_Flash*>::pop_front();
  }

  force_inline void
  Flash_Transaction_Queue::Report_results_in_XML(std::string name_prefix,
                                                 Utils::XmlWriter& xmlwriter)
  {
    xmlwriter.Write_start_element_tag(name_prefix);

    std::string attr = "Name";
    std::string val = __id;
    xmlwriter.Write_attribute_string_inline(attr, val);

    attr = "No_Of_Transactions_Enqueued";
    val = std::to_string(__queue_probe.NRequests());
    xmlwriter.Write_attribute_string_inline(attr, val);

    attr = "No_Of_Transactions_Dequeued";
    val = std::to_string(__queue_probe.NDepartures());
    xmlwriter.Write_attribute_string_inline(attr, val);

    attr = "Avg_Queue_Length";
    val = std::to_string(__queue_probe.AvgQueueLength());
    xmlwriter.Write_attribute_string_inline(attr, val);

    attr = "Max_Queue_Length";
    val = std::to_string(__queue_probe.MaxQueueLength());
    xmlwriter.Write_attribute_string_inline(attr, val);

    attr = "STDev_Queue_Length";
    val = std::to_string(__queue_probe.STDevQueueLength());
    xmlwriter.Write_attribute_string_inline(attr, val);

    attr = "Avg_Transaction_Waiting_Time";
    val = std::to_string(__queue_probe.AvgWaitingTime());
    xmlwriter.Write_attribute_string_inline(attr, val);

    attr = "Max_Transaction_Waiting_Time";
    val = std::to_string(__queue_probe.MaxWaitingTime());
    xmlwriter.Write_attribute_string_inline(attr, val);

    xmlwriter.Write_end_element_tag();
  }
}

#endif // !FLASH_TRANSACTION_QUEUE_H
