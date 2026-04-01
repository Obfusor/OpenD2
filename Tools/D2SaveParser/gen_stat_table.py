#!/usr/bin/env python3
"""Generate Modcode/Client/D2Client_StatTable.hpp from D2SData.hpp"""
import re, os

script_dir = os.path.dirname(os.path.abspath(__file__))
src = os.path.join(script_dir, "D2SData.hpp")
dst = os.path.join(
    script_dir, "..", "..", "Modcode", "Client", "D2Client_StatTable.hpp"
)

with open(src, "r") as f:
    content = f.read()

# Extract item bases
m = re.search(r"g_ItemBases\[\]\s*=\s*\{(.*?)\};", content, re.DOTALL)
bases_block = m.group(1)
entries = re.findall(r'\{"([^"]+)",\s*(\d+)\}', bases_block)
print(f"Found {len(entries)} item base entries")

# Extract stat array sizes
num_m = re.search(r"NUM_SAVE_BITS\s*=\s*(\d+)", content)
num_stats = int(num_m.group(1)) if num_m else 507

# Extract arrays (use dynamic size)
sb = re.search(r"g_SaveBits\[\d+\]\s*=\s*\{(.*?)\};", content, re.DOTALL)
sp = re.search(r"g_SaveParamBits\[\d+\]\s*=\s*\{(.*?)\};", content, re.DOTALL)
fs = re.search(r"g_FollowStats\[\d+\]\s*=\s*\{(.*?)\};", content, re.DOTALL)

with open(dst, "w") as out:
    out.write("#pragma once\n\n")
    out.write("//////////////////////////////////////////////////\n")
    out.write("// D2Client_StatTable.hpp\n")
    out.write("// Auto-generated from Tools/D2SaveParser/D2SData.hpp\n")
    out.write("// by gen_stat_table.py - DO NOT EDIT MANUALLY\n")
    out.write("//////////////////////////////////////////////////\n\n")
    out.write(f"#define D2CLIENT_NUM_ITEM_STATS   {num_stats}\n")
    out.write(f"#define D2CLIENT_NUM_FOLLOW_STATS {num_stats}\n\n")

    out.write("// Item stat SaveBits (index = stat ID, 0 = unused)\n")
    out.write(
        f"static const BYTE g_ClientSaveBits[{num_stats}] = {{{sb.group(1)}}};\n\n"
    )

    out.write("// Item stat SaveParamBits (index = stat ID)\n")
    out.write(
        f"static const BYTE g_ClientSaveParamBits[{num_stats}] = {{{sp.group(1)}}};\n\n"
    )

    out.write("// Follow stats count per stat ID\n")
    out.write(
        f"static const BYTE g_ClientFollowStats[{num_stats}] = {{{fs.group(1)}}};\n\n"
    )

    out.write("//////////////////////////////////////////////////\n")
    out.write("// Item base type flags lookup\n")
    out.write("// Flags: 0x01=quantity, 0x02=durability, 0x04=defense, 0x08=tome\n\n")
    out.write("struct D2ClientItemBase {\n")
    out.write("\tchar code[5]; // 4-char code + null\n")
    out.write("\tBYTE flags;\n")
    out.write("};\n\n")
    out.write("static const D2ClientItemBase g_ClientItemBases[] = {\n")
    for code, flags in entries:
        out.write(f'\t{{"{code}", {flags}}},\n')
    out.write("};\n")
    out.write(
        "static const int D2CLIENT_NUM_ITEM_BASES = sizeof(g_ClientItemBases) / sizeof(g_ClientItemBases[0]);\n\n"
    )

    out.write("// Binary search for item base flags. Returns -1 if not found.\n")
    out.write("static int D2Client_GetItemBaseFlags(const char* code)\n")
    out.write("{\n")
    out.write("\tint lo = 0, hi = D2CLIENT_NUM_ITEM_BASES - 1;\n")
    out.write("\twhile (lo <= hi)\n")
    out.write("\t{\n")
    out.write("\t\tint mid = (lo + hi) / 2;\n")
    out.write("\t\tint cmp = strncmp(code, g_ClientItemBases[mid].code, 4);\n")
    out.write("\t\tif (cmp == 0) return g_ClientItemBases[mid].flags;\n")
    out.write("\t\tif (cmp < 0) hi = mid - 1;\n")
    out.write("\t\telse lo = mid + 1;\n")
    out.write("\t}\n")
    out.write("\treturn -1;\n")
    out.write("}\n")

print(f"Generated {dst} successfully")
