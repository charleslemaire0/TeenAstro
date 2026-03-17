/*
 * TcpStream.h -- Arduino Stream interface over TCP sockets (Winsock2).
 *
 * TcpServerStream: listens on a port, accepts one client (non-blocking).
 * TcpClientStream: connects to host:port.
 * Both expose the Arduino Stream API (available, read, write, etc.).
 *
 * IMPORTANT: This header must be included BEFORE arduino.h or any header
 * that defines INPUT/OUTPUT macros, because Windows headers use INPUT as
 * a struct name. We save and restore those macros around the Windows includes.
 */
#pragma once

#ifdef _WIN32
#  ifndef _WIN32_WINNT
#    define _WIN32_WINNT 0x0601
#  endif
#  ifndef _WINSOCK2API_
#    ifdef INPUT
#      define _SAVED_INPUT INPUT
#      undef INPUT
#    endif
#    ifdef OUTPUT
#      define _SAVED_OUTPUT OUTPUT
#      undef OUTPUT
#    endif
#    include <winsock2.h>
#    include <ws2tcpip.h>
#    ifdef _SAVED_INPUT
#      define INPUT _SAVED_INPUT
#      undef _SAVED_INPUT
#    endif
#    ifdef _SAVED_OUTPUT
#      define OUTPUT _SAVED_OUTPUT
#      undef _SAVED_OUTPUT
#    endif
#  endif
#  pragma comment(lib, "ws2_32.lib")
   typedef SOCKET socket_t;
#  define INVALID_SOCK INVALID_SOCKET
#  define SOCK_CLOSE(s) closesocket(s)
#  define SOCK_ERR SOCKET_ERROR
#else
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <errno.h>
   typedef int socket_t;
#  define INVALID_SOCK (-1)
#  define SOCK_CLOSE(s) ::close(s)
#  define SOCK_ERR (-1)
#endif

/* Needs the Stream base class from arduino.h */
#include "arduino.h"
#include <cstdio>
#include <cstring>

namespace tcp_detail {

inline void initWsa() {
#ifdef _WIN32
    static bool inited = false;
    if (!inited) {
        WSADATA wd;
        WSAStartup(MAKEWORD(2, 2), &wd);
        inited = true;
    }
#endif
}

inline void setNonBlocking(socket_t s) {
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(s, FIONBIO, &mode);
#else
    int flags = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, flags | O_NONBLOCK);
#endif
}

} // namespace tcp_detail


class TcpServerStream : public Stream {
public:
    TcpServerStream() = default;
    ~TcpServerStream() { stop(); }

    bool listen(int port, bool anyInterface = false) {
        tcp_detail::initWsa();
        listen_sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listen_sock_ == INVALID_SOCK) return false;

        int opt = 1;
        setsockopt(listen_sock_, SOL_SOCKET, SO_REUSEADDR,
                   (const char*)&opt, sizeof(opt));

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = anyInterface ? htonl(INADDR_ANY) : htonl(INADDR_LOOPBACK);
        addr.sin_port = htons((uint16_t)port);

