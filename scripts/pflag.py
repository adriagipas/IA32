#!/usr/bin/env python3

def parity(val):
    count= 0
    for i in range(0,8):
        if (val&0x1)!=0: count+= 1
        val>>= 1
    return True if ((count&0x1)==0) else False

ret= []
for i in range(0,256):
    par= parity(i)
    ret.append('PF_FLAG' if par else '0')
print('static const uint32_t PFLAG[256]= {%s};'%', '.join(ret))
