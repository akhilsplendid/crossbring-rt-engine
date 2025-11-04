#ifdef USE_ZEROMQ

#include <iostream>
#include <string>
#include <zmq.h>

int main(int argc, char** argv) {
    std::string endpoint = argc > 1 ? argv[1] : std::string("tcp://localhost:5556");
    void* ctx = zmq_ctx_new();
    void* sock = zmq_socket(ctx, ZMQ_SUB);
    zmq_setsockopt(sock, ZMQ_SUBSCRIBE, "", 0);
    if (zmq_connect(sock, endpoint.c_str()) != 0) {
        std::cerr << "Connect failed: " << zmq_strerror(zmq_errno()) << "\n";
        return 1;
    }
    std::cerr << "Subscribed to " << endpoint << ". Ctrl+C to exit.\n";
    char buf[65536];
    while (true) {
        int n = zmq_recv(sock, buf, sizeof(buf)-1, 0);
        if (n < 0) break;
        buf[n] = 0;
        std::cout << buf << std::endl;
    }
    zmq_close(sock);
    zmq_ctx_term(ctx);
    return 0;
}

#else
int main(){return 0;}
#endif

