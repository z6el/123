#ifndef decryption_h
#define decryption_h

#include "../include.h"

namespace decryptions {
    inline uint64_t Il2cppGetHandle(int32_t ObjectHandleID) {
        uint64_t index = (uint64_t)(ObjectHandleID >> 3);
        uint64_t offset = (uint64_t)((ObjectHandleID & 7) - 1);
        uintptr_t baseAddr = globals::game_assembly + offsets::il2cpp_handle_base + offset * 0x28;
        uint32_t limit = m.read<uint32_t>(baseAddr + 0x10);
        if (index >= limit) return 0;

        uintptr_t objAddr = m.read<uintptr_t>(baseAddr);
        if (!objAddr) return 0;

        uint32_t bitMask = m.read<uint32_t>(objAddr + ((index >> 5) << 2));
        if (!(bitMask & (1 << (index & 0x1F)))) return 0;

        uintptr_t ObjectArray = m.read<uintptr_t>(baseAddr + 0x8) + (index << 3);
        return m.read<uint8_t>(baseAddr + 0x14) > 1 ? m.read<uintptr_t>(ObjectArray) : ~m.read<uint32_t>(ObjectArray);
    }

    inline uint64_t decrypt_base_networkable(uint64_t address) {
        uint64_t rax = m.read<uint64_t>(address + 0x18);
        uint32_t* ptr = (uint32_t*)&rax;
        uint32_t count = 2;
        do {
            uint32_t eax = *ptr;
            ptr++;
            eax ^= 0x8099F03B;
            eax += 0xD170F063;
            uint32_t ecx = eax;
            eax <<= 0xB;
            ecx >>= 0x15;
            ecx |= eax;
            ecx += 0x179959E1;
            *(ptr - 1) = ecx;
            --count;
        } while (count);
        return Il2cppGetHandle((int32_t)rax);
    }

    inline uint64_t decrypt_base_networkable_list(uint64_t address) {
        uint64_t rax = m.read<uint64_t>(address + 0x18);
        uint32_t* ptr = (uint32_t*)&rax;
        uint32_t count = 2;
        do {
            uint32_t eax = *ptr;
            ptr++;
            eax ^= 0xFDB6056B;
            uint32_t ecx = eax;
            eax <<= 0x6;
            ecx >>= 0x1A;
            ecx |= eax;
            ecx += 0xC2BF6C0D;
            eax = ecx;
            ecx <<= 0xB;
            eax >>= 0x15;
            eax |= ecx;
            *(ptr - 1) = eax;
            --count;
        } while (count);
        return Il2cppGetHandle((int32_t)rax);
    }

    inline uint64_t decrypt_player_inventory(uint64_t address) {
        uint64_t rax = m.read<uint64_t>(address + 0x18);
        uint32_t count = 2;
        uint32_t* ptr = (uint32_t*)&rax;
        do {
            uint32_t ecx = *ptr;
            uint32_t eax = *ptr;
            ptr++;
            eax <<= 9;
            ecx >>= 0x17;
            ecx |= eax;
            ecx ^= 0x6D74614B;
            eax = ecx;
            ecx <<= 0x0F;
            eax >>= 0x11;
            eax |= ecx;
            *(ptr - 1) = eax;
            --count;
        } while (count);
        return Il2cppGetHandle((int32_t)rax);
    }

    inline uint64_t decrypt_player_eyes(uint64_t address) {
        uint64_t rax = m.read<uint64_t>(address + 0x18);
        uint32_t count = 2;
        uint32_t* ptr = (uint32_t*)&rax;
        do {
            uint32_t ecx = *ptr;
            ptr++;
            ecx ^= 0xFDB6056B;
            uint32_t eax = ecx;
            ecx <<= 6;
            eax >>= 0x1A;
            eax |= ecx;
            eax += 0xC2BF6C0D;
            ecx = eax;
            eax <<= 0x0B;
            ecx >>= 0x15;
            ecx |= eax;
            *(ptr - 1) = ecx;
            --count;
        } while (count);
        return Il2cppGetHandle((int32_t)rax);
    }

    inline uintptr_t decrypt_cl_active_item(uintptr_t encrypted_value) {
        uintptr_t rax = encrypted_value;
        uint32_t* ptr = (uint32_t*)&rax;
        uint32_t count = 2;
        do {
            uint32_t v3 = *ptr;
            uint32_t v4 = *ptr;
            ptr++;
            uint32_t v5 = (((v4 ^ 0xA399A175) << 26) | (v3 >> 6)) + 0xEFC2C5D5 ^ 0x68D9648C;
            *(ptr - 1) = v5;
            --count;
        } while (count);
        return rax;
    }

    inline uint64_t encrypt_fov(uint32_t fov_bits) {
        uint32_t eax = fov_bits;
        eax += 0x48426F13;
        eax -= 0x145FD915;
        eax = (eax << 0x7) | (eax >> 0x19);
        return static_cast<uint64_t>(eax);
    }

    inline uint32_t decrypt_fov(uint64_t encrypted) {
        uint32_t eax = static_cast<uint32_t>(encrypted);
        eax = (eax >> 0x7) | (eax << 0x19);
        eax += 0x145FD915;
        eax -= 0x48426F13;
        return eax;
    }

    inline void set_fov(float fov) {
        uint32_t bits = *reinterpret_cast<uint32_t*>(&fov);
        uintptr_t graphicfov = m.read<uintptr_t>(globals::game_assembly + offsets::graphics_typeinfo);
        if (!graphicfov) return;
        uintptr_t field = m.read<uintptr_t>(graphicfov + 0xB8);
        if (!field) return;
        uint64_t enc = encrypt_fov(bits);
        if (!enc) return;
        m.write<uint32_t>(field + offsets::graphic::fov, static_cast<uint32_t>(enc));
    }
}

#endif
