#include "http.h"

//http请求行解析
//请求行格式是：方法 URL 版本
int wk_http_parse_request_line(wk_http_request_t *request){
    enum{
        sw_start = 0,
        sw_method,
        sw_spaces_before_uri,
        sw_after_slash_in_uri,
        sw_http,
        sw_http_H,
        sw_http_HT,
        sw_http_HTT,
        sw_http_HTTP,
        sw_first_major_digit,
        sw_major_digit,
        sw_first_minor_digit,
        sw_minor_digit,
        sw_spaces_after_digit,
        sw_almost_done
    }state;
	//获取请求request的当前状态state
    state = request->state;

	//p是缓冲区的某一个字符的地址，ch是这个字符,m指向的是请求方法的开始地址
    u_char ch, *p, *m;
    size_t pi;//缓冲区当前下标

	//只要缓冲区的有效内容不为空
    for(pi = request->pos; pi < request->last; pi++){
        p = (u_char *)&request->buff[pi % MAX_BUF];
        ch = *p;

        switch(state){
			//当前状态为sw_start即起始状态
            case sw_start:
                request->request_start = p;
                if(ch == CR || ch == LF)
                    break;
                if((ch < 'A' || ch > 'Z') && ch != '_')
                    return wk_HTTP_PARSE_INVALID_METHOD;
				//置state为sw_method，表示解析请求方法
                state = sw_method;
                break;
			
			// 当前状态为解析请求方法
            case sw_method:
                if(ch == ' '){
					//说明遇到了请求方法后面的空格了
					//request->method_end记录请求方法的结束位置
                    request->method_end = p;
					//r->request_start此时指向的是请求方法的第一个字符
                    m = request->request_start;
                    switch(p - m){
                        case 3:
                            if(wk_str3_cmp(m, 'G', 'E', 'T', ' ')){
                                request->method = wk_HTTP_GET;
                                break;
                            }
                            break;
                        case 4:
                            if(wk_str3Ocmp(m, 'P', 'O', 'S', 'T')){
                                request->method = wk_HTTP_POST;
                                break;
                            }
                            if(wk_str4cmp(m, 'H', 'E', 'A', 'D')){
                                request->method = wk_HTTP_HEAD;
                                break;
                            }
                            break;
                        default:
                            request->method = wk_HTTP_UNKNOWN;
                            break;
                    }
					
					//解析完请求方法, 置state为sw_spaces_before_uri, 表示解析URL前面的空格
					//因为上面已经解析到一个请求方法后的空格, 所以跳过状态sw_space_after_method,
                    state = sw_spaces_before_uri;
                    break;
                }

                if((ch < 'A' || ch > 'Z') && ch != '_')
                    return wk_HTTP_PARSE_INVALID_METHOD;
                break;

			//当前状态为解析URL前可能存在的多余空格
            case sw_spaces_before_uri:
				//如果当前字符为/, 说明遇到URL的第一个字符
                if(ch == '/'){
					//置r->uri_start为p+1, 记录URL的起始位置
                    request->uri_start = p + 1;
					//置state为sw_after_slash_in_uri, 表示解析URL路径中/后的内容
                    state = sw_after_slash_in_uri;
                    break;
                }
                switch(ch){
					//如果当前字符为空格，直接跳过
                    case ' ':
                        break;
                    default:
                        return wk_HTTP_PARSE_INVALID_REQUEST;
                }
                break;
			
			//当前状态为解析URL路径中/后的内容
            case sw_after_slash_in_uri:
                switch(ch){					
				//如果当前字符为空格, 表示遇到URL(或者路径)后紧跟的一个空格
                    case ' ':						
						//置r->uri_end为p-1, 记录URL中路径的结束位置
                        request->uri_end = p;
					    //置state为sw_http, 表示解析URL后紧跟空格后的内容
                        state = sw_http;
                        break;
                    default:
                        break;
                }
                break;

            case sw_http:
                switch(ch){
                    case ' ':
                        break;
                    case 'H':
                        state = sw_http_H;
                        break;
                    default:
                        return wk_HTTP_PARSE_INVALID_REQUEST;
                }
                break;

            case sw_http_H:
                switch(ch){
					// 当前状态为解析协议版本的第二个字符T
                    case 'T':
                        state = sw_http_HT;
                        break;
                    default:
                        return wk_HTTP_PARSE_INVALID_REQUEST;
                }
                break;

            case sw_http_HT:
                switch(ch){
					// 当前状态为解析协议版本的第三个字符T
                    case 'T':
                        state = sw_http_HTT;
                        break;
                    default:
                        return wk_HTTP_PARSE_INVALID_REQUEST;
                }
                break;

            case sw_http_HTT:
                switch(ch){
					//当前状态为解析协议版本的第四个字符P
                    case 'P':
                        state = sw_http_HTTP;
                        break;
                    default:
                        return wk_HTTP_PARSE_INVALID_REQUEST;
                }
                break;

            case sw_http_HTTP:
				
				//当前状态为解析协议版本的第五个字符/
                switch(ch){
					//置state为sw_first_major_digit
                    case '/':
                        state = sw_first_major_digit;
                        break;
                    default:
                        return wk_HTTP_PARSE_INVALID_REQUEST;
                }
                break;
			
			//当前状态为解析协议版本的主版本号的第一个数字
            case sw_first_major_digit:
                if(ch < '1' || ch > '9')
                    return wk_HTTP_PARSE_INVALID_REQUEST;
				//置r->http_major为ch-'0', 记录主版本号
                request->http_major = ch - '0';
				//置state为sw_major_digit, 表示解析协议版本的主版本号第一个数字后的数字或者.
                state = sw_major_digit;
                break;

			//当前状态为解析协议版本的主版本号第一个数字后的数字或者.
            case sw_major_digit:
                if(ch == '.'){
					//如果当前字符为., 说明遇到主版本号后紧跟的.了
                    //置state为sw_first_minor_digit, 表示解析次版本号的第一个数字
                    state = sw_first_minor_digit;
                    break;
                }
                if(ch < '0' || ch > '9')
                    return wk_HTTP_PARSE_INVALID_REQUEST;
				//更新主版本号r->http_major，说明不是1.这种，而是11.这种
                request->http_major = request->http_major * 10 + ch - '0';
                break;

			//当前状态为解析协议版本的次版本号的第一个数字
            case sw_first_minor_digit:
                if(ch < '0' || ch > '9')
                    return wk_HTTP_PARSE_INVALID_REQUEST;
				//置r->http_minor为ch-'0', 记录次版本号
                request->http_minor = ch - '0';
				 // 置state为sw_minor_digit, 表示解析协议版本的次版本号第一个数字后的数字
                state = sw_minor_digit;
                break;

			// 当前状态为解析协议版本的次版本号第一个数字后的数字
            case sw_minor_digit:
				
				//如果当前字符为\r, 说明遇到次版本号后紧跟的\r
				//置state为sw_almost_done, 表示解析结束的\n
                if(ch == CR){
                    state = sw_almost_done;
                    break;
                }
				
				//如果当前字符为\n, 说明遇到次版本号后的\n
				//置state为sw_done, 表示解析完成
                if(ch == LF)
                    goto done;
                if(ch == ' '){
                    state = sw_spaces_after_digit;
                    break;
                }
                if(ch < '0' || ch > '9')
                    return wk_HTTP_PARSE_INVALID_REQUEST;
				 //更新次版本号r->http_minor
                request->http_minor = request->http_minor * 10 + ch - '0';
                break;

            case sw_spaces_after_digit:
                switch(ch){
                    case ' ':
                        break;
                    case CR:
                        state = sw_almost_done;
                        break;
                    case LF:
                        goto done;
                    default:
                        return wk_HTTP_PARSE_INVALID_REQUEST;
                }
                break;

			
			//当前状态为解析结束的\n
            case sw_almost_done:				
			    //置r->request_end为p-1, 记录请求行有效内容的结束位置
                request->request_end = p - 1;
                switch(ch){
                    case LF:
                        goto done;
                    default:
                        return wk_HTTP_PARSE_INVALID_REQUEST;
                }
        }
    }
	//没有解析完，记录当前解析状态
    request->pos = pi;
    request->state = state;
    return wk_AGAIN;

    done:
    request->pos = pi + 1;
    if (request->request_end == NULL)
        request->request_end = p;
    request->state = sw_start;
    return 0;
}

