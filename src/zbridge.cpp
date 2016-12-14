//
// Created by WebSter on 14.12.2016.
//

#include <unordered_map>
#include "Data.h"
#define RECLEN_TYPE uint16_t

extern unordered_map<addr_id, Data> storage;
extern bool blockingMode;

// Прочесть len байтов из дейтаграммы для aid и записать по указателю bp.
int ureadn(addr_id aid, char* bp, int len) {
    if(storage.at(aid).empty()) {
        if(blockingMode) {
            while(storage.at(aid).empty()) { Sleep(10); };
        } else {
            set_errno(WSAEWOULDBLOCK);
            return -1;
        }
    }

    size_t size;
    char dgram[MAX_BUF_SIZE];
    char* ptr;

    size = storage.at(aid).getDataArrayCopy(dgram);
    ptr = storage.at(aid).getCurrentDataPointer();

    // Assuming all len bytes are in a single datagram
    int cnt = 0;
    while(cnt < len && cnt < size) {
        *bp++ = *ptr++;
        cnt++;
    }
    if(cnt == size) fprintf(stderr, "Datagram end reached in ureadn()!\n");

//    int cnt, rc;
//
//    cnt = len;
//    while(cnt > 0) {
//        //rc = recv(sock, bp, cnt, 0);
//        if(rc < 0) {                // Ошибка чтения?..
//            if(errno == EINTR)      // Или вызов был прерван?
//                continue;           // Повторить чтение
//            return -1;              // Вернуть код ошибки
//        }
//        if(rc == 0) {               // Если ничего не принято,
//            return len - cnt;       // вернуть неполный счетчик.
//        }
//        bp += rc;
//        cnt -= rc;
//    }
    storage.at(aid).setCurrentDataPointer(ptr);
    return len;
}

// Прочесть из дейтаграммы для aid запись переменной длины и записать по
// по указателю bp. Максимальный размер буфера для записи равен len.
// Максимальный размер записи определяется типом RECLEN_TYPE.
int ureadvrec(addr_id aid, char* bp, int len) {
    RECLEN_TYPE reclen;
    int rc;

    // Прочитать длину записи:
    rc = ureadn(aid, (char*) &reclen, sizeof(RECLEN_TYPE));
    // Проверка, что считалось верное число байт:
    if(rc != sizeof(RECLEN_TYPE)) return rc < 0 ? -1 : 0;
    switch (sizeof(RECLEN_TYPE)) {
        case 2: reclen = ntohs(reclen); break; //uint16_t
        case 4: reclen = ntohl(reclen); break; //uint32_t
        default: break;
    }
    if(reclen > len) {
        // В буфере не хватает места для размещения данных -
        // сохраняем, что влезает, а остальное отбрасываем.
        while(reclen > 0) {
            rc = ureadn(aid, bp, len);
            if(rc != len) return rc < 0 ? -1 : 0;
            reclen -= len;
            if(reclen < len) len = reclen;
        }
        set_errno(EMSGSIZE);
        return -1;
    }
    // Если всё в порядке, читаем запись:
    rc = ureadn(aid, bp, reclen);
    if(rc != reclen) return rc < 0 ? -1 : 0;
    return rc;
}

// Прочесть из дейтаграммы для aid одну строку (до '\n') по указателю bufptr.
// Максимальный размер буфера для записи равен len.
int ureadline(addr_id aid, char* bufptr, int len) {
    //char* bufx = bufptr;
    //static char* bp;
    //static int cnt = 0;
    //static char b[65536];
    //char c;

    if(storage.at(aid).empty()) {
        if(blockingMode) {
            while(storage.at(aid).empty()) { Sleep(10); };
        } else {
            set_errno(WSAEWOULDBLOCK);
            return -1;
        }
    }

    size_t size;
    char dgram[MAX_BUF_SIZE];
    char* ptr;

    size = storage.at(aid).getDataArrayCopy(dgram);
    ptr = storage.at(aid).getCurrentDataPointer();

    int cnt = 0;
    while(*ptr != '\n' && *ptr != '\0' && cnt < size) {
        *bufptr++ = *ptr++;
        cnt++;
    }
    if(*ptr == '\0') fprintf(stderr, "Symbol '\\0' met before '\\n' in ureadline()!\n");
    *bufptr = '\n';
    if(cnt == size) fprintf(stderr, "Datagram end reached in ureadline()!\n");

    return cnt;
//    while (--len > 0) {
//        cnt = recv(aid, &c, 1, 0);
//        if (cnt < 0) {
//            if (errno == EINTR) {
//                len++; /* Уменьшим на 1 в заголовке while. */
//                continue;
//            }
//            return -1;
//        }
//        if (cnt == 0) return 0;
//        *bufptr++ = c;
//        if (c == '\n') {
//            *bufptr = '\0';
//            return (int) (bufptr - bufx);
//        }
//    }

//    Snader:
//    while (--len > 0) {
//        if(--cnt <= 0) {
//            cnt = recv(sock, b, sizeof(b), 0);
//            if(cnt < 0) {
//                if(errno == EINTR) {
//                    len++; /* Уменьшим на 1 в заголовке while. */
//                    continue;
//                }
//                return -1;
//            }
//            if(cnt == 0) return 0;
//            bp = b;
//        }
//        c = *bp++;
//        *bufptr++ = c;
//        if(c == '\n') {
//            *bufptr = '\0';
//            return (int) (bufptr - bufx);
//        }
//    }

//    set_errno(EMSGSIZE);
//    return -1;
}