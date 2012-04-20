#include <examples/zurg/slave/SlaveService.h>

#include <muduo/base/FileUtil.h>
#include <muduo/base/Logging.h>

using namespace muduo;
using namespace zurg;

void SlaveServiceImpl::getFileContent(
    const boost::shared_ptr<const ::zurg::GetFileContentRequest>& request,
    const GetFileContentResponse* responsePrototype,
    const DoneCallback& done)
{
  LOG_INFO << "SlaveServiceImpl::getFileContent - " << request->file_name()
           << " maxSize = " << request->max_size();
  GetFileContentResponse response;
  int64_t file_size = 0;
  int err = FileUtil::readFile(request->file_name(),
                               request->max_size(),
                               response.mutable_content(),
                               &file_size);
  response.set_error_num(err);
  response.set_file_size(file_size);

  LOG_DEBUG << "SlaveServiceImpl::getFileContent - err " << err;
  done(&response);
}

