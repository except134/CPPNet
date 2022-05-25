#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

#include <socket_wrapper/socket_headers.h>
#include <socket_wrapper/socket_wrapper.h>
#include <socket_wrapper/socket_class.h>
#include <cstring>
#include <netinet/ip_icmp.h>

#define	ICMP_PING_LENGTH 64

std::string ip_as_string(uint32_t num)
{
    std::string ret;
    ret += std::to_string(num & 0xFF);
    ret += ".";
    ret += std::to_string(num >> 8 & 0xFF);
    ret += ".";
    ret += std::to_string(num >> 16 & 0xFF);
    ret += ".";
    ret += std::to_string(num >> 24);
    return ret;
}

typedef struct
{
    struct icmp hdr;
    char data[ICMP_PING_LENGTH];
} ping_pkt_t;

static ushort checksum(void *b, int len)
{
    ushort *buf = (ushort*)b;
    uint sum=0;
    ushort result;

    for (sum = 0; len > 1; len -= 2) {
        sum += *buf++;
    }
    if (len == 1) {
        sum += *(unsigned char*)buf;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

static void prepare_icmp_pkt(ping_pkt_t* ping_pkt)
{
    memset(ping_pkt, 0, sizeof(ping_pkt_t));
    srand(time(NULL));
    const short random_id = rand();
    ping_pkt->hdr.icmp_hun.ih_idseq.icd_id = random_id;
    ping_pkt->hdr.icmp_type = ICMP_ECHO;
    ping_pkt->hdr.icmp_hun.ih_idseq.icd_seq = 0;

    int i = 0;
    for (i = 0; i < sizeof(ping_pkt->data)-1; i++)
        ping_pkt->data[i] = i+'0';
    ping_pkt->data[i] = 0;

    ping_pkt->hdr.icmp_cksum = checksum(ping_pkt, sizeof(ping_pkt_t));
}

int main(int argc, char const *argv[])
{

    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <host>" << std::endl;
        return EXIT_FAILURE;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    if (addr.sin_addr.s_addr == (u_int)-1) {
        hostent *hp = gethostbyname(argv[1]);
        if (!hp) {
            std::cerr << "Unknown host " << argv[1] << std::endl;
            return EXIT_FAILURE;
        } else {
            memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);
        }
    }

    socklen_t len;
    char host[100], *ret_buf;
    len = sizeof(struct sockaddr_in);
    if (getnameinfo((sockaddr*)&addr, sizeof(sockaddr_in), host, sizeof(host), NULL, 0, NI_NAMEREQD)) {
        std::cerr << "Could not resolve reverse lookup of hostname\n";
    }

    std::cout << "Pinging " << host << "(" << ip_as_string(addr.sin_addr.s_addr) << ")\n";

    socket_wrapper::SocketWrapper sock_wrap;
    socket_wrapper::Socket sock = {AF_INET, SOCK_RAW, IPPROTO_ICMP };

    if (!sock)
    {
        std::cerr << sock_wrap.get_last_error_string() << std::endl;
        return EXIT_FAILURE;
    }

    int time_limit = 1000000;
    timeval tv;
    tv.tv_sec = time_limit / 1000000;
    tv.tv_usec = time_limit % 1000000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));

    ping_pkt_t pckt;

    while (true)
    {
        ssize_t recv_len = recvfrom(sock, &pckt, sizeof(pckt), 0, reinterpret_cast<sockaddr *>(&addr), &len);
        if (recv_len > 0)
        {
            std::cout << "Ping." << std::endl;
        }

        prepare_icmp_pkt(&pckt);

        if ( sendto(sock, &pckt, sizeof(pckt), 0, (struct sockaddr*)&addr, sizeof(addr)) <= 0 )
            std::cerr << "Failed sending ping packet.\n";

        usleep(300000);
    }

    return EXIT_SUCCESS;
}

