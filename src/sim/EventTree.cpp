#include "EventTree.h"
#include "Engine.h"
#include "../utils/Exception.h"

using namespace MQSimEngine;

EventTreeNode* EventTree::__sentinel = nullptr;

EventTree::EventTree()
  : __pool(),
    __count(0)
{
  // set up the sentinel node. the sentinel node is the key to a successfull
  // implementation and for understanding the red-black tree properties.
  __sentinel = __pool.construct();
  __sentinel->Left = nullptr;
  __sentinel->Right = nullptr;
  __sentinel->Parent = nullptr;
  __sentinel->Color = 1;
  __rb_tree = __sentinel;
  __last_found_node = __sentinel;
}

EventTree::~EventTree()
{
  if (__sentinel)
    __sentinel->release();
}

///<summary>
/// RestoreAfterInsert
/// Additions to red-black trees usually destroy the red-black
/// properties. Examine the tree and restore. Rotations are normally
/// required to restore it
///</summary>
force_inline void
EventTree::RestoreAfterInsert(EventTreeNode* x)
{
  // x and y are used as variable names for brevity, in a more formal
  // implementation, you should probably change the names

  EventTreeNode* y;

  // maintain red-black tree properties after adding x
  while (x != __rb_tree && x->Parent->Color == 0)
  {
    // Parent node is .Colored red;
    if (x->Parent == x->Parent->Parent->Left)  // determine traversal path
    {                    // is it on the Left or Right subtree?
      y = x->Parent->Parent->Right;      // get uncle
      if (y != nullptr && y->Color == 0)
      {  // uncle is red; change x's Parent and uncle to black
        x->Parent->Color = 1;
        y->Color = 1;
        // grandparent must be red. Why? Every red node that is not
        // a leaf has only black children
        x->Parent->Parent->Color = 0;
        x = x->Parent->Parent;  // continue loop with grandparent
      }
      else
      {
        // uncle is black; determine if x is greater than Parent
        if (x == x->Parent->Right)
        {  // yes, x is greater than Parent; rotate Left
          // make x a Left child
          x = x->Parent;
          RotateLeft(x);
        }
        // no, x is less than Parent
        x->Parent->Color = 1;  // make Parent black
        x->Parent->Parent->Color = 0;    // make grandparent black
        RotateRight(x->Parent->Parent);          // rotate right
      }
    }
    else
    {  // x's Parent is on the Right subtree
      // this code is the same as above with "Left" and "Right" swapped
      y = x->Parent->Parent->Left;
      if (y != nullptr && y->Color == 0)
      {
        x->Parent->Color = 1;
        y->Color = 1;
        x->Parent->Parent->Color = 0;
        x = x->Parent->Parent;
      }
      else
      {
        if (x == x->Parent->Left)
        {
          x = x->Parent;
          RotateRight(x);
        }
        x->Parent->Color = 1;
        x->Parent->Parent->Color = 0;
        RotateLeft(x->Parent->Parent);
      }
    }
  }
  __rb_tree->Color = 1;    // rbTree should always be black
}
///<summary>
/// Delete
/// Delete a node from the tree and restore red black properties
///<summary>
force_inline void
EventTree::Delete(EventTreeNode* z)
{
  // A node to be deleted will be:
  //    1. a leaf with no children
  //    2. have one child
  //    3. have two children
  // If the deleted node is red, the red black properties still hold.
  // If the deleted node is black, the tree needs rebalancing

  EventTreeNode* x;          // work node to contain the replacement node
  EventTreeNode* y;          // work node

  // find the replacement node (the successor to x) - the node one with
  // at *most* one child.
  if (z->Left == __sentinel || z->Right == __sentinel)
    y = z;            // node has sentinel as a child
  else
  {
    // z has two children, find replacement node which will
    // be the leftmost node greater than z
    y = z->Right;                // traverse right subtree
    while (y->Left != __sentinel)    // to find next node in sequence
      y = y->Left;
  }

  // at this point, y contains the replacement node. it's content will be copied
  // to the valules in the node to be deleted

  // x (y's only child) is the node that will be linked to y's old parent.
  if (y->Left != __sentinel)
    x = y->Left;
  else
    x = y->Right;

  // replace x's parent with y's parent and
  // link x to proper subtree in parent
  // this removes y from the chain
  x->Parent = y->Parent;
  if (y->Parent != nullptr)
    if (y == y->Parent->Left)
      y->Parent->Left = x;
    else
      y->Parent->Right = x;
  else
    __rb_tree = x;      // make x the root node

  // copy the values from y (the replacement node) to the node being deleted.
  // note: this effectively deletes the node.
  if (y != z)
  {
    z->Key = y->Key;
    z->FirstSimEvent = y->FirstSimEvent;
    z->LastSimEvent = y->LastSimEvent;
  }

  if (y->Color == 1)
    Restore_after_delete(x);

  __last_found_node = __sentinel;

  z->release();
}

