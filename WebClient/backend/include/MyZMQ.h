#pragma once

#include <mutex>
#include <string>

#include <zmq.hpp>

/**
 * @brief MyZMQ class works as a ZMQ Client, which send message to frontend.
 */
class MyZMQ {
public:
    ~MyZMQ();

    static auto getInstance() -> MyZMQ&;

    void send(const std::string& message);
    void receiveReq();

private:
    MyZMQ(); // singleton pattern
    MyZMQ(const MyZMQ&) = delete; // forbid copying
    auto operator=(const MyZMQ&) -> MyZMQ& = delete; // forbid assigning

    zmq::context_t m_context;
    zmq::socket_t m_otherall_socket;
    zmq::socket_t m_responder;
    mutable std::mutex m_mutex_pushpull;
    mutable std::mutex m_mutex_reqrep;
    int m_num = 0;
};
