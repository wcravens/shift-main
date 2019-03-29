#pragma once

#include <string>

class RawData {

private:
    std::string m_ticker;
    std::string m_dateTime;
    std::string m_orderType; // quotes "Q" or trades "T"
    std::string m_buyerID;
    double m_bidPrice;
    int m_bidSize;
    std::string m_sellerID;
    double m_offerPrice;
    int m_offerSize;
    std::string m_recordID;

public:
    RawData() {}

    ~RawData() {}

    // Quote
    RawData(std::string ticker,
        std::string dateTime,
        std::string orderType,
        std::string buyerID,
        double bidPrice,
        int bidSize,
        std::string sellerID,
        double offerPrice,
        int offerSize,
        std::string recordID)
        : m_ticker(std::move(ticker))
        , m_dateTime(std::move(dateTime))
        , m_orderType(std::move(orderType))
        , m_buyerID(std::move(buyerID))
        , m_bidPrice(bidPrice)
        , m_bidSize(bidSize)
        , m_sellerID(std::move(m_sellerID))
        , m_offerPrice(offerPrice)
        , m_offerSize(offerSize)
        , m_recordID(std::move(recordID))
    {
    }

    // Trade
    RawData(std::string ticker,
        std::string dateTime,
        std::string orderType,
        std::string buyerID,
        double bidPrice,
        int bidSize,
        std::string sellerID,
        double offerPrice,
        int offerSize,
        std::string recordID)
        : m_ticker(std::move(ticker))
        , m_dateTime(std::move(dateTime))
        , m_orderType(std::move(orderType))
        , m_buyerID(std::move(buyerID))
        , m_bidPrice(bidPrice)
        , m_bidSize(bidSize)
        , m_recordID(std::move(recordID))
    {
    }

    void setTicker(std::string ticker)
    {
        m_ticker = ticker;
    }

    std::string getTicker()
    {
        return m_ticker;
    }

    void setDateTime(std::string dateTime)
    {
        m_dateTime = dateTime;
    }

    std::string getDateTime()
    {
        return m_dateTime;
    }

    void setOrderType(std::string orderType)
    {
        m_orderType = orderType;
    }

    std::string getOrderType()
    {
        return m_orderType;
    }

    void setBuyerID(std::string buyerID)
    {
        m_buyerID = buyerID;
    }

    std::string getBuyerID()
    {
        return m_buyerID;
    }

    void setBidPrice(double bidPrice)
    {
        m_bidPrice = bidPrice;
    }

    double getBidPrice()
    {
        return m_bidPrice;
    }

    void setBidSize(int bidSize)
    {
        m_bidSize = bidSize;
    }

    int getBidSize()
    {
        return m_bidSize;
    }

    void setSellerID(std::string sellerID)
    {
        m_sellerID = sellerID;
    }

    std::string getSellerID()
    {
        return m_sellerID;
    }

    void setOfferPrice(double offerPrice)
    {
        m_offerPrice = offerPrice;
    }

    double getOfferPrice()
    {
        return m_offerPrice;
    }

    void setOfferSize(int offerSize)
    {
        m_offerSize = offerSize;
    }

    int getOfferSize()
    {
        return m_offerSize;
    }

    void setRecordID(std::string recordID)
    {
        m_recordID = recordID;
    }

    std::string getRecordID()
    {
        return m_recordID;
    }
};
