    #include <iostream>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <iphlpapi.h>
    #include <icmpapi.h>

    #pragma comment(lib, "ws2_32.lib")
    #pragma comment(lib, "iphlpapi.lib")

    void send_ping(const std::string& ip_address) {
        HANDLE hIcmpFile;
        DWORD dwRetVal = 0;
        char send_data[32] = "Ping test data";
        char reply_buffer[1024];
        ICMP_ECHO_REPLY* pEchoReply = reinterpret_cast<ICMP_ECHO_REPLY*>(reply_buffer);
        struct sockaddr_in sa;

        if (InetPtonA(AF_INET, ip_address.c_str(), &sa.sin_addr) != 1) {
            std::cerr << "Invalid IP address: " << ip_address << std::endl;
            return;
        }

        hIcmpFile = IcmpCreateFile();
        if (hIcmpFile == INVALID_HANDLE_VALUE) {
            std::cerr << "Unable to create ICMP handle. Error: " << GetLastError() << std::endl;
            return;
        }
        int Sucsess = 0;
        int Unsucsess = 0;
        for (int i = 0; i < 10; ++i) {

            dwRetVal = IcmpSendEcho(
                hIcmpFile,
                sa.sin_addr.S_un.S_addr,
                send_data,
                sizeof(send_data),
                nullptr,
                reply_buffer,
                sizeof(reply_buffer),
                1000
            );

            if (dwRetVal > 0) {
                std::cout << "Reply from " << ip_address << ": bytes=" << pEchoReply->DataSize
                    << " time=" << pEchoReply->RoundTripTime << "ms TTL=" << (int)pEchoReply->Options.Ttl << std::endl;
                Sucsess++;
            }
            else {
                std::cerr << "Request timed out or failed. Error: " << GetLastError() << std::endl;
                Unsucsess++;
            }
        }
        std::cout << "\nStats of Ping  " << ip_address <<"\n";
        std::cout << "Sucsess Pockets: "<< Sucsess<<"\n";
        std::cout << "Unsucsess Pockets: "<< Unsucsess <<"\n";
        IcmpCloseHandle(hIcmpFile);
    }

    int main(int argc, char* argv[]) {
        if (argc != 2) {
            std::cerr << "Usage: " << argv[0] << " <IP_ADDRESS>" << std::endl;
            return EXIT_FAILURE;
        }

        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << "WSAStartup failed. Error: " << result << std::endl;
            return EXIT_FAILURE;
        }

        std::string ip_address = argv[1];
        send_ping(ip_address);

        WSACleanup();
        return 0;
    }
