#ifndef BYTEORDER_H
#define BYTEORDER_H

#ifndef BYTE_ORDER
# error "Aiee: BYTE_ORDER not defined\n";
#endif

#define SWAP2(x) (((x>>8) & 0x00ff) |\
                  ((x<<8) & 0xff00))

#define SWAP4(x) (((x>>24) & 0x000000ff) |\
                  ((x>>8)  & 0x0000ff00) |\
                  ((x<<8)  & 0x00ff0000) |\
                  ((x<<24) & 0xff000000))

#endif /* BYTEORDER_H */
