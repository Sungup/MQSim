#ifndef FLASH_TRANSACTION_QUEUE_H
#define FLASH_TRANSACTION_QUEUE_H

#include <list>
#include <string>
#include "NvmTransactionFlash.h"
#include "Queue_Probe.h"
#include "../sim/Sim_Reporter.h"

#include "../utils/InlineTools.h"

namespace SSD_Components
{
  class Flash_Transaction_Queue : public MQSimEngine::Sim_Reporter
  {
  private:
    std::string __id;
    std::list<NvmTransactionFlash*> __queue;
    Queue_Probe __queue_probe;

  public:
    Flash_Transaction_Queue();
    explicit Flash_Transaction_Queue(std::string id);

    ~Flash_Transaction_Queue() final = default;

    void Set_id(const std::string& id);

    void remove(std::list<NvmTransactionFlash*>::iterator const& itr_pos);

    void push_back(NvmTransactionFlash* const&);
    void push_front(NvmTransactionFlash* const&);
    std::list<NvmTransactionFlash*>::iterator insert(std::list<NvmTransactionFlash*>::iterator position, NvmTransactionFlash* const& transaction);
    void remove(NvmTransactionFlash* const& transaction);
    void pop_front();

    bool empty() const;
    size_t size() const;
    NvmTransactionFlash* front();

    std::list<NvmTransactionFlash*>::iterator begin();
    std::list<NvmTransactionFlash*>::iterator end();
    std::list<NvmTransactionFlash*>::const_iterator begin() const;
    std::list<NvmTransactionFlash*>::const_iterator end() const;

    void Report_results_in_XML(std::string name_prefix,
                               Utils::XmlWriter& xmlwriter) final;

  };

  typedef std::vector<Flash_Transaction_Queue> FlashTransactionQueueList;

  // TODO Need to change normal pointer to shared_ptr

  force_inline
  Flash_Transaction_Queue::Flash_Transaction_Queue()
    : MQSimEngine::Sim_Reporter(),
      __id(),
      __queue(),
      __queue_probe()
  { }

  force_inline
  Flash_Transaction_Queue::Flash_Transaction_Queue(std::string id)
    : MQSimEngine::Sim_Reporter(),
      __id(std::move(id)),
      __queue(),
      __queue_probe()
  { }

  force_inline void
  Flash_Transaction_Queue::Set_id(const std::string& id)
  {
    __id = id;
  }

  force_inline void
  Flash_Transaction_Queue::remove(std::list<NvmTransactionFlash*>::iterator const& itr_pos)
  {
    __queue_probe.DequeueRequest(*itr_pos);
    __queue.erase(itr_pos);
  }

  force_inline void
  Flash_Transaction_Queue::push_back(NvmTransactionFlash* const& transaction)
  {
    __queue_probe.EnqueueRequest(transaction);
    return __queue.push_back(transaction);
  }

  force_inline void
  Flash_Transaction_Queue::push_front(NvmTransactionFlash* const& transaction)
  {
    __queue_probe.EnqueueRequest(transaction);
    return __queue.push_front(transaction);
  }

  force_inline std::list<NvmTransactionFlash*>::iterator
  Flash_Transaction_Queue::insert(std::list<NvmTransactionFlash*>::iterator position, NvmTransactionFlash* const& transaction)
  {
    __queue_probe.EnqueueRequest(transaction);
    return __queue.insert(position, transaction);
  }
  force_inline void
  Flash_Transaction_Queue::remove(NvmTransactionFlash* const& transaction)
  {
    __queue_probe.DequeueRequest(transaction);
    return __queue.remove(transaction);
  }

  force_inline void
  Flash_Transaction_Queue::pop_front()
  {
    __queue_probe.DequeueRequest(__queue.front());
    __queue.pop_front();
  }

  force_inline size_t
  Flash_Transaction_Queue::size() const
  {
    return __queue.size();
  }

  force_inline bool
  Flash_Transaction_Queue::empty() const
  {
    return __queue.empty();
  }

  force_inline NvmTransactionFlash*
  Flash_Transaction_Queue::front()
  {
    return __queue.front();
  }

  force_inline std::list<NvmTransactionFlash*>::const_iterator
  Flash_Transaction_Queue::begin() const
  {
    return __queue.begin();
  }

  force_inline std::list<NvmTransactionFlash*>::iterator
  Flash_Transaction_Queue::begin()
  {
    return __queue.begin();
  }

  force_inline std::list<NvmTransactionFlash*>::const_iterator
  Flash_Transaction_Queue::end() const
  {
    return __queue.end();
  }

  force_inline std::list<NvmTransactionFlash*>::iterator
  Flash_Transaction_Queue::end()
  {
    return __queue.end();
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
