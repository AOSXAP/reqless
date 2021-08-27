#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <iostream>
#include <errno.h>

using namespace std;

//ssl
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/ssl.h>
#include <openssl/pem.h>
#include <openssl/err.h>

//colored output
#include "colors.h"

namespace xSSL{
    struct context_socket{
        SSL *ssl_object; SSL_CTX* context;
    };

    void init(){
        //init SSL and load algorithms/error strings
        SSL_library_init(); OpenSSL_add_all_algorithms();
            SSL_load_error_strings();
    }

    SSL_CTX* create_context(){
        SSL_CTX *context = SSL_CTX_new(TLS_client_method());

        if(!context) cout<<"SSL Context Error!";

        return context;
    }

    context_socket create_secure_socket(){
        context_socket Obj;

        SSL_CTX* ctx = create_context();
            SSL *ssl_obj = SSL_new(ctx);
        
        Obj.context = ctx; 
        Obj.ssl_object = ssl_obj;

        return Obj;
    }
};

void msg(std::string message , std::string color){
    Color::CPRINT(color , message);
}

void prepare_hints(addrinfo &hints){
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET; hints.ai_socktype = SOCK_STREAM; 
    hints.ai_flags = AI_PASSIVE;
}

char * sendRequest(string url, string method) {
    struct addrinfo hints,*res; prepare_hints(hints);

    size_t start = url.find("://", 0);
    start += 3; 
    size_t end = url.find("/", start + 1);
    string protocol = url.substr(0, start - 3);
    string domain = url.substr(start, end - start);
    string path = url.substr(end);

    cout << protocol << endl;
    cout << domain << endl;
    cout << path << endl;
    
    getaddrinfo(domain.c_str(), protocol.c_str(), &hints, &res);
        int socketx = socket(AF_INET, SOCK_STREAM, 0);

    if(connect(socketx , res -> ai_addr, res -> ai_addrlen) == 0) 
        msg("Connection successful!\n", "green");
    else msg("Error connecting to socket!\n" , "red");

    string toSend = method + " " + path + " HTTP/1.1\r\nHost:" + domain + "\r\nUpgrade-Insecure-Requests: 0\r\n\r\n";
    cout << toSend << endl; 

    void *read[4096]; int bytesReceived;
    
    msg("Sent Request!\n", "green");

    xSSL::init();  
        
    xSSL::context_socket Builder  = xSSL::create_secure_socket();

    SSL* ssl_obj = Builder.ssl_object; SSL_CTX* ctx = Builder.context;

    //secure socket
    if(protocol == "https"){

        if(!SSL_set_tlsext_host_name(ssl_obj,(void *) domain.c_str()))
            msg("cant get site's ceritficate!\n" , "red");
        
        //set file descriptor
        SSL_set_fd(ssl_obj,socketx); if(SSL_connect(ssl_obj) == -1){
            msg("ssl connect failed!\n" , "red");
        }

        SSL_write(ssl_obj, toSend.c_str(), strlen(toSend.c_str()));
    }else{
        send(socketx, toSend.c_str(), strlen(toSend.c_str()) , 0);
    }

    do {
        if(protocol == "https") int bytesReceived = SSL_read(ssl_obj, read, 4096);
        else int bytesReceived = recv(socketx, read, 4096,0);

        if (bytesReceived > 0)
             cout << "Bytes received: " << bytesReceived << endl;
         else if (bytesReceived == 0){
            cout << "Connection closed" << endl; 
        }
        else {
            msg("Error while receiving!\n" , "red");
                fprintf(stderr, "recv: %s (%d)\n", strerror(errno), errno);
        }
    } while (bytesReceived > 0);

    if (protocol == "https") {
        SSL_shutdown(ssl_obj); SSL_free(ssl_obj);
            SSL_CTX_free(ctx);
    }

    freeaddrinfo(res);
    close(socketx);

    char *response;
    response = (char*)read;
    return response;
}

int main() {
    string url = "https://api.myip.com/";
    char *response = sendRequest(url, "GET");
    cout << "RESPONSE" << endl;
    cout << response << endl;
}