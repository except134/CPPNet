#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

#include <socket_wrapper/socket_headers.h>
#include <socket_wrapper/socket_wrapper.h>
#include <socket_wrapper/socket_class.h>
#include <cstring>


// Trim from end (in place).
static inline std::string &rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int c) { return !std::isspace(c); }).base());
    return s;
}

std::string host_by_ip(char *addr)
{
    struct in_addr in;
    struct hostent *hp;

    if (inet_aton(addr, &in))
        if ((hp = gethostbyaddr((char *) &in.s_addr, sizeof(in.s_addr), AF_INET)))
            return hp->h_name;

    return "Unknown";
}

#define MAX_MESSAGE_LENGTH 100

int main(int argc, char const *argv[])
{

    if (argc != 4) {
        std::cout << "Usage: " << argv[0] << " <host> <port> <message>" << std::endl;
        return EXIT_FAILURE;
    }

    socket_wrapper::SocketWrapper sock_wrap;

    std::string host(argv[1]);
    const int port { std::stoi(argv[2]) };

    socket_wrapper::Socket sock = {AF_INET, SOCK_DGRAM, IPPROTO_UDP};

    std::cout << "Send to server: " << argv[3] << "...\n";

    if (!sock) {
        std::cerr << sock_wrap.get_last_error_string() << std::endl;
        return EXIT_FAILURE;
    }

    sockaddr_in addr =
    {
        .sin_family = PF_INET,
        .sin_port = htons(port),
        .sin_addr = inet_addr(host.c_str())
    };

    if (connect(sock, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)) != 0) {
        std::cerr << sock_wrap.get_last_error_string() << std::endl;
        // Socket will be closed in the Socket destructor.
        return EXIT_FAILURE;
    }

    sendto(sock, argv[3], MAX_MESSAGE_LENGTH, 0, (struct sockaddr *) NULL, sizeof(addr));

    if (strcmp((char*)argv[3], "exit")) {
        char buffer[MAX_MESSAGE_LENGTH];
        recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *) NULL, NULL);

        std::cout << "Answer from server: " << buffer << std::endl;
    }

    return EXIT_SUCCESS;
}

