#include <muduo/base/StringPiece.h>
#include <map>

namespace zurg
{

std::string writeTempFile(muduo::StringPiece content);

void parseMd5sum(const std::string& lines, std::map<muduo::StringPiece, muduo::StringPiece>* md5sums);

void setupWorkingDir(const std::string& cwd);
}
