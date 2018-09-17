#include "include/app.h"

int main(int argc, char* argv[])
{
    // register types to Qt signal & slot
    qRegisterMetaType<QVector<shift::OrderBookEntry>>("QVector<shift::OrderBookEntry>");
    qRegisterMetaType<QVector<shift::Order>>("QVector<shift::Order>");
    qRegisterMetaType<std::string>("std::string");

    App a(argc, argv);
    return a.exec();
}
