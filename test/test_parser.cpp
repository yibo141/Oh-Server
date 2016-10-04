#include <iostream>
#include <string>
#include "http_parser.h"

using namespace std;

int main()
{
    string request = "GET /index.html HTTP/1.1\r\nHost: www.zhihu.com\r\nConnection: keep-alive\r\n";
    http_parser p(request);
    http_request result = p.get_parse_result();
    cout << "Method: " << result.method << endl;
    cout << "URI: " << result.uri << endl;
    cout << "Version: " << result.version << endl;
    cout << "Host: " << result.host << endl;
    cout << "Connection: " << result.connection << endl;
    return 0;
}
