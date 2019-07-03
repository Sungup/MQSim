#ifndef EVENT_TREE_H
#define EVENT_TREE_H

#include "Sim_Defs.h"
#include "SimEvent.h"

#include "../utils/InlineTools.h"
#include "../utils/ObjectPool.h"

// TODO Remove static features if possible

namespace MQSimEngine
{
  class EventTreeNodeBase;
  typedef Utils::ObjectPool<EventTreeNodeBase> EventTreeNodePool;
  typedef EventTreeNodePool::item_t            EventTreeNode;

  class EventTreeNodeBase
  {
  public:
    // key provided by the calling class
    sim_time_type Key;
    // the data or value associated with the key
    SimEvent* FirstSimEvent;
    SimEvent* LastSimEvent;
    // color - used to balance the tree
    /*RED = 0 , BLACK = 1;*/
    int Color;
    // left node 
    EventTreeNode* Left;
    // right node 
    EventTreeNode* Right;
    // parent node 
    EventTreeNode* Parent;

    EventTreeNodeBase();
  };

  force_inline
  EventTreeNodeBase::EventTreeNodeBase()
    : Key(0),
      FirstSimEvent(nullptr),
      LastSimEvent(nullptr),
      Color(0),
      Left(nullptr),
      Right(nullptr),
      Parent(nullptr)
  { }

  class EventTree {
  private:
    // Object Pool for EventTreeNode
    EventTreeNodePool __pool;

    // the number of nodes contained in the tree
    int __count;

    //  sentinelNode is convenient way of indicating a leaf node.
    static EventTreeNode* __sentinel;

    // the tree
    EventTreeNode* __rb_tree;
    // the node that was last found; used to optimize searches
    EventTreeNode* __last_found_node;

  private:
    void RestoreAfterInsert(EventTreeNode* x);
    void Delete(EventTreeNode* z);
    void Restore_after_delete(EventTreeNode* x);

  public:
    EventTree();
    ~EventTree();

    bool is_exist() const;

    void Add(sim_time_type key, SimEvent* data);
    void RotateLeft(EventTreeNode* x);
    void RotateRight(EventTreeNode* x);
    SimEvent* GetData(sim_time_type key);
    void Insert_sim_event(SimEvent* data);
    sim_time_type Get_min_key();
    SimEvent* Get_min_value();
    EventTreeNode* Get_min_node();
    void Remove(sim_time_type key);
    void Remove(EventTreeNode* node);
    void Remove_min();
    void Clear();
  };

  ///<summary>
  /// is_exist
  /// Check tree has an entry or not
  ///<summary>
  force_inline bool
  EventTree::is_exist() const
  {
    return __count > 0;
  }

  ///<summary>
  /// Clear
  /// Empties or clears the tree
  ///<summary>
  force_inline void
  EventTree::Clear()
  {
    __rb_tree = __sentinel;
    __count = 0;
  }

  ///<summary>
  /// Remove_min
  /// removes the node with the minimum key
  ///<summary>
  force_inline void
  EventTree::Remove_min()
  {
    Remove(Get_min_key());
  }

  ///<summary>
  /// Get_min_value
  /// Returns the object having the minimum key value
  ///<summary>
  force_inline SimEvent*
  EventTree::Get_min_value()
  {
    //      return GetData(Get_min_key());
    return Get_min_node()->FirstSimEvent;
  }

  ///<summary>
  /// Get_min_key
  /// Returns the minimum key value
  ///<summary>
  force_inline sim_time_type
  EventTree::Get_min_key()
  {
    return Get_min_node()->Key;
  }
}

#endif // !EVENT_TREE_H