// http请求头解析
int wk_http_parse_request_body(wk_http_request_t *request){
    // 状态列表
    enum{
        sw_start = 0,
        sw_key,
        sw_spaces_before_colon,//:之前的空格
        sw_spaces_after_colon,//:之后的空格
        sw_value,
        sw_cr,
        sw_crlf,
        sw_crlfcr
    }state;
    state = request->state;

    size_t pi;
    unsigned char ch, *p;
    wk_http_header_t *hd;
    for (pi = request->pos; pi < request->last; pi++) {
        p = (unsigned char*)&request->buff[pi % MAX_BUF];
        ch = *p;

        switch(state){
            case sw_start:
                if(ch == CR || ch == LF)
                    break;
                request->cur_header_key_start = p;
                state = sw_key;
                break;

            case sw_key:
                if(ch == ' '){
                    request->cur_header_key_end = p;
                    state = sw_spaces_before_colon;
                    break;
                }
                if(ch == ':'){
                    request->cur_header_key_end = p;
                    state = sw_spaces_after_colon;
                    break;
                }
                break;

            case sw_spaces_before_colon:
                if(ch == ' ')
                    break;
                else if(ch == ':'){
                    state = sw_spaces_after_colon;
                    break;
                }
                else
                    return wk_HTTP_PARSE_INVALID_HEADER;

            case sw_spaces_after_colon:
                if (ch == ' ')
                    break;
                state = sw_value;
                request->cur_header_value_start = p;
                break;

            case sw_value:
                if(ch == CR){
                    request->cur_header_value_end = p;
                    state = sw_cr;
                }
                if(ch == LF){
                    request->cur_header_value_end = p;
                    state = sw_crlf;
                }
                break;

            case sw_cr:
                if(ch == LF){
                    state = sw_crlf;
                    hd = (wk_http_header_t *) malloc(sizeof(wk_http_header_t));
                    hd->key_start = request->cur_header_key_start;
                    hd->key_end = request->cur_header_key_end;
                    hd->value_start = request->cur_header_value_start;
                    hd->value_end = request->cur_header_value_end;
					//形成一个key、value对之后，加入list链表
                    list_add(&(hd->list), &(request->list));
                    break;
                }
                else
                    return wk_HTTP_PARSE_INVALID_HEADER;

            case sw_crlf:
                if(ch == CR)
                    state = sw_crlfcr;
                else{
                    request->cur_header_key_start = p;
                    state = sw_key;
                }
                break;

            case sw_crlfcr:
                switch(ch){
                    case LF:
                        goto done;
                    default:
                        return wk_HTTP_PARSE_INVALID_HEADER;
                }
        }
    }
    request->pos = pi;
    request->state = state;
    return wk_AGAIN;

    done:
    request->pos = pi + 1;
    request->state = sw_start;
    return 0;
}
