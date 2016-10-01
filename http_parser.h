#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <string>
#include <vector>

// #define BUFFER_SIZE 4096

/*
 *  正在解析的请求的状态：PARSE_REQUESTLINE表示正在解析请求行
 *  PARSE_HEADER表示正在解析首部字段
 */
enum PARSE_STATE { PARSE_REQUESTLINE = 0, PARSE_HEADER };

// 表示正在解析的行的状态，即：读取到一个完整的行、行出错和接收到数据不完整
enum LINE_STATUS { LINE_OK = 0, LINE_ERROR, LINE_MORE };

/*
 * 服务器处理HTTP请求的结果：
 * MORE_DATE表示请求不完整，需继续读取请求
 * GET_REQUEST表示获得了一个完整的客户请求
 * REQUEST_ERROR表示客户请求的语法错误
 * FORBIDDEN_REQUEST表示客户对资源没有访问权限
 * INTERNAL_ERROR表示服务器内部出现错误
 * CLOSE_CONNECTION表示客户端已经关闭连接
 */
enum HTTP_CODE { MORE_DATA = 0, GET_REQUEST, REQUEST_ERROR, FORBIDDEN_REQUEST,
    INTERNAL_ERROR, CLOSE_CONNECTION };

// 客户请求的方法
enum METHOD { GET = 0, POST, HEAD, PUT, DELETE, TRACE,
    OPTIONS, CONNECT, PATCH };

// 解析请求后的数据存储在http_request结构体中
typedef struct
{
    METHOD method;          // 请求的方法
    std::string uri;        // 请求的uri
    std::string version;    // HTTP版本
    std::string host;       // 请求的主机名
    std::string connection; // Connection首部
} http_request;

class parser
{
public:
    parser(std::string request);
    ~parser();
    http_request get_parse_result(); // 返回请求的结果
    
private:
    LINE_STATUS parse_line();        // 解析出一行内容
    HTTP_CODE parse_requestline();   // 解析请求行
    HTTP_CODE parse_headers();       // 解析头部字段

    std::string request;             // 客户请求内容
    std::vector<std::string> lines;  // 存储每一行请求
    string::size_type line_begin;    // 正在解析的行的行首索引
    string::size_type check_index;   // 当前正在解析的字符索引
    http_request parse_result;
};  

#endif  // HTTP_PARSER_H
