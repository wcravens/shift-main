/**
 * Autogenerated by Thrift Compiler (0.11.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#include "SHIFTService.h"




SHIFTService_submitOrder_args::~SHIFTService_submitOrder_args() throw() {
}


uint32_t SHIFTService_submitOrder_args::read(::apache::thrift::protocol::TProtocol* iprot) {

  ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 1:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->username);
          this->__isset.username = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 2:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->orderType);
          this->__isset.orderType = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 3:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->orderSymbol);
          this->__isset.orderSymbol = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 4:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->orderSize);
          this->__isset.orderSize = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 5:
        if (ftype == ::apache::thrift::protocol::T_DOUBLE) {
          xfer += iprot->readDouble(this->orderPrice);
          this->__isset.orderPrice = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 6:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->orderID);
          this->__isset.orderID = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

uint32_t SHIFTService_submitOrder_args::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
  xfer += oprot->writeStructBegin("SHIFTService_submitOrder_args");

  xfer += oprot->writeFieldBegin("username", ::apache::thrift::protocol::T_STRING, 1);
  xfer += oprot->writeString(this->username);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("orderType", ::apache::thrift::protocol::T_STRING, 2);
  xfer += oprot->writeString(this->orderType);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("orderSymbol", ::apache::thrift::protocol::T_STRING, 3);
  xfer += oprot->writeString(this->orderSymbol);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("orderSize", ::apache::thrift::protocol::T_I32, 4);
  xfer += oprot->writeI32(this->orderSize);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("orderPrice", ::apache::thrift::protocol::T_DOUBLE, 5);
  xfer += oprot->writeDouble(this->orderPrice);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("orderID", ::apache::thrift::protocol::T_STRING, 6);
  xfer += oprot->writeString(this->orderID);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}


SHIFTService_submitOrder_pargs::~SHIFTService_submitOrder_pargs() throw() {
}


uint32_t SHIFTService_submitOrder_pargs::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
  xfer += oprot->writeStructBegin("SHIFTService_submitOrder_pargs");

  xfer += oprot->writeFieldBegin("username", ::apache::thrift::protocol::T_STRING, 1);
  xfer += oprot->writeString((*(this->username)));
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("orderType", ::apache::thrift::protocol::T_STRING, 2);
  xfer += oprot->writeString((*(this->orderType)));
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("orderSymbol", ::apache::thrift::protocol::T_STRING, 3);
  xfer += oprot->writeString((*(this->orderSymbol)));
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("orderSize", ::apache::thrift::protocol::T_I32, 4);
  xfer += oprot->writeI32((*(this->orderSize)));
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("orderPrice", ::apache::thrift::protocol::T_DOUBLE, 5);
  xfer += oprot->writeDouble((*(this->orderPrice)));
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("orderID", ::apache::thrift::protocol::T_STRING, 6);
  xfer += oprot->writeString((*(this->orderID)));
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}


SHIFTService_submitOrder_result::~SHIFTService_submitOrder_result() throw() {
}


uint32_t SHIFTService_submitOrder_result::read(::apache::thrift::protocol::TProtocol* iprot) {

  ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    xfer += iprot->skip(ftype);
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

uint32_t SHIFTService_submitOrder_result::write(::apache::thrift::protocol::TProtocol* oprot) const {

  uint32_t xfer = 0;

  xfer += oprot->writeStructBegin("SHIFTService_submitOrder_result");

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}


SHIFTService_submitOrder_presult::~SHIFTService_submitOrder_presult() throw() {
}


uint32_t SHIFTService_submitOrder_presult::read(::apache::thrift::protocol::TProtocol* iprot) {

  ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    xfer += iprot->skip(ftype);
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}


SHIFTService_webClientSendUsername_args::~SHIFTService_webClientSendUsername_args() throw() {
}


uint32_t SHIFTService_webClientSendUsername_args::read(::apache::thrift::protocol::TProtocol* iprot) {

  ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 1:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->username);
          this->__isset.username = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

uint32_t SHIFTService_webClientSendUsername_args::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
  xfer += oprot->writeStructBegin("SHIFTService_webClientSendUsername_args");

  xfer += oprot->writeFieldBegin("username", ::apache::thrift::protocol::T_STRING, 1);
  xfer += oprot->writeString(this->username);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}


SHIFTService_webClientSendUsername_pargs::~SHIFTService_webClientSendUsername_pargs() throw() {
}


uint32_t SHIFTService_webClientSendUsername_pargs::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
  xfer += oprot->writeStructBegin("SHIFTService_webClientSendUsername_pargs");

  xfer += oprot->writeFieldBegin("username", ::apache::thrift::protocol::T_STRING, 1);
  xfer += oprot->writeString((*(this->username)));
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}


SHIFTService_webClientSendUsername_result::~SHIFTService_webClientSendUsername_result() throw() {
}


uint32_t SHIFTService_webClientSendUsername_result::read(::apache::thrift::protocol::TProtocol* iprot) {

  ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    xfer += iprot->skip(ftype);
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

uint32_t SHIFTService_webClientSendUsername_result::write(::apache::thrift::protocol::TProtocol* oprot) const {

  uint32_t xfer = 0;

  xfer += oprot->writeStructBegin("SHIFTService_webClientSendUsername_result");

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}


SHIFTService_webClientSendUsername_presult::~SHIFTService_webClientSendUsername_presult() throw() {
}


uint32_t SHIFTService_webClientSendUsername_presult::read(::apache::thrift::protocol::TProtocol* iprot) {

  ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    xfer += iprot->skip(ftype);
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}


SHIFTService_webUserLogin_args::~SHIFTService_webUserLogin_args() throw() {
}


uint32_t SHIFTService_webUserLogin_args::read(::apache::thrift::protocol::TProtocol* iprot) {

  ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 1:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->username);
          this->__isset.username = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

uint32_t SHIFTService_webUserLogin_args::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
  xfer += oprot->writeStructBegin("SHIFTService_webUserLogin_args");

  xfer += oprot->writeFieldBegin("username", ::apache::thrift::protocol::T_STRING, 1);
  xfer += oprot->writeString(this->username);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}


SHIFTService_webUserLogin_pargs::~SHIFTService_webUserLogin_pargs() throw() {
}


uint32_t SHIFTService_webUserLogin_pargs::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
  xfer += oprot->writeStructBegin("SHIFTService_webUserLogin_pargs");

  xfer += oprot->writeFieldBegin("username", ::apache::thrift::protocol::T_STRING, 1);
  xfer += oprot->writeString((*(this->username)));
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}


SHIFTService_webUserLogin_result::~SHIFTService_webUserLogin_result() throw() {
}


uint32_t SHIFTService_webUserLogin_result::read(::apache::thrift::protocol::TProtocol* iprot) {

  ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    xfer += iprot->skip(ftype);
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

uint32_t SHIFTService_webUserLogin_result::write(::apache::thrift::protocol::TProtocol* oprot) const {

  uint32_t xfer = 0;

  xfer += oprot->writeStructBegin("SHIFTService_webUserLogin_result");

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}


SHIFTService_webUserLogin_presult::~SHIFTService_webUserLogin_presult() throw() {
}


uint32_t SHIFTService_webUserLogin_presult::read(::apache::thrift::protocol::TProtocol* iprot) {

  ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    xfer += iprot->skip(ftype);
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

void SHIFTServiceClient::submitOrder(const std::string& username, const std::string& orderType, const std::string& orderSymbol, const int32_t orderSize, const double orderPrice, const std::string& orderID)
{
  send_submitOrder(username, orderType, orderSymbol, orderSize, orderPrice, orderID);
  recv_submitOrder();
}

void SHIFTServiceClient::send_submitOrder(const std::string& username, const std::string& orderType, const std::string& orderSymbol, const int32_t orderSize, const double orderPrice, const std::string& orderID)
{
  int32_t cseqid = 0;
  oprot_->writeMessageBegin("submitOrder", ::apache::thrift::protocol::T_CALL, cseqid);

  SHIFTService_submitOrder_pargs args;
  args.username = &username;
  args.orderType = &orderType;
  args.orderSymbol = &orderSymbol;
  args.orderSize = &orderSize;
  args.orderPrice = &orderPrice;
  args.orderID = &orderID;
  args.write(oprot_);

  oprot_->writeMessageEnd();
  oprot_->getTransport()->writeEnd();
  oprot_->getTransport()->flush();
}

void SHIFTServiceClient::recv_submitOrder()
{

  int32_t rseqid = 0;
  std::string fname;
  ::apache::thrift::protocol::TMessageType mtype;

  iprot_->readMessageBegin(fname, mtype, rseqid);
  if (mtype == ::apache::thrift::protocol::T_EXCEPTION) {
    ::apache::thrift::TApplicationException x;
    x.read(iprot_);
    iprot_->readMessageEnd();
    iprot_->getTransport()->readEnd();
    throw x;
  }
  if (mtype != ::apache::thrift::protocol::T_REPLY) {
    iprot_->skip(::apache::thrift::protocol::T_STRUCT);
    iprot_->readMessageEnd();
    iprot_->getTransport()->readEnd();
  }
  if (fname.compare("submitOrder") != 0) {
    iprot_->skip(::apache::thrift::protocol::T_STRUCT);
    iprot_->readMessageEnd();
    iprot_->getTransport()->readEnd();
  }
  SHIFTService_submitOrder_presult result;
  result.read(iprot_);
  iprot_->readMessageEnd();
  iprot_->getTransport()->readEnd();

  return;
}

void SHIFTServiceClient::webClientSendUsername(const std::string& username)
{
  send_webClientSendUsername(username);
  recv_webClientSendUsername();
}

void SHIFTServiceClient::send_webClientSendUsername(const std::string& username)
{
  int32_t cseqid = 0;
  oprot_->writeMessageBegin("webClientSendUsername", ::apache::thrift::protocol::T_CALL, cseqid);

  SHIFTService_webClientSendUsername_pargs args;
  args.username = &username;
  args.write(oprot_);

  oprot_->writeMessageEnd();
  oprot_->getTransport()->writeEnd();
  oprot_->getTransport()->flush();
}

void SHIFTServiceClient::recv_webClientSendUsername()
{

  int32_t rseqid = 0;
  std::string fname;
  ::apache::thrift::protocol::TMessageType mtype;

  iprot_->readMessageBegin(fname, mtype, rseqid);
  if (mtype == ::apache::thrift::protocol::T_EXCEPTION) {
    ::apache::thrift::TApplicationException x;
    x.read(iprot_);
    iprot_->readMessageEnd();
    iprot_->getTransport()->readEnd();
    throw x;
  }
  if (mtype != ::apache::thrift::protocol::T_REPLY) {
    iprot_->skip(::apache::thrift::protocol::T_STRUCT);
    iprot_->readMessageEnd();
    iprot_->getTransport()->readEnd();
  }
  if (fname.compare("webClientSendUsername") != 0) {
    iprot_->skip(::apache::thrift::protocol::T_STRUCT);
    iprot_->readMessageEnd();
    iprot_->getTransport()->readEnd();
  }
  SHIFTService_webClientSendUsername_presult result;
  result.read(iprot_);
  iprot_->readMessageEnd();
  iprot_->getTransport()->readEnd();

  return;
}

void SHIFTServiceClient::webUserLogin(const std::string& username)
{
  send_webUserLogin(username);
  recv_webUserLogin();
}

void SHIFTServiceClient::send_webUserLogin(const std::string& username)
{
  int32_t cseqid = 0;
  oprot_->writeMessageBegin("webUserLogin", ::apache::thrift::protocol::T_CALL, cseqid);

  SHIFTService_webUserLogin_pargs args;
  args.username = &username;
  args.write(oprot_);

  oprot_->writeMessageEnd();
  oprot_->getTransport()->writeEnd();
  oprot_->getTransport()->flush();
}

void SHIFTServiceClient::recv_webUserLogin()
{

  int32_t rseqid = 0;
  std::string fname;
  ::apache::thrift::protocol::TMessageType mtype;

  iprot_->readMessageBegin(fname, mtype, rseqid);
  if (mtype == ::apache::thrift::protocol::T_EXCEPTION) {
    ::apache::thrift::TApplicationException x;
    x.read(iprot_);
    iprot_->readMessageEnd();
    iprot_->getTransport()->readEnd();
    throw x;
  }
  if (mtype != ::apache::thrift::protocol::T_REPLY) {
    iprot_->skip(::apache::thrift::protocol::T_STRUCT);
    iprot_->readMessageEnd();
    iprot_->getTransport()->readEnd();
  }
  if (fname.compare("webUserLogin") != 0) {
    iprot_->skip(::apache::thrift::protocol::T_STRUCT);
    iprot_->readMessageEnd();
    iprot_->getTransport()->readEnd();
  }
  SHIFTService_webUserLogin_presult result;
  result.read(iprot_);
  iprot_->readMessageEnd();
  iprot_->getTransport()->readEnd();

  return;
}

bool SHIFTServiceProcessor::dispatchCall(::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, const std::string& fname, int32_t seqid, void* callContext) {
  ProcessMap::iterator pfn;
  pfn = processMap_.find(fname);
  if (pfn == processMap_.end()) {
    iprot->skip(::apache::thrift::protocol::T_STRUCT);
    iprot->readMessageEnd();
    iprot->getTransport()->readEnd();
    ::apache::thrift::TApplicationException x(::apache::thrift::TApplicationException::UNKNOWN_METHOD, "Invalid method name: '"+fname+"'");
    oprot->writeMessageBegin(fname, ::apache::thrift::protocol::T_EXCEPTION, seqid);
    x.write(oprot);
    oprot->writeMessageEnd();
    oprot->getTransport()->writeEnd();
    oprot->getTransport()->flush();
    return true;
  }
  (this->*(pfn->second))(seqid, iprot, oprot, callContext);
  return true;
}

void SHIFTServiceProcessor::process_submitOrder(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext)
{
  void* ctx = NULL;
  if (this->eventHandler_.get() != NULL) {
    ctx = this->eventHandler_->getContext("SHIFTService.submitOrder", callContext);
  }
  ::apache::thrift::TProcessorContextFreer freer(this->eventHandler_.get(), ctx, "SHIFTService.submitOrder");

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->preRead(ctx, "SHIFTService.submitOrder");
  }

  SHIFTService_submitOrder_args args;
  args.read(iprot);
  iprot->readMessageEnd();
  uint32_t bytes = iprot->getTransport()->readEnd();

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->postRead(ctx, "SHIFTService.submitOrder", bytes);
  }

  SHIFTService_submitOrder_result result;
  try {
    iface_->submitOrder(args.username, args.orderType, args.orderSymbol, args.orderSize, args.orderPrice, args.orderID);
  } catch (const std::exception& e) {
    if (this->eventHandler_.get() != NULL) {
      this->eventHandler_->handlerError(ctx, "SHIFTService.submitOrder");
    }

    ::apache::thrift::TApplicationException x(e.what());
    oprot->writeMessageBegin("submitOrder", ::apache::thrift::protocol::T_EXCEPTION, seqid);
    x.write(oprot);
    oprot->writeMessageEnd();
    oprot->getTransport()->writeEnd();
    oprot->getTransport()->flush();
    return;
  }

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->preWrite(ctx, "SHIFTService.submitOrder");
  }

  oprot->writeMessageBegin("submitOrder", ::apache::thrift::protocol::T_REPLY, seqid);
  result.write(oprot);
  oprot->writeMessageEnd();
  bytes = oprot->getTransport()->writeEnd();
  oprot->getTransport()->flush();

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->postWrite(ctx, "SHIFTService.submitOrder", bytes);
  }
}

void SHIFTServiceProcessor::process_webClientSendUsername(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext)
{
  void* ctx = NULL;
  if (this->eventHandler_.get() != NULL) {
    ctx = this->eventHandler_->getContext("SHIFTService.webClientSendUsername", callContext);
  }
  ::apache::thrift::TProcessorContextFreer freer(this->eventHandler_.get(), ctx, "SHIFTService.webClientSendUsername");

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->preRead(ctx, "SHIFTService.webClientSendUsername");
  }

  SHIFTService_webClientSendUsername_args args;
  args.read(iprot);
  iprot->readMessageEnd();
  uint32_t bytes = iprot->getTransport()->readEnd();

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->postRead(ctx, "SHIFTService.webClientSendUsername", bytes);
  }

  SHIFTService_webClientSendUsername_result result;
  try {
    iface_->webClientSendUsername(args.username);
  } catch (const std::exception& e) {
    if (this->eventHandler_.get() != NULL) {
      this->eventHandler_->handlerError(ctx, "SHIFTService.webClientSendUsername");
    }

    ::apache::thrift::TApplicationException x(e.what());
    oprot->writeMessageBegin("webClientSendUsername", ::apache::thrift::protocol::T_EXCEPTION, seqid);
    x.write(oprot);
    oprot->writeMessageEnd();
    oprot->getTransport()->writeEnd();
    oprot->getTransport()->flush();
    return;
  }

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->preWrite(ctx, "SHIFTService.webClientSendUsername");
  }

  oprot->writeMessageBegin("webClientSendUsername", ::apache::thrift::protocol::T_REPLY, seqid);
  result.write(oprot);
  oprot->writeMessageEnd();
  bytes = oprot->getTransport()->writeEnd();
  oprot->getTransport()->flush();

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->postWrite(ctx, "SHIFTService.webClientSendUsername", bytes);
  }
}

void SHIFTServiceProcessor::process_webUserLogin(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext)
{
  void* ctx = NULL;
  if (this->eventHandler_.get() != NULL) {
    ctx = this->eventHandler_->getContext("SHIFTService.webUserLogin", callContext);
  }
  ::apache::thrift::TProcessorContextFreer freer(this->eventHandler_.get(), ctx, "SHIFTService.webUserLogin");

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->preRead(ctx, "SHIFTService.webUserLogin");
  }

  SHIFTService_webUserLogin_args args;
  args.read(iprot);
  iprot->readMessageEnd();
  uint32_t bytes = iprot->getTransport()->readEnd();

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->postRead(ctx, "SHIFTService.webUserLogin", bytes);
  }

  SHIFTService_webUserLogin_result result;
  try {
    iface_->webUserLogin(args.username);
  } catch (const std::exception& e) {
    if (this->eventHandler_.get() != NULL) {
      this->eventHandler_->handlerError(ctx, "SHIFTService.webUserLogin");
    }

    ::apache::thrift::TApplicationException x(e.what());
    oprot->writeMessageBegin("webUserLogin", ::apache::thrift::protocol::T_EXCEPTION, seqid);
    x.write(oprot);
    oprot->writeMessageEnd();
    oprot->getTransport()->writeEnd();
    oprot->getTransport()->flush();
    return;
  }

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->preWrite(ctx, "SHIFTService.webUserLogin");
  }

  oprot->writeMessageBegin("webUserLogin", ::apache::thrift::protocol::T_REPLY, seqid);
  result.write(oprot);
  oprot->writeMessageEnd();
  bytes = oprot->getTransport()->writeEnd();
  oprot->getTransport()->flush();

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->postWrite(ctx, "SHIFTService.webUserLogin", bytes);
  }
}

::apache::thrift::stdcxx::shared_ptr< ::apache::thrift::TProcessor > SHIFTServiceProcessorFactory::getProcessor(const ::apache::thrift::TConnectionInfo& connInfo) {
  ::apache::thrift::ReleaseHandler< SHIFTServiceIfFactory > cleanup(handlerFactory_);
  ::apache::thrift::stdcxx::shared_ptr< SHIFTServiceIf > handler(handlerFactory_->getHandler(connInfo), cleanup);
  ::apache::thrift::stdcxx::shared_ptr< ::apache::thrift::TProcessor > processor(new SHIFTServiceProcessor(handler));
  return processor;
}

void SHIFTServiceConcurrentClient::submitOrder(const std::string& username, const std::string& orderType, const std::string& orderSymbol, const int32_t orderSize, const double orderPrice, const std::string& orderID)
{
  int32_t seqid = send_submitOrder(username, orderType, orderSymbol, orderSize, orderPrice, orderID);
  recv_submitOrder(seqid);
}

int32_t SHIFTServiceConcurrentClient::send_submitOrder(const std::string& username, const std::string& orderType, const std::string& orderSymbol, const int32_t orderSize, const double orderPrice, const std::string& orderID)
{
  int32_t cseqid = this->sync_.generateSeqId();
  ::apache::thrift::async::TConcurrentSendSentry sentry(&this->sync_);
  oprot_->writeMessageBegin("submitOrder", ::apache::thrift::protocol::T_CALL, cseqid);

  SHIFTService_submitOrder_pargs args;
  args.username = &username;
  args.orderType = &orderType;
  args.orderSymbol = &orderSymbol;
  args.orderSize = &orderSize;
  args.orderPrice = &orderPrice;
  args.orderID = &orderID;
  args.write(oprot_);

  oprot_->writeMessageEnd();
  oprot_->getTransport()->writeEnd();
  oprot_->getTransport()->flush();

  sentry.commit();
  return cseqid;
}

void SHIFTServiceConcurrentClient::recv_submitOrder(const int32_t seqid)
{

  int32_t rseqid = 0;
  std::string fname;
  ::apache::thrift::protocol::TMessageType mtype;

  // the read mutex gets dropped and reacquired as part of waitForWork()
  // The destructor of this sentry wakes up other clients
  ::apache::thrift::async::TConcurrentRecvSentry sentry(&this->sync_, seqid);

  while(true) {
    if(!this->sync_.getPending(fname, mtype, rseqid)) {
      iprot_->readMessageBegin(fname, mtype, rseqid);
    }
    if(seqid == rseqid) {
      if (mtype == ::apache::thrift::protocol::T_EXCEPTION) {
        ::apache::thrift::TApplicationException x;
        x.read(iprot_);
        iprot_->readMessageEnd();
        iprot_->getTransport()->readEnd();
        sentry.commit();
        throw x;
      }
      if (mtype != ::apache::thrift::protocol::T_REPLY) {
        iprot_->skip(::apache::thrift::protocol::T_STRUCT);
        iprot_->readMessageEnd();
        iprot_->getTransport()->readEnd();
      }
      if (fname.compare("submitOrder") != 0) {
        iprot_->skip(::apache::thrift::protocol::T_STRUCT);
        iprot_->readMessageEnd();
        iprot_->getTransport()->readEnd();

        // in a bad state, don't commit
        using ::apache::thrift::protocol::TProtocolException;
        throw TProtocolException(TProtocolException::INVALID_DATA);
      }
      SHIFTService_submitOrder_presult result;
      result.read(iprot_);
      iprot_->readMessageEnd();
      iprot_->getTransport()->readEnd();

      sentry.commit();
      return;
    }
    // seqid != rseqid
    this->sync_.updatePending(fname, mtype, rseqid);

    // this will temporarily unlock the readMutex, and let other clients get work done
    this->sync_.waitForWork(seqid);
  } // end while(true)
}

void SHIFTServiceConcurrentClient::webClientSendUsername(const std::string& username)
{
  int32_t seqid = send_webClientSendUsername(username);
  recv_webClientSendUsername(seqid);
}

int32_t SHIFTServiceConcurrentClient::send_webClientSendUsername(const std::string& username)
{
  int32_t cseqid = this->sync_.generateSeqId();
  ::apache::thrift::async::TConcurrentSendSentry sentry(&this->sync_);
  oprot_->writeMessageBegin("webClientSendUsername", ::apache::thrift::protocol::T_CALL, cseqid);

  SHIFTService_webClientSendUsername_pargs args;
  args.username = &username;
  args.write(oprot_);

  oprot_->writeMessageEnd();
  oprot_->getTransport()->writeEnd();
  oprot_->getTransport()->flush();

  sentry.commit();
  return cseqid;
}

void SHIFTServiceConcurrentClient::recv_webClientSendUsername(const int32_t seqid)
{

  int32_t rseqid = 0;
  std::string fname;
  ::apache::thrift::protocol::TMessageType mtype;

  // the read mutex gets dropped and reacquired as part of waitForWork()
  // The destructor of this sentry wakes up other clients
  ::apache::thrift::async::TConcurrentRecvSentry sentry(&this->sync_, seqid);

  while(true) {
    if(!this->sync_.getPending(fname, mtype, rseqid)) {
      iprot_->readMessageBegin(fname, mtype, rseqid);
    }
    if(seqid == rseqid) {
      if (mtype == ::apache::thrift::protocol::T_EXCEPTION) {
        ::apache::thrift::TApplicationException x;
        x.read(iprot_);
        iprot_->readMessageEnd();
        iprot_->getTransport()->readEnd();
        sentry.commit();
        throw x;
      }
      if (mtype != ::apache::thrift::protocol::T_REPLY) {
        iprot_->skip(::apache::thrift::protocol::T_STRUCT);
        iprot_->readMessageEnd();
        iprot_->getTransport()->readEnd();
      }
      if (fname.compare("webClientSendUsername") != 0) {
        iprot_->skip(::apache::thrift::protocol::T_STRUCT);
        iprot_->readMessageEnd();
        iprot_->getTransport()->readEnd();

        // in a bad state, don't commit
        using ::apache::thrift::protocol::TProtocolException;
        throw TProtocolException(TProtocolException::INVALID_DATA);
      }
      SHIFTService_webClientSendUsername_presult result;
      result.read(iprot_);
      iprot_->readMessageEnd();
      iprot_->getTransport()->readEnd();

      sentry.commit();
      return;
    }
    // seqid != rseqid
    this->sync_.updatePending(fname, mtype, rseqid);

    // this will temporarily unlock the readMutex, and let other clients get work done
    this->sync_.waitForWork(seqid);
  } // end while(true)
}

void SHIFTServiceConcurrentClient::webUserLogin(const std::string& username)
{
  int32_t seqid = send_webUserLogin(username);
  recv_webUserLogin(seqid);
}

int32_t SHIFTServiceConcurrentClient::send_webUserLogin(const std::string& username)
{
  int32_t cseqid = this->sync_.generateSeqId();
  ::apache::thrift::async::TConcurrentSendSentry sentry(&this->sync_);
  oprot_->writeMessageBegin("webUserLogin", ::apache::thrift::protocol::T_CALL, cseqid);

  SHIFTService_webUserLogin_pargs args;
  args.username = &username;
  args.write(oprot_);

  oprot_->writeMessageEnd();
  oprot_->getTransport()->writeEnd();
  oprot_->getTransport()->flush();

  sentry.commit();
  return cseqid;
}

void SHIFTServiceConcurrentClient::recv_webUserLogin(const int32_t seqid)
{

  int32_t rseqid = 0;
  std::string fname;
  ::apache::thrift::protocol::TMessageType mtype;

  // the read mutex gets dropped and reacquired as part of waitForWork()
  // The destructor of this sentry wakes up other clients
  ::apache::thrift::async::TConcurrentRecvSentry sentry(&this->sync_, seqid);

  while(true) {
    if(!this->sync_.getPending(fname, mtype, rseqid)) {
      iprot_->readMessageBegin(fname, mtype, rseqid);
    }
    if(seqid == rseqid) {
      if (mtype == ::apache::thrift::protocol::T_EXCEPTION) {
        ::apache::thrift::TApplicationException x;
        x.read(iprot_);
        iprot_->readMessageEnd();
        iprot_->getTransport()->readEnd();
        sentry.commit();
        throw x;
      }
      if (mtype != ::apache::thrift::protocol::T_REPLY) {
        iprot_->skip(::apache::thrift::protocol::T_STRUCT);
        iprot_->readMessageEnd();
        iprot_->getTransport()->readEnd();
      }
      if (fname.compare("webUserLogin") != 0) {
        iprot_->skip(::apache::thrift::protocol::T_STRUCT);
        iprot_->readMessageEnd();
        iprot_->getTransport()->readEnd();

        // in a bad state, don't commit
        using ::apache::thrift::protocol::TProtocolException;
        throw TProtocolException(TProtocolException::INVALID_DATA);
      }
      SHIFTService_webUserLogin_presult result;
      result.read(iprot_);
      iprot_->readMessageEnd();
      iprot_->getTransport()->readEnd();

      sentry.commit();
      return;
    }
    // seqid != rseqid
    this->sync_.updatePending(fname, mtype, rseqid);

    // this will temporarily unlock the readMutex, and let other clients get work done
    this->sync_.waitForWork(seqid);
  } // end while(true)
}