        if (bind(listen_sock_, (struct sockaddr*)&addr, sizeof(addr)) == SOCK_ERR) {
            SOCK_CLOSE(listen_sock_);
            listen_sock_ = INVALID_SOCK;
            return false;
        }
        if (::listen(listen_sock_, 1) == SOCK_ERR) {
            SOCK_CLOSE(listen_sock_);
            listen_sock_ = INVALID_SOCK;
            return false;
        }
        tcp_detail::setNonBlocking(listen_sock_);
        port_ = port;
        printf("[TcpServer] Listening on %s:%d\n",
               anyInterface ? "0.0.0.0" : "127.0.0.1", port);
        fflush(stdout);
        return true;
    }

    void acceptIfNeeded() {
        if (client_sock_ != INVALID_SOCK) return;
        if (listen_sock_ == INVALID_SOCK) return;
        struct sockaddr_in ca;
        int len = sizeof(ca);
        socket_t s = accept(listen_sock_, (struct sockaddr*)&ca,
#ifdef _WIN32
                            &len
#else
                            (socklen_t*)&len
#endif
                            );
        if (s != INVALID_SOCK) {
            tcp_detail::setNonBlocking(s);
            client_sock_ = s;
            printf("[TcpServer] Client connected on port %d\n", port_);
            fflush(stdout);
        }
    }

    void stop() {
        if (client_sock_ != INVALID_SOCK) { SOCK_CLOSE(client_sock_); client_sock_ = INVALID_SOCK; }
        if (listen_sock_ != INVALID_SOCK) { SOCK_CLOSE(listen_sock_); listen_sock_ = INVALID_SOCK; }
    }

    bool isListening() const { return listen_sock_ != INVALID_SOCK; }

    // Arduino Stream interface
    int available() override {
        acceptIfNeeded();
        if (client_sock_ == INVALID_SOCK) return 0;
        fillBuffer();
        return buf_len_ - buf_pos_;
    }

    int read() override {
        acceptIfNeeded();
        if (client_sock_ == INVALID_SOCK) return -1;
        fillBuffer();
        if (buf_pos_ >= buf_len_) return -1;
        return (int)(unsigned char)buf_[buf_pos_++];
    }

    int peek() override {
        acceptIfNeeded();
        if (client_sock_ == INVALID_SOCK) return -1;
        fillBuffer();
        if (buf_pos_ >= buf_len_) return -1;
        return (int)(unsigned char)buf_[buf_pos_];
    }

    size_t write(uint8_t b) override {
        if (client_sock_ == INVALID_SOCK) return 0;
        int r = send(client_sock_, (const char*)&b, 1, 0);
        if (r <= 0) { disconnectClient(); return 0; }
        return 1;
    }

    size_t write(const uint8_t* buf, size_t len) {
        if (client_sock_ == INVALID_SOCK) return 0;
        int r = send(client_sock_, (const char*)buf, (int)len, 0);
        if (r <= 0) { disconnectClient(); return 0; }
        return (size_t)r;
    }

    bool connected() const { return client_sock_ != INVALID_SOCK; }

    // compatibility with Arduino Serial
    void begin(unsigned long) { /* no-op, call listen() instead */ }

private:
    void fillBuffer() {
        if (buf_pos_ < buf_len_) return;
        buf_pos_ = buf_len_ = 0;
        if (client_sock_ == INVALID_SOCK) return;
        int r = recv(client_sock_, buf_, (int)sizeof(buf_), 0);
        if (r > 0) {
            buf_len_ = r;
        } else if (r == 0) {
            disconnectClient();
        } else if (!isWouldBlock()) {
            disconnectClient();
        }
    }

    static bool isWouldBlock() {
#ifdef _WIN32
        return WSAGetLastError() == WSAEWOULDBLOCK;
#else
        return errno == EWOULDBLOCK || errno == EAGAIN;
#endif
    }

    void disconnectClient() {
        if (client_sock_ != INVALID_SOCK) {
            SOCK_CLOSE(client_sock_);
            client_sock_ = INVALID_SOCK;
            printf("[TcpServer] Client disconnected on port %d\n", port_);
            fflush(stdout);
        }
    }

    socket_t listen_sock_ = INVALID_SOCK;
    socket_t client_sock_ = INVALID_SOCK;
    int port_ = 0;
    char buf_[512];
    int buf_pos_ = 0;
    int buf_len_ = 0;
};


/* Optional yield hook: called from TcpClientStream::available() while
   waiting for data, so the SHC emulator can pump SDL events + blit. */
#ifdef EMU_SHC
extern void _emu_shc_blit();
static inline void _tcp_yield() { _emu_shc_blit(); }
#else
static inline void _tcp_yield() {}
#endif