///<summary>
/// Restore_after_delete
/// Deletions from red-black trees may destroy the red-black
/// properties. Examine the tree and restore. Rotations are normally
/// required to restore it
///</summary>
force_inline void
EventTree::Restore_after_delete(EventTreeNode* x)
{
  // maintain Red-Black tree balance after deleting node

  EventTreeNode* y;

  while (x != __rb_tree && x->Color == 1)
  {
    if (x == x->Parent->Left)      // determine sub tree from parent
    {
      y = x->Parent->Right;      // y is x's sibling
      if (y->Color == 0)
      {  // x is black, y is red - make both black and rotate
        y->Color = 1;
        x->Parent->Color = 0;
        RotateLeft(x->Parent);
        y = x->Parent->Right;
      }
      if (y->Left->Color == 1 &&
          y->Right->Color == 1)
      {  // children are both black
        y->Color = 0;    // change parent to red
        x = x->Parent;          // move up the tree
      }
      else
      {
        if (y->Right->Color == 1)
        {
          y->Left->Color = 1;
          y->Color = 0;
          RotateRight(y);
          y = x->Parent->Right;
        }
        y->Color = x->Parent->Color;
        x->Parent->Color = 1;
        y->Right->Color = 1;
        RotateLeft(x->Parent);
        x = __rb_tree;
      }
    }
    else
    {  // right subtree - same as code above with right and left swapped
      y = x->Parent->Left;
      if (y->Color == 0)
      {
        y->Color = 1;
        x->Parent->Color = 0;
        RotateRight(x->Parent);
        y = x->Parent->Left;
      }
      if (y->Right->Color == 1 &&
          y->Left->Color == 1)
      {
        y->Color = 0;
        x = x->Parent;
      }
      else
      {
        if (y->Left->Color == 1)
        {
          y->Right->Color = 1;
          y->Color = 0;
          RotateLeft(y);
          y = x->Parent->Left;
        }
        y->Color = x->Parent->Color;
        x->Parent->Color = 1;
        y->Left->Color = 1;
        RotateRight(x->Parent);
        x = __rb_tree;
      }
    }
  }
  x->Color = 1;
}

