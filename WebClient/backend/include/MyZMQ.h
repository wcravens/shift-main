#pragma once

#include <mutex>
#include <string>

#include <zmq.hpp>

/**
 * @brief MyZMQ class works as a ZMQ Client, which send message to frontend.
 */
class MyZMQ {
public:
    static MyZMQ& getInstance();

    MyZMQ(MyZMQ const&) = delete;
    void operator=(MyZMQ const&) = delete;

    void send(std::string str);
    void receiveReq();

private:
    MyZMQ();
    ~MyZMQ();

    zmq::context_t m_context;
    zmq::socket_t m_otherall_socket;
    zmq::socket_t m_responder;
    std::mutex m_mutex_pushpull;
    std::mutex m_mutex_reqrep;
    int m_num = 0;
};