class TcpClientStream : public Stream {
public:
    TcpClientStream() = default;
    ~TcpClientStream() { stop(); }

    bool connect(const char* host, int port) {
        tcp_detail::initWsa();
        sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock_ == INVALID_SOCK) return false;

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons((uint16_t)port);
        addr.sin_addr.s_addr = inet_addr(host);

        if (::connect(sock_, (struct sockaddr*)&addr, sizeof(addr)) == SOCK_ERR) {
            SOCK_CLOSE(sock_);
            sock_ = INVALID_SOCK;
            return false;
        }
        tcp_detail::setNonBlocking(sock_);
        strncpy(host_, host, sizeof(host_) - 1);
        host_[sizeof(host_) - 1] = '\0';
        port_ = port;
        printf("[TcpClient] Connected to %s:%d\n", host, port);
        fflush(stdout);
        return true;
    }

    bool reconnect() {
        if (sock_ != INVALID_SOCK) return true;
        if (port_ == 0) return false;
        printf("[TcpClient] Attempting reconnect to %s:%d...\n", host_, port_);
        fflush(stdout);
        return connect(host_, port_);
    }

    void stop() {
        if (sock_ != INVALID_SOCK) { SOCK_CLOSE(sock_); sock_ = INVALID_SOCK; }
    }

    int available() override {
        if (sock_ == INVALID_SOCK) { reconnect(); return 0; }
        fillBuffer();
        int n = buf_len_ - buf_pos_;
        if (n == 0) _tcp_yield();
        return n;
    }

    int read() override {
        if (sock_ == INVALID_SOCK) return -1;
        fillBuffer();
        if (buf_pos_ >= buf_len_) return -1;
        return (int)(unsigned char)buf_[buf_pos_++];
    }

    int peek() override {
        if (sock_ == INVALID_SOCK) return -1;
        fillBuffer();
        if (buf_pos_ >= buf_len_) return -1;
        return (int)(unsigned char)buf_[buf_pos_];
    }

    size_t write(uint8_t b) override {
        if (sock_ == INVALID_SOCK) return 0;
        int r = send(sock_, (const char*)&b, 1, 0);
        if (r <= 0) { handleSendError(); return 0; }
        return 1;
    }

    size_t write(const uint8_t* buf, size_t len) {
        if (sock_ == INVALID_SOCK) return 0;
        int r = send(sock_, (const char*)buf, (int)len, 0);
        if (r <= 0) { handleSendError(); return 0; }
        return (size_t)r;
    }

    bool connected() const { return sock_ != INVALID_SOCK; }

    void begin(unsigned long) { /* no-op, call connect() instead */ }

private:
    void fillBuffer() {
        if (buf_pos_ < buf_len_) return;
        buf_pos_ = buf_len_ = 0;
        if (sock_ == INVALID_SOCK) return;
        int r = recv(sock_, buf_, (int)sizeof(buf_), 0);
        if (r > 0) {
            buf_len_ = r;
        } else if (r == 0) {
            disconnect();
        } else if (!isWouldBlock()) {
            disconnect();
        }
    }

    void disconnect() {
        if (sock_ != INVALID_SOCK) {
            SOCK_CLOSE(sock_);
            sock_ = INVALID_SOCK;
            printf("[TcpClient] Disconnected from %s:%d\n", host_, port_);
            fflush(stdout);
        }
    }

    void handleSendError() {
        if (!isWouldBlock()) disconnect();
    }

    static bool isWouldBlock() {
#ifdef _WIN32
        return WSAGetLastError() == WSAEWOULDBLOCK;
#else
        return errno == EWOULDBLOCK || errno == EAGAIN;
#endif
    }

    socket_t sock_ = INVALID_SOCK;
    char host_[64] = "";
    int port_ = 0;
    char buf_[512];
    int buf_pos_ = 0;
    int buf_len_ = 0;
};
