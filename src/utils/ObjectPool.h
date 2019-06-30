//
// ObjectPool
// MQSim
//
// Created by 文盛業 on 2019-06-28.
// Copyright (c) 2019 文盛業. All rights reserved.
//

#ifndef __MQSim__ObjectPool__
#define __MQSim__ObjectPool__

#include <cassert>
#include <cstdint>
#include <memory>

#include "InlineTools.h"

namespace Utils {
  class ObjectPoolBase;

  // ----------------------------
  // Definition of ObjectItemBase
  // ----------------------------
  class ObjectItemBase {
  private:
    ObjectPoolBase* __obj_pool;
    ObjectItemBase* __prev_obj;
    ObjectItemBase* __next_obj;

    void __set_pool(ObjectPoolBase* pool);

    ObjectItemBase* __next() const;

    bool __freed() const;

    void __append(ObjectItemBase* item);
    void __unlink();

    friend class ObjectPoolBase;

  protected:
    // Hide the constructor and destructor
    explicit ObjectItemBase(ObjectPoolBase* pool);
    virtual ~ObjectItemBase() = default;

    void _release();
  };

  // ----------------------------
  // Definition of ObjectPoolBase
  // ----------------------------
  class ObjectPoolBase {
  private:
    uint32_t __free_cnt;
    uint32_t __used_cnt;

    ObjectItemBase __free_list;
    ObjectItemBase __used_list;

    static void __destroy_all(ObjectItemBase& list);

  protected:
    bool _has_free() const;

    ObjectItemBase* _get_free();

    void _release(ObjectItemBase* item);
    void _make_in_use(ObjectItemBase* item);

    friend void ObjectItemBase::_release();

  public:
    ObjectPoolBase();
    virtual ~ObjectPoolBase();

    void destroy_all();
  };

  // ------------------------
  // Definition of ObjectItem
  // ------------------------
  template <class T>
  class ObjectItem : public T, public ObjectItemBase {
    friend class std::allocator< Utils::ObjectItem<T> >;
    
    // for the linux based working environments
    friend class __gnu_cxx::new_allocator< Utils::ObjectItem<T> >;

  protected:
    template <typename... _Args>
    explicit ObjectItem(ObjectPoolBase* pool, _Args&&... args);

    ~ObjectItem() final = default;

  public:
    void release();
  };

  // ------------------------
  // Definition of ObjectPool
  // ------------------------
  template <class T>
  class ObjectPool : public ObjectPoolBase {
  public:
    typedef ObjectItem<T> item_t;

  private:
    std::allocator<item_t> __allocator;

  public:
    ObjectPool();
    ~ObjectPool() override = default;

    template <typename... _Args>
    item_t* construct(_Args&&... args);
  };

  // --------------------------------
  // Implementation of ObjectItemBase
  // --------------------------------
  force_inline
  ObjectItemBase::ObjectItemBase(ObjectPoolBase* pool)
    : __obj_pool(pool),
      __prev_obj(this),
      __next_obj(this)
  { }

  force_inline void
  ObjectItemBase::__set_pool(ObjectPoolBase* pool)
  {
    __obj_pool = pool;
  }

  force_inline ObjectItemBase*
  ObjectItemBase::__next() const
  {
    return __next_obj;
  }

  force_inline bool
  ObjectItemBase::__freed() const
  {
    // Self linked item
    return __next_obj == this;
  }

  force_inline void
  ObjectItemBase::__append(ObjectItemBase* item)
  {
    auto* next = __next_obj;

    item->__prev_obj = this; item->__next_obj = next;
    next->__prev_obj = item; this->__next_obj = item;
  }

  force_inline void
  ObjectItemBase::__unlink()
  {
    auto* next = __next_obj; auto* prev = __prev_obj;

    next->__prev_obj = prev; prev->__next_obj = next;

    this->__prev_obj = this->__next_obj = this;
  }

  force_inline void
  ObjectItemBase::_release()
  {
    __obj_pool->_release(this);
  }

  // --------------------------------
  // Implementation of ObjectPoolBase
  // --------------------------------
  force_inline void
  ObjectPoolBase::__destroy_all(ObjectItemBase& list)
  {
    while (!list.__freed()) {
      auto* item = list.__next();

      item->__unlink();

      delete item;
    }
  }

  force_inline void
  ObjectPoolBase::destroy_all()
  {
    __free_cnt = __used_cnt = 0;
    __destroy_all(__free_list);
    __destroy_all(__used_list);
  }

  force_inline
  ObjectPoolBase::ObjectPoolBase()
    : __free_cnt(0),
      __used_cnt(0),
      __free_list(this),
      __used_list(this)
  { }

  force_inline
  ObjectPoolBase::~ObjectPoolBase()
  {
    destroy_all();
  }

  force_inline bool
  ObjectPoolBase::_has_free() const
  {
    return 0 < __free_cnt;
  }

  force_inline ObjectItemBase*
  ObjectPoolBase::_get_free()
  {
    assert(_has_free());

    auto* item = __free_list.__next();
    item->__unlink();
    --__free_cnt;

    return item;
  }

  force_inline void
  ObjectPoolBase::_release(ObjectItemBase* item)
  {
    item->__unlink();
    --__used_cnt;

    __free_list.__append(item);
    ++__free_cnt;
  }

  force_inline void
  ObjectPoolBase::_make_in_use(Utils::ObjectItemBase* item)
  {
    __used_list.__append(item);
    ++__used_cnt;
  }

  // ----------------------------
  // Implementation of ObjectItem
  // ----------------------------
  template <class T>
  template <typename... _Args>
  force_inline
  ObjectItem<T>::ObjectItem(ObjectPoolBase* pool, _Args&& ... args)
    : T(args...),
      ObjectItemBase(pool)
  { }

  template <class T>
  force_inline void
  ObjectItem<T>::release()
  {
    _release();
  }

  // ----------------------------
  // Implementation of ObjectPool
  // ----------------------------
  template <class T>
  force_inline
  ObjectPool<T>::ObjectPool()
    : ObjectPoolBase()
  { }

  template <class T>
  template <typename... _Args>
  force_inline typename ObjectPool<T>::item_t*
  ObjectPool<T>::construct(_Args&& ... args)
  {
    // 1. Get an allocated object.
    auto* object = _has_free()
                     ? static_cast<item_t*>(_get_free())
                     :  __allocator.allocate(1);

    __allocator.construct(object, this, args...);

    _make_in_use(object);

    return object;
  }

}

#endif /* Predefined include guard __MQSim__ObjectPool__ */
