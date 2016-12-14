#include "etcp.h"

// Прочесть len байтов из сокета sock и записать по указателю bp.
int readn(addr_id aid, char* bp, int len) {
    return ureadn(aid, bp, len);
}

// Прочесть из сокета sock запись переменной длины и записать по
// по указателю bp. Максимальный размер буфера для записи равен len.
// Максимальный размер записи определяется типом RECLEN_TYPE.
int readvrec(addr_id aid, char* bp, int len) {
    return ureadvrec(aid, bp, len);
}

// Прочесть из сокета sock одну строку (до '\n') по указателю bufptr.
// Максимальный размер буфера для записи равен len.
int readline(addr_id aid, char* bufptr, int len) {
    return ureadline(aid, bufptr, len);
}