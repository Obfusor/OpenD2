#!/usr/bin/env python3
"""Extract files from MPQ v1 archives using PKWARE DCL decompression."""
import struct, ctypes, os, sys

# Load PKWARE explode DLL
script_dir = os.path.dirname(os.path.abspath(__file__))
pk_dll = ctypes.CDLL(os.path.join(script_dir, "pkexplode.dll"))
pk_dll.pk_explode.argtypes = [
    ctypes.c_char_p,
    ctypes.c_uint,
    ctypes.c_char_p,
    ctypes.c_uint,
    ctypes.POINTER(ctypes.c_uint),
]
pk_dll.pk_explode.restype = ctypes.c_int


def pk_explode(data, expected_size):
    out_buf = ctypes.create_string_buffer(expected_size + 4096)
    written = ctypes.c_uint(0)
    result = pk_dll.pk_explode(
        data, len(data), out_buf, expected_size + 4096, ctypes.byref(written)
    )
    if result != 0:
        raise RuntimeError(f"pk_explode failed with code {result}")
    return out_buf.raw[: written.value]


# MPQ Encryption table
crypt_table = [0] * 0x500


def build_crypt_table():
    seed = 0x00100001
    for i in range(256):
        idx = i
        for j in range(5):
            seed = (seed * 125 + 3) % 0x2AAAAB
            t1 = (seed & 0xFFFF) << 16
            seed = (seed * 125 + 3) % 0x2AAAAB
            t2 = seed & 0xFFFF
            crypt_table[idx] = (t1 | t2) & 0xFFFFFFFF
            idx += 256


build_crypt_table()


def hash_string(s, hash_type):
    seed1 = 0x7FED7FED
    seed2 = 0xEEEEEEEE
    for c in s.upper():
        ch = ord(c)
        seed1 = crypt_table[hash_type * 256 + ch] ^ ((seed1 + seed2) & 0xFFFFFFFF)
        seed1 &= 0xFFFFFFFF
        seed2 = (ch + seed1 + seed2 + (seed2 << 5) + 3) & 0xFFFFFFFF
    return seed1


def decrypt_block(data, key):
    padded = data + b"\x00" * ((4 - len(data) % 4) % 4)
    seed = 0xEEEEEEEE
    result = bytearray()
    for i in range(0, len(padded), 4):
        seed = (seed + crypt_table[0x400 + (key & 0xFF)]) & 0xFFFFFFFF
        ch = struct.unpack_from("<I", padded, i)[0]
        ch = (ch ^ (key + seed)) & 0xFFFFFFFF
        key = (((~key << 0x15) + 0x11111111) | (key >> 0x0B)) & 0xFFFFFFFF
        seed = (ch + seed + (seed << 5) + 3) & 0xFFFFFFFF
        result += struct.pack("<I", ch)
    return bytes(result[: len(data)])


class MPQ:
    def __init__(self, path):
        with open(path, "rb") as f:
            self.data = f.read()
        assert self.data[:4] == b"MPQ\x1a"
        self.sector_shift = struct.unpack_from("<H", self.data, 14)[0]
        self.sector_size = 512 << self.sector_shift
        ht_off = struct.unpack_from("<I", self.data, 16)[0]
        bt_off = struct.unpack_from("<I", self.data, 20)[0]
        ht_count = struct.unpack_from("<I", self.data, 24)[0]
        bt_count = struct.unpack_from("<I", self.data, 28)[0]

        ht_dec = decrypt_block(self.data[ht_off : ht_off + ht_count * 16], 0xC3AF3770)
        self.hash_table = [
            struct.unpack_from("<IIHHI", ht_dec, i * 16) for i in range(ht_count)
        ]
        self.ht_count = ht_count

        bt_dec = decrypt_block(self.data[bt_off : bt_off + bt_count * 16], 0xEC83B3A3)
        self.block_table = [
            struct.unpack_from("<IIII", bt_dec, i * 16) for i in range(bt_count)
        ]

    def find(self, filename):
        ha = hash_string(filename, 1)
        hb = hash_string(filename, 2)
        start = hash_string(filename, 0) % self.ht_count
        for i in range(self.ht_count):
            e = self.hash_table[(start + i) % self.ht_count]
            if e[4] == 0xFFFFFFFF:
                return None
            if e[0] == ha and e[1] == hb:
                return e[4]
        return None

    def read(self, filename):
        bi = self.find(filename)
        if bi is None:
            return None
        offset, csize, fsize, flags = self.block_table[bi]
        if not (flags & 0x80000000):
            return None

        is_encrypted = bool(flags & 0x10000)
        is_compressed = bool(flags & 0x200)
        is_imploded = bool(flags & 0x100)
        is_single = bool(flags & 0x1000000)
        is_fixkey = bool(flags & 0x20000)

        fkey = 0
        if is_encrypted:
            fkey = hash_string(filename.rsplit("\\", 1)[-1], 3)
            if is_fixkey:
                fkey = ((fkey + offset) ^ fsize) & 0xFFFFFFFF

        raw = self.data[offset : offset + csize]

        if is_single:
            if is_encrypted:
                raw = decrypt_block(raw, fkey)
            if (is_compressed or is_imploded) and csize < fsize:
                return pk_explode(raw, fsize)
            return raw[:fsize]

        # Multi-sector
        nsectors = (fsize + self.sector_size - 1) // self.sector_size
        sot = raw[: 4 * (nsectors + 1)]
        if is_encrypted:
            sot = decrypt_block(sot, fkey - 1)
        offsets = [struct.unpack_from("<I", sot, i * 4)[0] for i in range(nsectors + 1)]

        result = bytearray()
        for i in range(nsectors):
            sd = raw[offsets[i] : offsets[i + 1]]
            if is_encrypted:
                sd = decrypt_block(sd, fkey + i)
            expected = min(self.sector_size, fsize - i * self.sector_size)
            if (is_compressed or is_imploded) and len(sd) < expected:
                sd = pk_explode(sd, expected)
            result += sd
        return bytes(result[:fsize])


if __name__ == "__main__":
    mpq_path = (
        sys.argv[1] if len(sys.argv) > 1 else r"C:\Diablo2\ProjectD2\Live\pd2data.mpq"
    )
    out_dir = sys.argv[2] if len(sys.argv) > 2 else "extracted"

    mpq = MPQ(mpq_path)
    os.makedirs(out_dir, exist_ok=True)

    files = [
        r"data\global\excel\ItemStatCost.txt",
        r"data\global\excel\Armor.txt",
        r"data\global\excel\Weapons.txt",
        r"data\global\excel\Misc.txt",
    ]

    for f in files:
        data = mpq.read(f)
        if data:
            out = os.path.join(out_dir, os.path.basename(f))
            with open(out, "wb") as fh:
                fh.write(data)
            print(f"  {f} → {out} ({len(data)} bytes)")
        else:
            print(f"  {f}: not found")
