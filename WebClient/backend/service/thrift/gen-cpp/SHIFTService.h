/**
 * Autogenerated by Thrift Compiler (0.11.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef SHIFTService_H
#define SHIFTService_H

#include <thrift/TDispatchProcessor.h>
#include <thrift/async/TConcurrentClientSyncInfo.h>
#include "shift_service_types.h"



#ifdef _MSC_VER
  #pragma warning( push )
  #pragma warning (disable : 4250 ) //inheriting methods via dominance 
#endif

class SHIFTServiceIf {
 public:
  virtual ~SHIFTServiceIf() {}
  virtual void submitOrder(const std::string& stockName, const std::string& orderID, const double price, const int32_t shareSize, const std::string& orderType, const std::string& username) = 0;
  virtual void webClientSendUsername(const std::string& username) = 0;
  virtual void webUserLogin(const std::string& username) = 0;
  virtual void startStrategy(const std::string& stockName, const double price, const int32_t shareSize, const std::string& orderType, const std::string& username, const int32_t interval) = 0;
};

class SHIFTServiceIfFactory {
 public:
  typedef SHIFTServiceIf Handler;

  virtual ~SHIFTServiceIfFactory() {}

  virtual SHIFTServiceIf* getHandler(const ::apache::thrift::TConnectionInfo& connInfo) = 0;
  virtual void releaseHandler(SHIFTServiceIf* /* handler */) = 0;
};

class SHIFTServiceIfSingletonFactory : virtual public SHIFTServiceIfFactory {
 public:
  SHIFTServiceIfSingletonFactory(const ::apache::thrift::stdcxx::shared_ptr<SHIFTServiceIf>& iface) : iface_(iface) {}
  virtual ~SHIFTServiceIfSingletonFactory() {}

  virtual SHIFTServiceIf* getHandler(const ::apache::thrift::TConnectionInfo&) {
    return iface_.get();
  }
  virtual void releaseHandler(SHIFTServiceIf* /* handler */) {}

 protected:
  ::apache::thrift::stdcxx::shared_ptr<SHIFTServiceIf> iface_;
};

class SHIFTServiceNull : virtual public SHIFTServiceIf {
 public:
  virtual ~SHIFTServiceNull() {}
  void submitOrder(const std::string& /* stockName */, const std::string& /* orderID */, const double /* price */, const int32_t /* shareSize */, const std::string& /* orderType */, const std::string& /* username */) {
    return;
  }
  void webClientSendUsername(const std::string& /* username */) {
    return;
  }
  void webUserLogin(const std::string& /* username */) {
    return;
  }
  void startStrategy(const std::string& /* stockName */, const double /* price */, const int32_t /* shareSize */, const std::string& /* orderType */, const std::string& /* username */, const int32_t /* interval */) {
    return;
  }
};

typedef struct _SHIFTService_submitOrder_args__isset {
  _SHIFTService_submitOrder_args__isset() : stockName(false), orderID(false), price(false), shareSize(false), orderType(false), username(false) {}
  bool stockName :1;
  bool orderID :1;
  bool price :1;
  bool shareSize :1;
  bool orderType :1;
  bool username :1;
} _SHIFTService_submitOrder_args__isset;

class SHIFTService_submitOrder_args {
 public:

  SHIFTService_submitOrder_args(const SHIFTService_submitOrder_args&);
  SHIFTService_submitOrder_args& operator=(const SHIFTService_submitOrder_args&);
  SHIFTService_submitOrder_args() : stockName(), orderID(), price(0), shareSize(0), orderType(), username() {
  }

  virtual ~SHIFTService_submitOrder_args() throw();
  std::string stockName;
  std::string orderID;
  double price;
  int32_t shareSize;
  std::string orderType;
  std::string username;

  _SHIFTService_submitOrder_args__isset __isset;

  void __set_stockName(const std::string& val);

  void __set_orderID(const std::string& val);

  void __set_price(const double val);

  void __set_shareSize(const int32_t val);

