#include "MyZMQ.h"

#include "MainClient.h"

#include <shift/coreclient/FIXInitiator.h>

#include <shift/miscutils/terminal/Common.h>

/**
 * @brief Constructor for a MyZMQ instance
 * @param None
 */
MyZMQ::MyZMQ()
    : m_context(new zmq::context_t(1))
    , m_otherall_socket(new zmq::socket_t(*m_context, ZMQ_PUSH))
    , m_responder(new zmq::socket_t(*m_context, ZMQ_REP))
{
    // set high water mark to 1 message
    // m_socket->setsockopt(23, 1);
    m_responder->connect("tcp://localhost:5550");
    m_otherall_socket->connect("tcp://localhost:5555");
}

/**
 * @brief Method to get the singleton instance of MyZMQ.
 * @param None
 * @return MyZMQ&: reference of current MyZMQ instance
 */
MyZMQ& MyZMQ::getInstance()
{
    static MyZMQ instance;
    return instance;
}

/**
 * @brief Method to send data to the frontend
 * @param msg: the message to send
 * @return None
 */
void MyZMQ::send(std::string msg)
{
    std::lock_guard<std::mutex> lock(m_mutex_pushpull);
    try {
        zmq::message_t jsondata(msg.length());
        std::memcpy(jsondata.data(), msg.c_str(), msg.length());
        m_otherall_socket->send(jsondata);
    } catch (const std::exception& e) {
        cout << e.what() << endl;
    } catch (...) {
        cout << "MyZMQ Exception" << endl;
    }
}

/**
 * @brief Method to receive request and send reply to frontend
 * @param None
 * @return None
 */
void MyZMQ::receiveReq()
{
    // using dynamic cast to cast pointer to CoreClient to MainClient so that we can call it's member function
    MainClient* mainClient = dynamic_cast<MainClient*>(shift::FIXInitiator::getInstance().getSuperUser());

    std::lock_guard<std::mutex> lock(m_mutex_reqrep);
    zmq::message_t request;
    m_responder->recv(&request);
    mainClient->sendOnce((char*)request.data());
    zmq::message_t reply(5);
    memcpy(reply.data(), "World", 5);
    m_responder->send(reply);
}