///<summary>
/// Add
/// args: ByVal key As IComparable, ByVal data As Object
/// key is object that implements IComparable interface
/// performance tip: change to use use int type (such as the hashcode)
///</summary>
void
EventTree::Add(sim_time_type key, SimEvent* data)
{
  // traverse tree - find where node belongs
  // create new node
  EventTreeNode* node = __pool.construct();
  EventTreeNode* temp = __rb_tree;        // grab the rbTree node of the tree

  while (temp != __sentinel)
  {  // find Parent
    node->Parent = temp;

    if (key > temp->Key)
      temp = temp->Right;
    else
      temp = temp->Left;
  }

  // setup node
  node->Key = key;
  node->FirstSimEvent = data;
  node->LastSimEvent = data;
  node->Left = __sentinel;
  node->Right = __sentinel;

  // insert node into tree starting at parent's location
  if (node->Parent != nullptr)
  {
    if (node->Key > (node->Parent->Key))
      node->Parent->Right = node;
    else
      node->Parent->Left = node;
  }
  else
    __rb_tree = node;          // first node added

  RestoreAfterInsert(node);           // restore red-black properities

  __last_found_node = node;

  __count++;
}
///<summary>
/// RotateLeft
/// Rebalance the tree by rotating the nodes to the left
///</summary>
void
EventTree::RotateLeft(EventTreeNode* x)
{
  // pushing node x down and to the Left to balance the tree. x's Right child (y)
  // replaces x (since y > x), and y's Left child becomes x's Right child
  // (since it's < y but > x).

  EventTreeNode* y = x->Right;      // get x's Right node, this becomes y

                      // set x's Right link
  x->Right = y->Left;          // y's Left child's becomes x's Right child

                    // modify parents
  if (y->Left != __sentinel)
    y->Left->Parent = x;        // sets y's Left Parent to x

  if (y != __sentinel)
    y->Parent = x->Parent;      // set y's Parent to x's Parent

  if (x->Parent != nullptr)
  {  // determine which side of it's Parent x was on
    if (x == x->Parent->Left)
      x->Parent->Left = y;      // set Left Parent to y
    else
      x->Parent->Right = y;      // set Right Parent to y
  }
  else
    __rb_tree = y;            // at rbTree, set it to y

                    // link x and y
  y->Left = x;              // put x on y's Left
  if (x != __sentinel)            // set y as x's Parent
    x->Parent = y;
}
///<summary>
/// RotateRight
/// Rebalance the tree by rotating the nodes to the right
///</summary>
void
EventTree::RotateRight(EventTreeNode* x)
{
  // pushing node x down and to the Right to balance the tree. x's Left child (y)
  // replaces x (since x < y), and y's Right child becomes x's Left child
  // (since it's < x but > y).

  EventTreeNode* y = x->Left;      // get x's Left node, this becomes y

                      // set x's Right link
  x->Left = y->Right;          // y's Right child becomes x's Left child

                    // modify parents
  if (y->Right != __sentinel)
    y->Right->Parent = x;        // sets y's Right Parent to x

  if (y != __sentinel)
    y->Parent = x->Parent;      // set y's Parent to x's Parent

  if (x->Parent != nullptr)        // null=rbTree, could also have used rbTree
  {  // determine which side of it's Parent x was on
    if (x == x->Parent->Right)
      x->Parent->Right = y;      // set Right Parent to y
    else
      x->Parent->Left = y;      // set Left Parent to y
  }
  else
    __rb_tree = y;            // at rbTree, set it to y

                    // link x and y
  y->Right = x;            // put x on y's Right
  if (x != __sentinel)        // set y as x's Parent
    x->Parent = y;
}
///<summary>
/// GetData
/// Gets the data object associated with the specified key
///<summary>
SimEvent*
EventTree::GetData(sim_time_type key)
{
  EventTreeNode* treeNode = __rb_tree;     // begin at root
  // traverse tree until node is found
  while (treeNode != __sentinel)
  {
    if (key == treeNode->Key)
    {
      __last_found_node = treeNode;
      return treeNode->FirstSimEvent;
    }
    if (key < (treeNode->Key))
      treeNode = treeNode->Left;
    else
      treeNode = treeNode->Right;
  }
  return nullptr;
}

void
EventTree::Insert_sim_event(SimEvent* event)
{
  if (event->Fire_time < Engine::Instance()->Time())
    throw mqsim_error("Illegal request to register a simulation event before Now!");

  sim_time_type key = event->Fire_time;
  EventTreeNode* treeNode = __rb_tree;     // begin at root

                      // traverse tree until node is found
  while (treeNode != __sentinel)
  {
    if (key == treeNode->Key)
    {
      treeNode->LastSimEvent->Next_event = event;
      treeNode->LastSimEvent = event;
      return;
    }
    if (key < (treeNode->Key))
      treeNode = treeNode->Left;
    else
      treeNode = treeNode->Right;
  }
  Add(event->Fire_time, event);
}

///<summary>
/// Get_min_value
/// Returns the object having the minimum key value
///<summary>
EventTreeNode*
EventTree::Get_min_node()
{
  EventTreeNode* treeNode = __rb_tree;

  // traverse to the extreme left to find the smallest key
  while (treeNode->Left != __sentinel)
    treeNode = treeNode->Left;

  __last_found_node = treeNode;

  return treeNode;
}

///<summary>
/// Remove
/// removes the key and data object (delete)
///<summary>
void
EventTree::Remove(sim_time_type key)
{
  // find node
  EventTreeNode* node;

  // see if node to be deleted was the last one found
  if (key == __last_found_node->Key)
    node = __last_found_node;
  else
  {  // not found, must search
    node = __rb_tree;
    while (node != __sentinel)
    {
      if (key == __last_found_node->Key)
        break;
      if (key < __last_found_node->Key)
        node = node->Left;
      else
        node = node->Right;
    }

    if (node == __sentinel)
      return;        // key not found
  }

  Delete(node);

  __count--;
}

void
EventTree::Remove(EventTreeNode* node)
{
  Delete(node);
  __count--;
}

