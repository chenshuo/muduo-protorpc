#include <muduo/base/StringPiece.h>
#include <map>

namespace zurg
{

// returns file name
std::string writeTempFile(muduo::StringPiece prefix, muduo::StringPiece content);

void parseMd5sum(const std::string& lines, std::map<muduo::StringPiece, muduo::StringPiece>* md5sums);

void setupWorkingDir(const std::string& cwd);

void setNonBlockAndCloseOnExec(int fd);

}
