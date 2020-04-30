#include "MyZMQ.h"

#include "MainClient.h"

#include <shift/coreclient/FIXInitiator.h>

#include <shift/miscutils/terminal/Common.h>

/**
 * @brief Constructor for a MyZMQ instance.
 */
MyZMQ::MyZMQ()
    : m_context { 1 }
    , m_otherall_socket { m_context, ZMQ_PUSH }
    , m_responder { m_context, ZMQ_REP }
{
    // http://api.zeromq.org/master:zmq-setsockopt
    // ZMQ_LINGER: Set linger period for socket shutdown
    // ZMQ_RCVTIMEO: Maximum time before a recv operation returns with EAGAIN
    // ZMQ_SNDTIMEO: Maximum time before a send operation returns with EAGAIN

    m_otherall_socket.setsockopt(ZMQ_LINGER, 0); // 0s
    m_otherall_socket.setsockopt(ZMQ_SNDTIMEO, 10000); // 10s
    m_otherall_socket.connect("tcp://localhost:5555");

    m_responder.setsockopt(ZMQ_LINGER, 0); // 0s
    m_responder.setsockopt(ZMQ_RCVTIMEO, 10000); // 10s
    m_responder.setsockopt(ZMQ_SNDTIMEO, 10000); // 10s
    m_responder.connect("tcp://localhost:5550");
}

MyZMQ::~MyZMQ()
{
    m_responder.disconnect("tcp://localhost:5550");

    m_otherall_socket.disconnect("tcp://localhost:5555");
}

/**
 * @brief Method to get the singleton instance of MyZMQ.
 * @return Pointer to current MyZMQ instance.
 */
/* static */ auto MyZMQ::getInstance() -> MyZMQ&
{
    static MyZMQ s_MyZMQInst;
    return s_MyZMQInst;
}

/**
 * @brief Method to send data to the frontend.
 * @param message The message to send.
 */
void MyZMQ::send(const std::string& message)
{
    std::lock_guard<std::mutex> lock(m_mutex_pushpull);
    try {
        zmq::message_t jsondata(message.length());
        std::memcpy(jsondata.data(), message.c_str(), message.length());
#if (defined(CPPZMQ_VERSION) && CPPZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 3, 1))
        m_otherall_socket.send(jsondata, zmq::send_flags::none);
#else
        m_otherall_socket.send(jsondata);
#endif
    } catch (const std::exception& e) {
        cout << e.what() << endl;
    } catch (...) {
        cout << "MyZMQ Exception" << endl;
    }
}

/**
 * @brief Method to receive request and send reply to frontend.
 */
void MyZMQ::receiveReq()
{
    // using dynamic cast to cast from CoreClient* to
    // MainClient* so that we can call it's member functions
    MainClient* mainClient = dynamic_cast<MainClient*>(shift::FIXInitiator::getInstance().getSuperUser());

    std::lock_guard<std::mutex> lock(m_mutex_reqrep);
    zmq::message_t request;
#if (defined(CPPZMQ_VERSION) && CPPZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 3, 1))
    if (!m_responder.recv(request, zmq::recv_flags::none)) {
#else
    if (!m_responder.recv(&request)) { // http://api.zeromq.org/2-2:zmq-cpp
#endif
        return;
    }
    mainClient->sendOnce((const char*)request.data());
    zmq::message_t reply(5);
    memcpy(reply.data(), "World", 5);
#if (defined(CPPZMQ_VERSION) && CPPZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 3, 1))
    m_responder.send(reply, zmq::send_flags::none);
#else
    m_responder.send(reply);
#endif
}
