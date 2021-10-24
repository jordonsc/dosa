#pragma once

#include <Arduino.h>

namespace dosa {

class Base64
{
   public:
    static String decode(char const* input)
    {
        String output = "";

        int i = 0, j;
        unsigned char A3[3];
        unsigned char A4[4];

        while (*input != '=' && *input != 0) {
            A4[i++] = *(input++);
            if (i == 4) {
                for (i = 0; i < 4; i++) {
                    A4[i] = lookupTable(A4[i]);
                }

                fromA4ToA3(A3, A4);

                for (i = 0; i < 3; i++) {
                    output += char(A3[i]);
                }

                i = 0;
            }
        }

        if (i) {
            for (j = i; j < 4; j++) {
                A4[j] = '\0';
            }

            for (j = 0; j < 4; j++) {
                A4[j] = lookupTable(A4[j]);
            }

            fromA4ToA3(A3, A4);

            for (j = 0; j < i - 1; j++) {
                output += char(A3[j]);
            }
        }

        return output;
    }

   private:
    static void fromA3ToA4(unsigned char* A4, unsigned char const* A3)
    {
        A4[0] = (A3[0] & 0xfc) >> 2;
        A4[1] = ((A3[0] & 0x03) << 4) + ((A3[1] & 0xf0) >> 4);
        A4[2] = ((A3[1] & 0x0f) << 2) + ((A3[2] & 0xc0) >> 6);
        A4[3] = (A3[2] & 0x3f);
    }

    static void fromA4ToA3(unsigned char* A3, unsigned char const* A4)
    {
        A3[0] = (A4[0] << 2) + ((A4[1] & 0x30) >> 4);
        A3[1] = ((A4[1] & 0xf) << 4) + ((A4[2] & 0x3c) >> 2);
        A3[2] = ((A4[2] & 0x3) << 6) + A4[3];
    }

    static unsigned char lookupTable(unsigned char c)
    {
        if (c >= 'A' && c <= 'Z')
            return c - 'A';
        if (c >= 'a' && c <= 'z')
            return c - 71;
        if (c >= '0' && c <= '9')
            return c + 4;
        if (c == '+')
            return 62;
        if (c == '/')
            return 63;
        return 0;
    }
};

}  // namespace dosa