  void __set_orderType(const std::string& val);

  void __set_username(const std::string& val);

  bool operator == (const SHIFTService_submitOrder_args & rhs) const
  {
    if (!(stockName == rhs.stockName))
      return false;
    if (!(orderID == rhs.orderID))
      return false;
    if (!(price == rhs.price))
      return false;
    if (!(shareSize == rhs.shareSize))
      return false;
    if (!(orderType == rhs.orderType))
      return false;
    if (!(username == rhs.username))
      return false;
    return true;
  }
  bool operator != (const SHIFTService_submitOrder_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SHIFTService_submitOrder_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class SHIFTService_submitOrder_pargs {
 public:


  virtual ~SHIFTService_submitOrder_pargs() throw();
  const std::string* stockName;
  const std::string* orderID;
  const double* price;
  const int32_t* shareSize;
  const std::string* orderType;
  const std::string* username;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class SHIFTService_submitOrder_result {
 public:

  SHIFTService_submitOrder_result(const SHIFTService_submitOrder_result&);
  SHIFTService_submitOrder_result& operator=(const SHIFTService_submitOrder_result&);
  SHIFTService_submitOrder_result() {
  }

  virtual ~SHIFTService_submitOrder_result() throw();

  bool operator == (const SHIFTService_submitOrder_result & /* rhs */) const
  {
    return true;
  }
  bool operator != (const SHIFTService_submitOrder_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SHIFTService_submitOrder_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class SHIFTService_submitOrder_presult {
 public:


  virtual ~SHIFTService_submitOrder_presult() throw();

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};

typedef struct _SHIFTService_webClientSendUsername_args__isset {
  _SHIFTService_webClientSendUsername_args__isset() : username(false) {}
  bool username :1;
} _SHIFTService_webClientSendUsername_args__isset;

class SHIFTService_webClientSendUsername_args {
 public:

  SHIFTService_webClientSendUsername_args(const SHIFTService_webClientSendUsername_args&);
  SHIFTService_webClientSendUsername_args& operator=(const SHIFTService_webClientSendUsername_args&);
  SHIFTService_webClientSendUsername_args() : username() {
  }

  virtual ~SHIFTService_webClientSendUsername_args() throw();
  std::string username;

  _SHIFTService_webClientSendUsername_args__isset __isset;

  void __set_username(const std::string& val);

  bool operator == (const SHIFTService_webClientSendUsername_args & rhs) const
  {
    if (!(username == rhs.username))
      return false;
    return true;
  }
  bool operator != (const SHIFTService_webClientSendUsername_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SHIFTService_webClientSendUsername_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class SHIFTService_webClientSendUsername_pargs {
 public:


  virtual ~SHIFTService_webClientSendUsername_pargs() throw();
  const std::string* username;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class SHIFTService_webClientSendUsername_result {
 public:

  SHIFTService_webClientSendUsername_result(const SHIFTService_webClientSendUsername_result&);
  SHIFTService_webClientSendUsername_result& operator=(const SHIFTService_webClientSendUsername_result&);
  SHIFTService_webClientSendUsername_result() {
  }

  virtual ~SHIFTService_webClientSendUsername_result() throw();

  bool operator == (const SHIFTService_webClientSendUsername_result & /* rhs */) const
  {
    return true;
  }
  bool operator != (const SHIFTService_webClientSendUsername_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SHIFTService_webClientSendUsername_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class SHIFTService_webClientSendUsername_presult {
 public:


  virtual ~SHIFTService_webClientSendUsername_presult() throw();

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};

typedef struct _SHIFTService_webUserLogin_args__isset {
  _SHIFTService_webUserLogin_args__isset() : username(false) {}
  bool username :1;
} _SHIFTService_webUserLogin_args__isset;

class SHIFTService_webUserLogin_args {
 public:

  SHIFTService_webUserLogin_args(const SHIFTService_webUserLogin_args&);
  SHIFTService_webUserLogin_args& operator=(const SHIFTService_webUserLogin_args&);
  SHIFTService_webUserLogin_args() : username() {
  }

  virtual ~SHIFTService_webUserLogin_args() throw();
  std::string username;

  _SHIFTService_webUserLogin_args__isset __isset;

  void __set_username(const std::string& val);

  bool operator == (const SHIFTService_webUserLogin_args & rhs) const
  {
    if (!(username == rhs.username))
      return false;
    return true;
  }
  bool operator != (const SHIFTService_webUserLogin_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SHIFTService_webUserLogin_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class SHIFTService_webUserLogin_pargs {
 public:


  virtual ~SHIFTService_webUserLogin_pargs() throw();
  const std::string* username;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class SHIFTService_webUserLogin_result {
 public:

  SHIFTService_webUserLogin_result(const SHIFTService_webUserLogin_result&);
  SHIFTService_webUserLogin_result& operator=(const SHIFTService_webUserLogin_result&);
  SHIFTService_webUserLogin_result() {
  }

  virtual ~SHIFTService_webUserLogin_result() throw();

  bool operator == (const SHIFTService_webUserLogin_result & /* rhs */) const
  {
    return true;
  }
  bool operator != (const SHIFTService_webUserLogin_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SHIFTService_webUserLogin_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class SHIFTService_webUserLogin_presult {
 public:


  virtual ~SHIFTService_webUserLogin_presult() throw();

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};

typedef struct _SHIFTService_startStrategy_args__isset {
  _SHIFTService_startStrategy_args__isset() : stockName(false), price(false), shareSize(false), orderType(false), username(false), interval(false) {}
  bool stockName :1;
  bool price :1;
  bool shareSize :1;
  bool orderType :1;
  bool username :1;
  bool interval :1;
} _SHIFTService_startStrategy_args__isset;

class SHIFTService_startStrategy_args {
 public:

  SHIFTService_startStrategy_args(const SHIFTService_startStrategy_args&);
  SHIFTService_startStrategy_args& operator=(const SHIFTService_startStrategy_args&);
  SHIFTService_startStrategy_args() : stockName(), price(0), shareSize(0), orderType(), username(), interval(0) {
  }

  virtual ~SHIFTService_startStrategy_args() throw();
  std::string stockName;
  double price;
  int32_t shareSize;
  std::string orderType;
  std::string username;
  int32_t interval;

  _SHIFTService_startStrategy_args__isset __isset;

  void __set_stockName(const std::string& val);

  void __set_price(const double val);

  void __set_shareSize(const int32_t val);

  void __set_orderType(const std::string& val);

  void __set_username(const std::string& val);

  void __set_interval(const int32_t val);

  bool operator == (const SHIFTService_startStrategy_args & rhs) const
  {
    if (!(stockName == rhs.stockName))
      return false;
    if (!(price == rhs.price))
      return false;
    if (!(shareSize == rhs.shareSize))
      return false;
    if (!(orderType == rhs.orderType))
      return false;
    if (!(username == rhs.username))
      return false;
    if (!(interval == rhs.interval))
      return false;
    return true;
  }
  bool operator != (const SHIFTService_startStrategy_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SHIFTService_startStrategy_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class SHIFTService_startStrategy_pargs {
 public:


  virtual ~SHIFTService_startStrategy_pargs() throw();
  const std::string* stockName;
  const double* price;
  const int32_t* shareSize;
  const std::string* orderType;
  const std::string* username;
  const int32_t* interval;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class SHIFTService_startStrategy_result {
 public:

  SHIFTService_startStrategy_result(const SHIFTService_startStrategy_result&);
  SHIFTService_startStrategy_result& operator=(const SHIFTService_startStrategy_result&);
  SHIFTService_startStrategy_result() {
  }

  virtual ~SHIFTService_startStrategy_result() throw();

  bool operator == (const SHIFTService_startStrategy_result & /* rhs */) const
  {
    return true;
  }
  bool operator != (const SHIFTService_startStrategy_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const SHIFTService_startStrategy_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class SHIFTService_startStrategy_presult {
 public:


  virtual ~SHIFTService_startStrategy_presult() throw();

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};

class SHIFTServiceClient : virtual public SHIFTServiceIf {
 public:
  SHIFTServiceClient(apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> prot) {
    setProtocol(prot);
  }
  SHIFTServiceClient(apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot) {
    setProtocol(iprot,oprot);
  }
 private:
  void setProtocol(apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> prot) {
  setProtocol(prot,prot);
  }
  void setProtocol(apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot) {
    piprot_=iprot;
    poprot_=oprot;
    iprot_ = iprot.get();
    oprot_ = oprot.get();
  }
 public:
  apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> getInputProtocol() {
    return piprot_;
  }
  apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> getOutputProtocol() {
    return poprot_;
  }
  void submitOrder(const std::string& stockName, const std::string& orderID, const double price, const int32_t shareSize, const std::string& orderType, const std::string& username);
  void send_submitOrder(const std::string& stockName, const std::string& orderID, const double price, const int32_t shareSize, const std::string& orderType, const std::string& username);
  void recv_submitOrder();
  void webClientSendUsername(const std::string& username);
  void send_webClientSendUsername(const std::string& username);
  void recv_webClientSendUsername();
  void webUserLogin(const std::string& username);
  void send_webUserLogin(const std::string& username);
  void recv_webUserLogin();
  void startStrategy(const std::string& stockName, const double price, const int32_t shareSize, const std::string& orderType, const std::string& username, const int32_t interval);
  void send_startStrategy(const std::string& stockName, const double price, const int32_t shareSize, const std::string& orderType, const std::string& username, const int32_t interval);
  void recv_startStrategy();
 protected:
  apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> piprot_;
  apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> poprot_;
  ::apache::thrift::protocol::TProtocol* iprot_;
  ::apache::thrift::protocol::TProtocol* oprot_;
};

class SHIFTServiceProcessor : public ::apache::thrift::TDispatchProcessor {
 protected:
  ::apache::thrift::stdcxx::shared_ptr<SHIFTServiceIf> iface_;
  virtual bool dispatchCall(::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, const std::string& fname, int32_t seqid, void* callContext);
 private:
  typedef  void (SHIFTServiceProcessor::*ProcessFunction)(int32_t, ::apache::thrift::protocol::TProtocol*, ::apache::thrift::protocol::TProtocol*, void*);
  typedef std::map<std::string, ProcessFunction> ProcessMap;
  ProcessMap processMap_;
  void process_submitOrder(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_webClientSendUsername(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_webUserLogin(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_startStrategy(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
 public:
  SHIFTServiceProcessor(::apache::thrift::stdcxx::shared_ptr<SHIFTServiceIf> iface) :
    iface_(iface) {
    processMap_["submitOrder"] = &SHIFTServiceProcessor::process_submitOrder;
    processMap_["webClientSendUsername"] = &SHIFTServiceProcessor::process_webClientSendUsername;
    processMap_["webUserLogin"] = &SHIFTServiceProcessor::process_webUserLogin;
    processMap_["startStrategy"] = &SHIFTServiceProcessor::process_startStrategy;
  }

  virtual ~SHIFTServiceProcessor() {}
};

class SHIFTServiceProcessorFactory : public ::apache::thrift::TProcessorFactory {
 public:
  SHIFTServiceProcessorFactory(const ::apache::thrift::stdcxx::shared_ptr< SHIFTServiceIfFactory >& handlerFactory) :
      handlerFactory_(handlerFactory) {}

  ::apache::thrift::stdcxx::shared_ptr< ::apache::thrift::TProcessor > getProcessor(const ::apache::thrift::TConnectionInfo& connInfo);

 protected:
  ::apache::thrift::stdcxx::shared_ptr< SHIFTServiceIfFactory > handlerFactory_;
};

class SHIFTServiceMultiface : virtual public SHIFTServiceIf {
 public:
  SHIFTServiceMultiface(std::vector<apache::thrift::stdcxx::shared_ptr<SHIFTServiceIf> >& ifaces) : ifaces_(ifaces) {
  }
  virtual ~SHIFTServiceMultiface() {}
 protected:
  std::vector<apache::thrift::stdcxx::shared_ptr<SHIFTServiceIf> > ifaces_;
  SHIFTServiceMultiface() {}
  void add(::apache::thrift::stdcxx::shared_ptr<SHIFTServiceIf> iface) {
    ifaces_.push_back(iface);
  }
 public:
  void submitOrder(const std::string& stockName, const std::string& orderID, const double price, const int32_t shareSize, const std::string& orderType, const std::string& username) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->submitOrder(stockName, orderID, price, shareSize, orderType, username);
    }
    ifaces_[i]->submitOrder(stockName, orderID, price, shareSize, orderType, username);
  }

  void webClientSendUsername(const std::string& username) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->webClientSendUsername(username);
    }
    ifaces_[i]->webClientSendUsername(username);
  }

  void webUserLogin(const std::string& username) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->webUserLogin(username);
    }
    ifaces_[i]->webUserLogin(username);
  }

  void startStrategy(const std::string& stockName, const double price, const int32_t shareSize, const std::string& orderType, const std::string& username, const int32_t interval) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->startStrategy(stockName, price, shareSize, orderType, username, interval);
    }
    ifaces_[i]->startStrategy(stockName, price, shareSize, orderType, username, interval);
  }

};

// The 'concurrent' client is a thread safe client that correctly handles
// out of order responses.  It is slower than the regular client, so should
// only be used when you need to share a connection among multiple threads
class SHIFTServiceConcurrentClient : virtual public SHIFTServiceIf {
 public:
  SHIFTServiceConcurrentClient(apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> prot) {
    setProtocol(prot);
  }
  SHIFTServiceConcurrentClient(apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot) {
    setProtocol(iprot,oprot);
  }
 private:
  void setProtocol(apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> prot) {
  setProtocol(prot,prot);
  }
  void setProtocol(apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot) {
    piprot_=iprot;
    poprot_=oprot;
    iprot_ = iprot.get();
    oprot_ = oprot.get();
  }
 public:
  apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> getInputProtocol() {
    return piprot_;
  }
  apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> getOutputProtocol() {
    return poprot_;
  }
  void submitOrder(const std::string& stockName, const std::string& orderID, const double price, const int32_t shareSize, const std::string& orderType, const std::string& username);
  int32_t send_submitOrder(const std::string& stockName, const std::string& orderID, const double price, const int32_t shareSize, const std::string& orderType, const std::string& username);
  void recv_submitOrder(const int32_t seqid);
  void webClientSendUsername(const std::string& username);
  int32_t send_webClientSendUsername(const std::string& username);
  void recv_webClientSendUsername(const int32_t seqid);
  void webUserLogin(const std::string& username);
  int32_t send_webUserLogin(const std::string& username);
  void recv_webUserLogin(const int32_t seqid);
  void startStrategy(const std::string& stockName, const double price, const int32_t shareSize, const std::string& orderType, const std::string& username, const int32_t interval);
  int32_t send_startStrategy(const std::string& stockName, const double price, const int32_t shareSize, const std::string& orderType, const std::string& username, const int32_t interval);
  void recv_startStrategy(const int32_t seqid);
 protected:
  apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> piprot_;
  apache::thrift::stdcxx::shared_ptr< ::apache::thrift::protocol::TProtocol> poprot_;
  ::apache::thrift::protocol::TProtocol* iprot_;
  ::apache::thrift::protocol::TProtocol* oprot_;
  ::apache::thrift::async::TConcurrentClientSyncInfo sync_;
};

#ifdef _MSC_VER
  #pragma warning( pop )
#endif



#endif
