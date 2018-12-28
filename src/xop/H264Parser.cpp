#include "H264Parser.h"
#include <cstring>

using namespace xop;

Nal H264Parser::findNal(const uint8_t *data, uint32_t size)
{
    Nal nal(nullptr, nullptr);

    if(size < 5)
    {
        return nal;
    }

    nal.second = const_cast<uint8_t*>(data) + (size-1);

    uint32_t startCode = 0;
    uint32_t pos = 0;
    uint8_t prefix[3] = {0};
    memcpy(prefix, data, 3);
    size -= 3;
    data += 2;

    while(size--)
    {
        if ((prefix[pos % 3] == 0) && (prefix[(pos + 1) % 3] == 0) && (prefix[(pos + 2) % 3] == 1))
        {
            if(nal.first == nullptr) // 00 00 01 
            {
                nal.first = const_cast<uint8_t*>(data) + 1;
                startCode = 3;
            }
            else if(startCode == 3)
            {
                nal.second = const_cast<uint8_t*>(data) - 3;
                break;
            }               
        }
        else if ((prefix[pos % 3] == 0) && (prefix[(pos + 1) % 3] == 0) && (prefix[(pos + 2) % 3] == 0))
        {
            if (*(data+1) == 0x01) // 00 00 00 01 
            {              
                if(nal.first == nullptr)
                {
                    if(size >= 1)
                    {
                        nal.first = const_cast<uint8_t*>(data) + 2;
                    }                       
                    else
                    {
                        break;  
                    }                  
                    startCode = 4;
                }
                else if(startCode == 4)
                {
                    nal.second = const_cast<uint8_t*>(data) - 3;
                    break;
                }                    
            }
        }

        prefix[(pos++) % 3] = *(++data);
    }
        
    if(nal.first == nullptr)
        nal.second = nullptr;

    return nal;
}


