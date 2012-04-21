#include <google/protobuf/stubs/common.h>

namespace zurg
{

void zurgLogHandler(google::protobuf::LogLevel level,
                    const char* filename,
                    int line,
                    const std::string& message);

}
