#include "HTTPRequestHandler.h"
#include "Buffer.h"
#include "Request.h"

static uint32_t  usual[] = {
    0xffffdbfe, /* 1111 1111 1111 1111  1101 1011 1111 1110 */

                /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
    0x7fff37d6, /* 0111 1111 1111 1111  0011 0111 1101 0110 */
    
                /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
    0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */

                /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
    0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */

    0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    0xffffffff  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
};

static int parseRequestLine(Buffer&,
        const boost::shared_ptr<Request>&);


int processRequestLine(Buffer& headerBuf,
        const boost::shared_ptr<Request>& r)
{
    int rc = AGAIN;
    
    for( ;; ){
        rc = parseRequestLine(headerBuf, r);

        if (rc == OK) {
             
            /* the request line has been parsed successfully */

            return rc;
        } else {
            return -1;
        }
    }

}

/*
 * HTTP request line state machine
 * 
 * @return: 
 *      ERROR  error in request line.
 *
 *      AGAIN  parse not complete. In this case
 *             buffer will be read again and this
 *             method is called again.
 *      
 *      OK     parse complete.
 *
 */

static int parseRequestLine(Buffer& header,
        const boost::shared_ptr<Request>& r)
{
    u_char  c, ch, *p, *m;
    u_char *uri_start, *uri_end;
    unsigned int http_major= 0, http_minor = 0;
    enum {
        sw_start = 0,
        sw_method,
        sw_spaces_before_uri,
        sw_schema,
        sw_schema_slash,
        sw_schema_slash_slash,
        sw_host_start,
        sw_host,
        sw_host_end,
        sw_host_ip_literal,
        sw_port,
        sw_host_http_09,
        sw_after_slash_in_uri,
        sw_check_uri,
        sw_check_uri_http_09,
        sw_uri,
        sw_http_09,
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
    };

    uint8_t state = r->state();

    for (p = header.start(); p < header.last(); p++) {
        ch = *p;

        switch (state) {
        case sw_start:
            std::cout << "sw_start\n";
            m = p;    // m points to the start of request line.
            if (ch == '\n' || ch == '\r') {
                break;
            }
            if (( ch < 'A' || ch > 'Z' ) && ch != '_' && ch != '-') {
                return HTTP_PARSE_INVALID_METHOD;
            }

            state = sw_method;
            break;

        case sw_method:
            std::cout << "sw_method\n";

            if( ch == ' ' ) {
                switch (p - m) {
                case 3:
                    std::cout << "method: GET\n";
                    if(str3_cmp(m, 'G', 'E', 'T')) {
                        r->setMethod(HTTP_GET);
                        break;
                    }

                case 4:
                    if(str4_cmp(m, 'P', 'O', 'S', 'T')) {
                        r->setMethod(HTTP_POST);
                        break;
                    }

                default:
                    return HTTP_PARSE_INVALID_METHOD;
                }

                state = sw_spaces_before_uri;
            }

            break;

        case sw_spaces_before_uri: 

            std::cout << "sw_spaces_before_uri\n";
            if(ch == '/') {
                std::cout << "/\n";
                uri_start = p;

                /*
                 * origin form
                 *
                 * GET / HTTP/1.1
                 * 
                 * GET /static/favico.png HTTP/1.1
                 *
                 */
                state = sw_after_slash_in_uri;
                break;
            }

            c = (u_char)(ch | 0x20);    // upper to lower
            if (c >= 'a' && c <= 'z') {
                //TODO r->schema_start = p;
                
                /* 
                 * absolute form
                 *
                 * GET http://buzzlightyear.xyz/static/favico.png HTTP/1.1
                 * 
                 */
                state = sw_schema;
                break;
            }

            switch (ch) {
            case ' ':
                //skip spaces
                break;
            default:
                return HTTP_PARSE_INVALID_REQUEST;
            }

            break;

        case sw_schema:
            c = (u_char) (ch | 0x20);
            if (c >= 'z' || c <= 'a') {
                break;
            }
            
            switch(ch) {
            case ':':
                //TODO r->schema_end = p
                state = sw_schema_slash;
                break;
            default:
                return HTTP_PARSE_INVALID_REQUEST;
            }
            break;

        case sw_schema_slash:
            switch(ch) {
            case '/':
                state = sw_schema_slash_slash;
                break;
            default:
                return HTTP_PARSE_INVALID_REQUEST;
            }
            break;

        case sw_schema_slash_slash:
            switch(ch) {
            case '/':
                state = sw_host_start;
                break;
            default:
                return HTTP_PARSE_INVALID_REQUEST;
            }
            break;

        case sw_host_start:
            //TODO r->host_start = p;
            state = sw_host;
            break;

        case sw_host:

            /*
             * break to skip domain name
             */
            c = (u_char)(ch | 0x20);
            if (c >= 'a' && c <= 'z') {
                break;
            }

            if (( ch >= '0' && ch <= '9' ) || ch == '.' || ch == '-' ) {
                break;
            }

            /* else fall through */

        case sw_host_end:
            //TODO r->host_end = p

            switch (ch) {
            case ':':
                state = sw_port;
                break;
            case '/':
                uri_start = p;
                /*
                 * GET http://buzzlightyear.xyz/static/favico.png HTTP/1.1
                 * p points to '/' after 'xyz'
                 */
                state = sw_after_slash_in_uri;
                break;
            //TODO case ' ':

            default:
                return HTTP_PARSE_INVALID_REQUEST;
            }
            break;

        case sw_port:
            /* skip port number */
            if (ch >= '0' && ch <= '9') {
                break;
            }

            switch (ch) {
            case '/':
                //TODO r->port_end = p;
                uri_start = p;
                state = sw_after_slash_in_uri;
                break;
            default:
                return HTTP_PARSE_INVALID_REQUEST;
            }
            break;

        case sw_after_slash_in_uri:
            
            std::cout << "sw_after_slash_in_uri: " << ch << std::endl;
            if(usual[ch >> 5] & (1U << (ch & 0x1f))) {
                state = sw_check_uri;
                break;
            }

            switch (ch) {
            case ' ':
                uri_end = p;
                state = sw_check_uri_http_09;
                break;
            case '\r':
                //TODO r->uri_end = p
                //TODO r->http_minor = 9
                state = sw_almost_done;
                break;
            case '\n':
                //TODO r->uri_end = p
                //TODO r->http_minor = 9
                goto done;
            case '/':
                //TODO r->complex_uri = 1
                state = sw_uri;
                break;
            case '?':
                //TODO r->args_start = p+1;
                state = sw_uri;
            case '\0':
                return HTTP_PARSE_INVALID_REQUEST;
            }
            break;

        case sw_check_uri:

            std::cout << "sw_check_uri: " << ch << std::endl;
            if(usual[ch >> 5] & (1U << (ch&0x1f))) {
                break;
            }

            switch (ch) {
            case ' ':
                std::cout << "uri end\n";
                uri_end = p;
                state = sw_check_uri_http_09;
                break;
            case '.':
                break;
            case '\r':
                //TODO r->uri_end = p
                //TODO r->http_minor = 9
                state = sw_almost_done;
                break;
            case '\n':
                //TODO r->uri_end = p
                //TODO r->http_minor = 9
                goto done;
            case '/':
                //TODO r->complex_uri = 1
                state = sw_uri;
                break;
            case '?':
                //TODO r->args_start = p+1;
                state = sw_uri;
                break;
            case '\0':
                return HTTP_PARSE_INVALID_REQUEST;

                //TODO other cases
            }
            break;

        case sw_check_uri_http_09:
            std::cout << "check_uri_http_09\n";
            switch (ch) {
            case ' ':
                break;
            case 'H':
                state = sw_http_H;
                break;
            default:
                state = sw_check_uri;
                p--;
                break;
            }
        
        case sw_uri:
            
            std::cout << "sw_uri: " << ch << std::endl;
            if(usual[ch >> 5] & (1U << (ch&0x1f))) {
                break;
            }

            switch (ch) {
            case ' ':   
                uri_end = p;
                state = sw_http_09;
                break;
            case '\r':
                //TODO r->uri_end = p;
                //TODO r->http_minor = 9;
                state = sw_almost_done;
                break;
            case '\n':
                //TODO r->uri_end = p;
                //TODO r->http_minor = 9;
                goto done;
            case '\0':
                return HTTP_PARSE_INVALID_REQUEST;
            }
            break;

        case sw_http_09:
            switch (ch) {
            case ' ':
                break;
            case 'H':
                state = sw_http_H;
                break;
            case '\r':
                //TODO r->http_minor = 9;
                state = sw_almost_done;
                break;
            case '\n':
                //TODO r->http_minor = 9;
                goto done;
            default:
                //there is space in uri
                state = sw_uri;
                p--;
                break;
            }
            break;

        case sw_http_H:
            std::cout << "H\n";
            switch (ch) {
            case 'T':
                state = sw_http_HT;
                break;
            default:
                return HTTP_PARSE_INVALID_REQUEST;
            }
            break;

        case sw_http_HT:
            std::cout << "HT\n";
            switch (ch) {
            case 'T':
                state = sw_http_HTT;
                break;
            default:
                return HTTP_PARSE_INVALID_REQUEST;
            }
            break;

        case sw_http_HTT:
            std::cout << "HTT\n";
            switch (ch) {
            case 'P':
                state = sw_http_HTTP;
                break;
            default:
                return HTTP_PARSE_INVALID_REQUEST;
            }
            break;

        case sw_http_HTTP:
            std::cout << "HTTP\n";
            switch (ch) {
            case '/':
                state = sw_first_major_digit;
                break;
            default:
                return HTTP_PARSE_INVALID_REQUEST;
            }
            break;

        case sw_first_major_digit:
            std::cout << "first_major_digit\n";
            if (ch < '1' || ch > '9') {
                return HTTP_PARSE_INVALID_REQUEST;
            }

            http_major = ch - '0';
            std::cout << "first_major_digit: " << http_major << std::endl;

            if ((ch - '0') > 1) {
                return HTTP_PARSE_INVALID_VERSION; 
            }

            state = sw_major_digit;
            break;
        
        case sw_major_digit:
            std::cout << "major_digit\n";
            if(ch == '.') {
                state = sw_first_minor_digit;
                break;
            }
            
            if (ch <= '0' || ch >= '9') {
                return HTTP_PARSE_INVALID_REQUEST;
            }

            //TODO http_major = r->http_major * 10 + (ch - '0');
        
        case sw_first_minor_digit:
            std::cout << "first_minor_digit\n";
            if (ch <= '0' || ch >= '9') {
                return HTTP_PARSE_INVALID_REQUEST;
            }

            http_minor = ch - '0';
            std::cout << "http_minor: " << http_minor << std::endl;
            state = sw_minor_digit;
            break;

        case sw_minor_digit:
            std::cout << "minor_digit\n";
            if (ch == '\r') {
                state = sw_almost_done;
                break;
            }

            if (ch == '\n') {
                goto done;
            }

            if (ch == ' ') {
                state = sw_spaces_after_digit;
                break;
            }

            if (ch <= '0' || ch >= '9') {
                return HTTP_PARSE_INVALID_REQUEST;
            }

            //TODO r->http_minor = r->http_minor * 10 + (ch - '0');
            break;
        
        case sw_spaces_after_digit:
            switch (ch) {
            case ' ':
                break;
            case '\r':
                state = sw_almost_done;
                break;
            case '\n':
                goto done;
            default:
                return HTTP_PARSE_INVALID_REQUEST;
            }
            break;

        case sw_almost_done:
            //TODO r->request_end = p-1;
            std::cout << "almost done\n";
            switch (ch) {
            case '\n':
                goto done;
            default:
                return HTTP_PARSE_INVALID_REQUEST;
            }

        } //switch
    } // for

    header.setPos(p);
    r->setState(state);

    return AGAIN;
    
done:

    std::cout << "done\n";
    header.setPos(p+1);
    r->setHTTPVersion(http_major * 1000 + http_minor);
    r->setState(sw_start);
    r->setUri(uri_start, uri_end);

    if(r->httpVersion() < 1000 && r->method() != HTTP_GET) {
        return HTTP_PARSE_INVALID_09_METHOD;
    }

    return OK;
}

