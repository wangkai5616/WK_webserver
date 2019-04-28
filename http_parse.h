#ifndef HTTP_PARSE_H
#define HTTP_PARSE_H

#define CR '\r'
#define LF '\n'
//处理请求行和请求头

// http请求行解析
int wk_http_parse_request_line(wk_http_request_t *request);
// http请求体解析
int wk_http_parse_request_body(wk_http_request_t *request);

#endif
