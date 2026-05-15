#pragma once
#include "Net_Common.h"

namespace olc {
    namespace net {
        template<typename T>
        struct tsQueue {
        private:
            std::deque<T> dq;
            std::mutex key;
            std::condition_variable cvBlocking;
            std::mutex muxBlocking;

        public:
            tsQueue() = default;
            tsQueue(const tsQueue<T>&) = delete;
            virtual ~tsQueue() { clear(); }

        public:
            T front() {
                std::scoped_lock lock(key);
                if (dq.empty()) return T();
                return dq.front();
            }

            T back() {
                std::scoped_lock lock(key);
                if (dq.empty()) return T();
                return dq.back();
            }

            T pop_front() {
                std::scoped_lock lock(key);
                if (dq.empty()) return T();
                auto t = std::move(dq.front());
                dq.pop_front();
                return t;
            }

            void push_back(const T& item) {
                std::scoped_lock lock(key);
                dq.emplace_back(std::move(item));

                std::unique_lock<std::mutex> ul(muxBlocking);
                cvBlocking.notify_one();
            }

            bool empty() {
                std::scoped_lock lock(key);
                return dq.empty();
            }

            size_t count() {
                std::scoped_lock lock(key);
                return dq.size();
            }

            void clear() {
                std::scoped_lock lock(key);
                dq.clear();
            }

            void wait() {
                while (empty()) {
                    std::unique_lock<std::mutex> ul(muxBlocking);
                    cvBlocking.wait(ul);
                }
            }
        }; 
    }
}