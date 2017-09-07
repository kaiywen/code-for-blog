#include <cstdlib>
#include <mutex>

#define CACHE_LINE_SIZE 64
#define CACHE_ALIGNED __attribute__(( __aligned__(CACHE_LINE_SIZE) ))

#define CAS(val, oldval, newval) \
    __sync_bool_compare_and_swap(&(val), oldval, newval)

template <typename T> struct QueueNode;

template <typename T>
struct NodePointer {
    QueueNode<T> *ptr_;
    int count_;
    NodePointer(QueueNode<T> ptr, int count) {
        ptr_ = ptr;
        count_ = count;
    }
} CACHE_ALIGNED;

template <typename T>
struct QueueNode {
    T val_;
    NodePointer<T> next_;
    QueueNode(T val, QueueNode<T> *node) {
        val_ = val;
        next_.ptr_ = node;
    }
};

template <typename T>
class LockFreeQueue {
private:
   NodePointer<T> head_;
   NodePointer<T> tail_;

public:
   LockFreeQueue(T val);
   bool Dequeue(T& val);
   void Enqueue(T val);
};

template <typename T>
LockFreeQueue<T>::LockFreeQueue(T val) {
    auto node = new QueueNode<T>(val, nullptr);
    head_.ptr_ = tail_.ptr_ = &node;
    head_.count_ = tail_.count_ = 1;
}

template <typename T>
void LockFreeQueue<T>::Enqueue(T val) {
    NodePointer<T> tail, next;
    auto node = new QueueNode<T>(val, nullptr);
    while (true) {
       tail = this->tail_;
       next = tail.ptr_->next_;
       if(tail == this->tail_) {
           if(next.ptr_ == nullptr) {
               // link the new node into the queue
               if(CAS(tail.ptr_->next_, next, NodePointer<T>(&node, next.count_+1))) {
                   break;
               }
           }
           else {
               // move the tail_ node forward
               CAS(this->tail_, tail, NodePointer<T>(next.ptr_, tail.count_+1));
           }
       }
    }
    // put the tail_ on its right position
    CAS(this->tail_, tail, NodePointer<T>(&node, tail.count_+1));
}

template <typename T> 
bool LockFreeQueue<T>::Dequeue(T& ret_val) {
    NodePointer<T> head, tail, next;
    while (true) {
        head = this->head_;
        tail = this->tail_;
        next = head.ptr_->next_;
        if(head == this->head_) {
            if(head.ptr_ == tail.ptr_) {
                if(next.ptr_ == nullptr) {
                    return false;
                }
                // the tail lags, move it forward
                CAS(this->tail_, tail, NodePointer<T>(next.ptr_, tail.count_+1));
            }
            else {
                ret_val = head.ptr_->val_; 
                if(CAS(this->head_, head, NodePointer<T>(next.ptr_, head.count_+1))) {
                    break;
                }
            }
        }
    }
    delete head.ptr_;
    return true;
}
