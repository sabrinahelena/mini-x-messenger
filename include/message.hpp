#ifndef MSG_HPP
#define MSG_HPP

#include <cstring>
#include <cstdint>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

struct Message {
    uint16_t type;
    uint16_t orig_uid;
    uint16_t dest_uid;
    uint16_t text_len;
    char text[141];

    void serialize(char* buffer) const {
        uint16_t* p = (uint16_t*) buffer;
        p[0] = htons(type);
        p[1] = htons(orig_uid);
        p[2] = htons(dest_uid);
        p[3] = htons(text_len);
        memcpy(buffer + 8, text, 141);
    }

    void deserialize(const char* buffer) {
        const uint16_t* p = (const uint16_t*) buffer;
        type = ntohs(p[0]);
        orig_uid = ntohs(p[1]);
        dest_uid = ntohs(p[2]);
        text_len = ntohs(p[3]);
        memcpy(text, buffer + 8, 141);
    }
};

#endif

