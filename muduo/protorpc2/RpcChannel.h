// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_PROTORPC2_RPCCHANNEL_H
#define MUDUO_PROTORPC2_RPCCHANNEL_H

#include <muduo/base/Atomic.h>
#include <muduo/base/Mutex.h>
#include <muduo/net/protorpc/RpcCodec.h>

#include <google/protobuf/stubs/common.h> // implicit_cast, down_cast
#if GOOGLE_PROTOBUF_VERSION >= 3000000
#include <google/protobuf/stubs/casts.h> // implicit_cast, down_cast
#endif

#include <map>

// Service and RpcChannel classes are incorporated from
// google/protobuf/service.h

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

namespace google {
namespace protobuf {

// Defined in other files.
class Descriptor;            // descriptor.h
class ServiceDescriptor;     // descriptor.h
class MethodDescriptor;      // descriptor.h
class Message;               // message.h
typedef ::std::shared_ptr<Message> MessagePtr;

// When you upcast (that is, cast a pointer from type Foo to type
// SuperclassOfFoo), it's fine to use implicit_cast<>, since upcasts
// always succeed.  When you downcast (that is, cast a pointer from
// type Foo to type SubclassOfFoo), static_cast<> isn't safe, because
// how do you know the pointer is really of type SubclassOfFoo?  It
// could be a bare Foo, or of type DifferentSubclassOfFoo.  Thus,
// when you downcast, you should use this macro.  In debug mode, we
// use dynamic_cast<> to double-check the downcast is legal (we die
// if it's not).  In normal mode, we do the efficient static_cast<>
// instead.  Thus, it's important to test in debug mode to make sure
// the cast is legal!
//    This is the only place in the code we should use dynamic_cast<>.
// In particular, you SHOULDN'T be using dynamic_cast<> in order to
// do RTTI (eg code like this:
//    if (dynamic_cast<Subclass1>(foo)) HandleASubclass1Object(foo);
//    if (dynamic_cast<Subclass2>(foo)) HandleASubclass2Object(foo);
// You should design the code some other way not to need this.

template<typename To, typename From>     // use like this: down_pointer_cast<T>(foo);
inline ::std::shared_ptr<To> down_pointer_cast(const ::std::shared_ptr<From>& f) {
  // so we only accept smart pointers
  // Ensures that To is a sub-type of From *.  This test is here only
  // for compile-time type checking, and has no overhead in an
  // optimized build at run-time, as it will be optimized away
  // completely.
  if (false) {
    implicit_cast<const From*, To*>(0);
  }

#if !defined(NDEBUG) && !defined(GOOGLE_PROTOBUF_NO_RTTI)
  assert(f == NULL || dynamic_cast<To*>(muduo::get_pointer(f)) != NULL);  // RTTI: debug mode only!
#endif
  return ::std::static_pointer_cast<To>(f);
}

}  // namespace protobuf
}  // namespace google


namespace muduo
{
namespace net
{

class RpcController;
class Service;

// Abstract interface for an RPC channel.  An RpcChannel represents a
// communication line to a Service which can be used to call that Service's
// methods.  The Service may be running on another machine.  Normally, you
// should not call an RpcChannel directly, but instead construct a stub Service
// wrapping it.  Example:
// FIXME: update here
//   RpcChannel* channel = new MyRpcChannel("remotehost.example.com:1234");
//   MyService* service = new MyService::Stub(channel);
//   service->MyMethod(request, &response, callback);
class RpcChannel : noncopyable
{
 public:
  typedef std::map<std::string, Service*> ServiceMap;

  RpcChannel();

  explicit RpcChannel(const TcpConnectionPtr& conn);

  ~RpcChannel();

  void setConnection(const TcpConnectionPtr& conn)
  {
    conn_ = conn;
  }

  const ServiceMap* getServices() const
  {
    return services_;
  }

  void setServices(const ServiceMap* services)
  {
    services_ = services;
  }

  typedef ::std::function<void(const ::google::protobuf::MessagePtr&)> ClientDoneCallback;

  // Call the given method of the remote service.  The signature of this
  // procedure looks the same as Service::CallMethod(), but the requirements
  // are less strict in one important way:  the request and response objects
  // need not be of any specific class as long as their descriptors are
  // method->input_type() and method->output_type().
  void CallMethod(const ::google::protobuf::MethodDescriptor* method,
                  const ::google::protobuf::Message& request,
                  const ::google::protobuf::Message* response,
                  const ClientDoneCallback& done);

  template<typename Output>
  static void downcastcall(const ::std::function<void(const std::shared_ptr<Output>&)>& done,
                           const ::google::protobuf::MessagePtr& output)
  {
    done(::google::protobuf::down_pointer_cast<Output>(output));
  }

  template<typename Output>
  void CallMethod(const ::google::protobuf::MethodDescriptor* method,
                  const ::google::protobuf::Message& request,
                  const Output* response,
                  const ::std::function<void(const std::shared_ptr<Output>&)>& done)
  {
    CallMethod(method, request, response, std::bind(&downcastcall<Output>, done, _1));
  }

  void onDisconnect();

  void onMessage(const TcpConnectionPtr& conn,
                 Buffer* buf,
                 Timestamp receiveTime);

 private:
  void onRpcMessage(const TcpConnectionPtr& conn,
                    const RpcMessagePtr& messagePtr,
                    Timestamp receiveTime);

  void callServiceMethod(const RpcMessage& message);
  void doneCallback(const ::google::protobuf::Message* responsePrototype,
                    const ::google::protobuf::Message* response,
                    int64_t id);

  struct OutstandingCall
  {
    const ::google::protobuf::Message* response;
    ClientDoneCallback done;
  };

  RpcCodec codec_;
  TcpConnectionPtr conn_;
  AtomicInt64 id_;

  MutexLock mutex_;
  std::map<int64_t, OutstandingCall> outstandings_;

  const ServiceMap* services_;
};
typedef std::shared_ptr<RpcChannel> RpcChannelPtr; // FIXME: unique_ptr

}
}

#endif  // MUDUO_PROTORPC2_RPCCHANNEL_H
