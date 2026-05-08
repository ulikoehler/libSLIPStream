/**
 * @file CRC32.cpp
 * @author Uli Köhler <github@techoverflow.net>
 * @version 1.0
 * @date 2025-08-19
 *
 * CRC32 calculation using Ethernet polynomial
 *
 * @copyright Copyright (C) 2022..2025 Uli Köhler
 */
#include "SLIPStream/CRC32.hpp"

namespace SLIPStream {

// CRC32 polynomial (Ethernet polynomial)
constexpr uint32_t CRC32_POLYNOMIAL = 0x04C11DB7;

// Pre-computed CRC32 lookup table (matching Python implementation)
static const uint32_t crc32_table[256] = {
    0x00000000, 0x09823B6E, 0x130476DC, 0x1A864DB2, 0x2608EDB8, 0x2F8AD6D6, 0x350C9B64, 0x3C8EA00A,
    0x4C11DB70, 0x4593E01E, 0x5F15ADAC, 0x569796C2, 0x6A1936C8, 0x639B0DA6, 0x791D4014, 0x709F7B7A,
    0x9823B6E0, 0x91A18D8E, 0x8B27C03C, 0x82A5FB52, 0xBE2B5B58, 0xB7A96036, 0xAD2F2D84, 0xA4AD16EA,
    0xD4326D90, 0xDDB056FE, 0xC7361B4C, 0xCEB42022, 0xF23A8028, 0xFBB8BB46, 0xE13EF6F4, 0xE8BCCD9A,
    0x39C556AE, 0x30476DC0, 0x2AC12072, 0x23431B1C, 0x1FCDBB16, 0x164F8078, 0x0CC9CDCA, 0x054BF6A4,
    0x75D48DDE, 0x7C56B6B0, 0x66D0FB02, 0x6F52C06C, 0x53DC6066, 0x5A5E5B08, 0x40D816BA, 0x495A2DD4,
    0xA1E6E04E, 0xA864DB20, 0xB2E29692, 0xBB60ADFC, 0x87EE0DF6, 0x8E6C3698, 0x94EA7B2A, 0x9D684044,
    0xEDF73B3E, 0xE4750050, 0xFEF34DE2, 0xF771768C, 0xCBFFD686, 0xC27DEDE8, 0xD8FBA05A, 0xD1799B34,
    0x738AAD5C, 0x7A089632, 0x608EDB80, 0x690CE0EE, 0x558240E4, 0x5C007B8A, 0x46863638, 0x4F040D56,
    0x3F9B762C, 0x36194D42, 0x2C9F00F0, 0x251D3B9E, 0x19939B94, 0x1011A0FA, 0x0A97ED48, 0x0315D626,
    0xEBA91BBC, 0xE22B20D2, 0xF8AD6D60, 0xF12F560E, 0xCDA1F604, 0xC423CD6A, 0xDEA580D8, 0xD727BBB6,
    0xA7B8C0CC, 0xAE3AFBA2, 0xB4BCB610, 0xBD3E8D7E, 0x81B02D74, 0x8832161A, 0x92B45BA8, 0x9B3660C6,
    0x4A4FFBF2, 0x43CDC09C, 0x594B8D2E, 0x50C9B640, 0x6C47164A, 0x65C52D24, 0x7F436096, 0x76C15BF8,
    0x065E2082, 0x0FDC1BEC, 0x155A565E, 0x1CD86D30, 0x2056CD3A, 0x29D4F654, 0x3352BBE6, 0x3AD08088,
    0xD26C4D12, 0xDBEE767C, 0xC1683BCE, 0xC8EA00A0, 0xF464A0AA, 0xFDE69BC4, 0xE760D676, 0xEEE2ED18,
    0x9E7D9662, 0x97FFAD0C, 0x8D79E0BE, 0x84FBDBD0, 0xB8757BDA, 0xB1F740B4, 0xAB710D06, 0xA2F33668,
    0xE7155AB8, 0xEE9761D6, 0xF4112C64, 0xFD93170A, 0xC11DB700, 0xC89F8C6E, 0xD219C1DC, 0xDB9BFAB2,
    0xAB0481C8, 0xA286BAA6, 0xB800F714, 0xB182CC7A, 0x8D0C6C70, 0x848E571E, 0x9E081AAC, 0x978A21C2,
    0x7F36EC58, 0x76B4D736, 0x6C329A84, 0x65B0A1EA, 0x593E01E0, 0x50BC3A8E, 0x4A3A773C, 0x43B84C52,
    0x33273728, 0x3AA50C46, 0x202341F4, 0x29A17A9A, 0x152FDA90, 0x1CADE1FE, 0x062BAC4C, 0x0FA99722,
    0xDED00C16, 0xD7523778, 0xCDD47ACA, 0xC45641A4, 0xF8D8E1AE, 0xF15ADAC0, 0xEBDC9772, 0xE25EAC1C,
    0x92C1D766, 0x9B43EC08, 0x81C5A1BA, 0x88479AD4, 0xB4C93ADE, 0xBD4B01B0, 0xA7CD4C02, 0xAE4F776C,
    0x46F3BAF6, 0x4F718198, 0x55F7CC2A, 0x5C75F744, 0x60FB574E, 0x69796C20, 0x73FF2192, 0x7A7D1AFC,
    0x0AE26186, 0x03605AE8, 0x19E6175A, 0x10642C34, 0x2CEA8C3E, 0x2568B750, 0x3FEEFAE2, 0x366CC18C,
    0x949FF7E4, 0x9D1DCC8A, 0x879B8138, 0x8E19BA56, 0xB2971A5C, 0xBB152132, 0xA1936C80, 0xA81157EE,
    0xD88E2C94, 0xD10C17FA, 0xCB8A5A48, 0xC2086126, 0xFE86C12C, 0xF704FA42, 0xED82B7F0, 0xE4008C9E,
    0x0CBC4104, 0x053E7A6A, 0x1FB837D8, 0x163A0CB6, 0x2AB4ACBC, 0x233697D2, 0x39B0DA60, 0x3032E10E,
    0x40AD9A74, 0x492FA11A, 0x53A9ECA8, 0x5A2BD7C6, 0x66A577CC, 0x6F274CA2, 0x75A10110, 0x7C233A7E,
    0xAD5AA14A, 0xA4D89A24, 0xBE5ED796, 0xB7DCECF8, 0x8B524CF2, 0x82D0779C, 0x98563A2E, 0x91D40140,
    0xE14B7A3A, 0xE8C94154, 0xF24F0CE6, 0xFBCD3788, 0xC7439782, 0xCEC1ACEC, 0xD447E15E, 0xDDC5DA30,
    0x357917AA, 0x3CFB2CC4, 0x267D6176, 0x2FFF5A18, 0x1371FA12, 0x1AF3C17C, 0x00758CCE, 0x09F7B7A0,
    0x7968CCDA, 0x70EAF7B4, 0x6A6CBA06, 0x63EE8168, 0x5F602162, 0x56E21A0C, 0x4C6457BE, 0x45E66CD0
};

uint32_t calculate_crc32(const uint8_t* data, size_t length) {
    return calculate_crc32_with_initial(data, length, 0xFFFFFFFF);
}

uint32_t calculate_crc32_with_initial(const uint8_t* data, size_t length, uint32_t initial_crc) {
    uint32_t crc = initial_crc;
    
    for (size_t i = 0; i < length; i++) {
        uint8_t byte = data[i];
        crc = ((crc << 8) ^ crc32_table[((crc >> 24) ^ byte) & 0xFF]) & 0xFFFFFFFF;
    }
    
    return crc;
}

size_t append_crc32(uint8_t* data, size_t length) {
    if (data == nullptr) {
        return length;
    }
    
    uint32_t crc = calculate_crc32(data, length);
    
    // Append CRC in little-endian format (LSB first)
    data[length] = crc & 0xFF;
    data[length + 1] = (crc >> 8) & 0xFF;
    data[length + 2] = (crc >> 16) & 0xFF;
    data[length + 3] = (crc >> 24) & 0xFF;
    
    return length + 4;
}

size_t extract_crc32(const uint8_t* data, size_t length, uint32_t* crc_out) {
    if (data == nullptr || length < 4 || crc_out == nullptr) {
        return 0;
    }
    
    // Extract CRC from last 4 bytes in little-endian format
    *crc_out = static_cast<uint32_t>(data[length - 4]) |
               (static_cast<uint32_t>(data[length - 3]) << 8) |
               (static_cast<uint32_t>(data[length - 2]) << 16) |
               (static_cast<uint32_t>(data[length - 1]) << 24);
    
    return length - 4;
}

bool verify_crc32(const uint8_t* data, size_t length) {
    if (data == nullptr || length < 4) {
        return false;
    }
    
    uint32_t stored_crc;
    size_t data_length = extract_crc32(data, length, &stored_crc);
    
    if (data_length == 0) {
        return false;
    }
    
    uint32_t calculated_crc = calculate_crc32(data, data_length);
    
    return calculated_crc == stored_crc;
}

} // namespace SLIPStream
