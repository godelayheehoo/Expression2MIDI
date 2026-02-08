import re
from pathlib import Path

parent_dir = Path(__file__).parent
HEADER_FILENAME = parent_dir / "../include/cc_definitions.h"
FOLDER_PATH = parent_dir / "." 


def parse_xml_to_cc_array(xml_path: Path):
    """
    Parses a single XML manually using regex to handle numeric attributes.
    Returns (instrument_name, list of (cc_number, label))
    """
    instrument_name = xml_path.stem
    cc_list = []

    with open(xml_path, "r", encoding="utf-8") as f:
        content = f.read()

    # Find the ccLabels tag content
    match = re.search(r"<ccLabels\s+(.*?)\s*/>", content, re.DOTALL)
    if not match:
        return instrument_name, []

    attrs = match.group(1)

    # Extract all number="label" pairs
    pattern = re.compile(r'(\d+)\s*=\s*"([^"]*)"')
    for cc_str, label in pattern.findall(attrs):
        cc_num = int(cc_str)
        if label.strip() == "":
            label = None
        cc_list.append((cc_num, label))

    return instrument_name, cc_list


def generate_header(instrument_ccs):
    lines = [
        "#pragma once",
        "#include <stdint.h>",
        "",
        "struct CCLabel {",
        "    uint8_t cc;",
        "    const char* label;",
        "};",
        ""
    ]

    # Generate the arrays for each instrument
    for name, cc_list in instrument_ccs:
        safe_name = name.replace("-", "_")
        lines.append(f"// {name}")
        lines.append(f"const CCLabel {safe_name}[] = {{")
        for cc, label in cc_list:
            if label is None:
                lines.append(f"    {{ {cc}, nullptr }},")
            else:
                label_escaped = label.replace('"', '\\"')
                lines.append(f'    {{ {cc}, "{label_escaped}" }},')
        lines.append("    { 0, nullptr }")
        lines.append("};\n")

    # Generate the registry array
    lines.append("struct InstrumentCCs {")
    lines.append("    const char* name;")
    lines.append("    const CCLabel* ccArray;")
    lines.append("};\n")

    lines.append("const InstrumentCCs allInstruments[] = {")
    for name, _ in instrument_ccs:
        safe_name = name.replace("-", "_")
        lines.append(f'    {{ "{name}", {safe_name} }},')
    lines.append("    { nullptr, nullptr } // sentinel")
    lines.append("};\n")

    return "\n".join(lines)


def process_folder(folder_path: str, header_filename: str = HEADER_FILENAME):
    folder = Path(folder_path)
    xml_files = list(folder.glob("*.xml"))

    if not xml_files:
        print(f"No XML files found in {folder_path}")
        return

    instrument_ccs = []
    for xml_file in xml_files:
        name, cc_list = parse_xml_to_cc_array(xml_file)
        instrument_ccs.append((name, cc_list))

    header_content = generate_header(instrument_ccs)
    with open(header_filename, "w", encoding="utf-8") as f:
        f.write(header_content)
    print(f"Wrote {header_filename} with {len(instrument_ccs)} instruments.")


# ----------------- Run -----------------
if __name__ == "__main__":
    process_folder(FOLDER_PATH)
