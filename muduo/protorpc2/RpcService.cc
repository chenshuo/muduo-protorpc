#include <muduo/protorpc2/RpcService.h>

#include <google/protobuf/descriptor.h>

using namespace muduo::net;

namespace
{
  void addMethodNames(const ::google::protobuf::ServiceDescriptor* sd,
                      ListRpcResponse* response)
  {
    for (int i = 0; i < sd->method_count(); ++i)
    {
      const ::google::protobuf::MethodDescriptor* method = sd->method(i);
      std::string methodName;
      methodName.reserve(10
          + method->full_name().size()
          + method->input_type()->full_name().size()
          + method->output_type()->full_name().size());
      methodName.append(method->full_name());
      methodName.append(" (");
      methodName.append(method->input_type()->full_name());
      methodName.append(") : ");
      methodName.append(method->output_type()->full_name());
      response->add_method_name(methodName);
    }
  }
}

void RpcServiceImpl::listRpc(const ListRpcRequestPtr& request,
                             const ListRpcResponse* responsePrototype,
                             const RpcDoneCallback& done)
{
  ListRpcResponse response;
  if (request->has_service_name())
  {
    ServiceMap::const_iterator it = services_->find(request->service_name());
    if (it != services_->end())
    {
      response.set_error(NO_ERROR);
      response.add_service_name(it->first);
      if (request->list_method())
      {
        addMethodNames(it->second->GetDescriptor(), &response);
      }
    }
    else
    {
      response.set_error(NO_SERVICE);
    }
  }
  else
  {
    response.set_error(NO_ERROR);
    for (ServiceMap::const_iterator it = services_->begin();
        it != services_->end(); ++it)
    {
      response.add_service_name(it->first);
      if (request->list_method())
      {
        addMethodNames(it->second->GetDescriptor(), &response);
      }
    }
  }
  done(&response);
}

void RpcServiceImpl::getService(const GetServiceRequestPtr& request,
                                const GetServiceResponse* responsePrototype,
                                const RpcDoneCallback& done)
{
  GetServiceResponse response;
  ServiceMap::const_iterator it = services_->find(request->service_name());
  if (it != services_->end())
  {
    response.set_error(NO_ERROR);
    const ::google::protobuf::FileDescriptor* fd = it->second->GetDescriptor()->file();
    response.add_proto_file(fd->DebugString());
    response.add_proto_file_name(fd->name());
    // FIXME: recursive
    for (int i = 0; i < fd->dependency_count(); ++i)
    {
      response.add_proto_file(fd->dependency(i)->DebugString());
      response.add_proto_file_name(fd->dependency(i)->name());
    }
  }
  else
  {
    response.set_error(NO_SERVICE);
  }
  done(&response);
}

