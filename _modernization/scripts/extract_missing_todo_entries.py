#!/usr/bin/env -S uv run --script
# /// script
# dependencies = []
# ///

"""
Extract status information from 9 missing report files and format as TODO entries.

Reads Part 1, Part 2, and Part 3 summaries from each report and creates
standardized TODO entries in the same format as existing entries.
"""

import re
from pathlib import Path

def extract_ratings(content: str, filename: str) -> dict:
    """Extract star ratings and key metrics from report content."""

    result = {
        'filename': filename,
        'extraction_stars': 0,
        'extraction_text': '',
        'architecture_stars': 0,
        'architecture_text': '',
        'config_stars': 0,
        'config_text': '',
        'key_finding': '',
        'lines': 0,
        'functions': 0
    }

    # Extract lines and functions from header
    lines_match = re.search(r'\*\*Lines of Code\*\*: (\d+)', content)
    funcs_match = re.search(r'\*\*Function Count\*\*: (\d+)', content)

    if lines_match:
        result['lines'] = int(lines_match.group(1))
    if funcs_match:
        result['functions'] = int(funcs_match.group(1))

    # Try consolidated format first (spew.c, main.c style)
    consolidated = re.search(
        r'\*\*3-Part Analysis Results\*\*:.*?'
        r'1\. \*\*Unit Testable Extraction Potential\*\*: (⭐+ [^)]+\))',
        content,
        re.DOTALL
    )

    if consolidated:
        # Consolidated format found
        match_extract = re.search(
            r'1\. \*\*Unit Testable Extraction Potential\*\*: (⭐+) ([^)]+\))',
            content
        )
        match_arch = re.search(
            r'2\. \*\*Architectural Placement\*\*: (⭐+) ([^)]+\))',
            content
        )
        match_config = re.search(
            r'3\. \*\*Configuration Coupling\*\*: (⭐+) ([^)]+\))',
            content
        )

        if match_extract:
            result['extraction_stars'] = match_extract.group(1).count('⭐')
            result['extraction_text'] = match_extract.group(2).strip()
        if match_arch:
            result['architecture_stars'] = match_arch.group(1).count('⭐')
            result['architecture_text'] = match_arch.group(2).strip()
        if match_config:
            result['config_stars'] = match_config.group(1).count('⭐')
            result['config_text'] = match_config.group(2).strip()
    else:
        # Part-based format (most files)

        # Part 1: Extraction
        part1 = re.search(
            r'## Part 1: Unit Testable Extraction Potential.*?'
            r'### [^#]*?Summary.*?'
            r'- \*\*Rating\*\*: (⭐+) ([^)]+\))',
            content,
            re.DOTALL
        )
        if not part1:
            part1 = re.search(
                r'## Part 1: Unit Testable Extraction Potential.*?'
                r'### 1\.1 Extraction Summary.*?'
                r'\*\*TOTAL EXTRACTABLE\*\*.*?(\d+%)',
                content,
                re.DOTALL
            )
            if part1:
                # misc.c style - extract from table
                result['extraction_stars'] = 1  # Default VERY LOW
                result['extraction_text'] = f"VERY LOW ({part1.group(1)} extractable)"
        else:
            result['extraction_stars'] = part1.group(1).count('⭐')
            result['extraction_text'] = part1.group(2).strip()

        # Part 2: Architecture
        part2 = re.search(
            r'## Part 2: Architectural Placement Analysis.*?'
            r'### [^#]*?Summary.*?'
            r'- \*\*Rating\*\*: (⭐+) ([^)]+\))',
            content,
            re.DOTALL
        )
        if not part2:
            part2 = re.search(
                r'### 2\.1 Placement Summary.*?'
                r'\*\*TOTAL ARCHITECTURAL DEBT\*\*.*?(\d+).*?(\d+%)',
                content,
                re.DOTALL
            )
            if part2:
                debt_pct = int(part2.group(2).strip('%'))
                if debt_pct >= 70:
                    result['architecture_stars'] = 1
                    result['architecture_text'] = f"CRITICAL ({debt_pct}% debt)"
                elif debt_pct >= 40:
                    result['architecture_stars'] = 2
                    result['architecture_text'] = f"POOR ({debt_pct}% debt)"
                elif debt_pct >= 20:
                    result['architecture_stars'] = 3
                    result['architecture_text'] = f"MODERATE ({debt_pct}% debt)"
                else:
                    result['architecture_stars'] = 4
                    result['architecture_text'] = f"VERY GOOD ({debt_pct}% debt)"
        else:
            result['architecture_stars'] = part2.group(1).count('⭐')
            result['architecture_text'] = part2.group(2).strip()

        # Part 3: Configuration
        part3 = re.search(
            r'## Part 3: Configuration Coupling.*?'
            r'### [^#]*?Summary.*?'
            r'\*\*Verdict\*\*: (⭐+) ([^)]+)',
            content,
            re.DOTALL
        )
        if not part3:
            part3 = re.search(
                r'### 3\.1 Configuration Coupling Summary.*?'
                r'\*\*Verdict\*\*: (⭐+) ([^)]+)',
                content,
                re.DOTALL
            )

        if part3:
            result['config_stars'] = part3.group(1).count('⭐')
            result['config_text'] = part3.group(2).strip()

    # Extract key finding from Critical Findings or Executive Summary
    key_find = re.search(
        r'## Critical Findings Summary.*?'
        r'\*\*Strengths\*\*:.*?'
        r'- ⭐+ ([^\n]+)',
        content,
        re.DOTALL
    )
    if not key_find:
        key_find = re.search(
            r'## Executive Summary.*?'
            r'\*\*Critical Findings\*\*:.*?'
            r'- \*\*[^*]+\*\*: ([^\n]+)',
            content,
            re.DOTALL
        )
    if not key_find:
        # Get first bullet from executive summary
        key_find = re.search(
            r'## Executive Summary.*?'
            r'(?:- [^-\n]*)?'
            r'([^\n]{100,200})',
            content,
            re.DOTALL
        )

    if key_find:
        result['key_finding'] = key_find.group(1).strip()[:200]

    return result


def format_todo_entry(data: dict) -> str:
    """Format extracted data as TODO entry."""

    # Map star ratings to priority labels
    extract_priority = {
        0: "N/A extraction",
        1: "VERY LOW extraction priority",
        2: "LOW extraction priority",
        3: "MEDIUM extraction priority",
        4: "HIGH extraction priority",
        5: "CRITICAL extraction priority"
    }.get(data['extraction_stars'], "UNKNOWN extraction priority")

    arch_priority = {
        0: "N/A architectural",
        1: "CRITICAL architectural debt",
        2: "POOR architectural placement",
        3: "MODERATE architectural",
        4: "VERY GOOD architectural placement",
        5: "PERFECT architectural placement"
    }.get(data['architecture_stars'], "UNKNOWN architectural")

    config_priority = {
        0: "N/A config",
        1: "CATASTROPHIC config coupling",
        2: "POOR config coupling",
        3: "MODERATE config coupling",
        4: "VERY GOOD config",
        5: "EXCELLENT config"
    }.get(data['config_stars'], "UNKNOWN config")

    # Build status line
    stars_extract = '⭐' * data['extraction_stars'] if data['extraction_stars'] > 0 else '⛔ N/A'
    stars_arch = '⭐' * data['architecture_stars']
    stars_config = '⭐' * data['config_stars']

    status_line = (
        f"{stars_extract} {extract_priority}, "
        f"{stars_arch} {arch_priority}, "
        f"{stars_config} {config_priority}"
    )

    # Create TODO entry
    entry = f"""- [x] **{data['filename']}** - COMPLETE (2025-10-12)
  - Full report: `PHASE_10.02.02_refactor_report_on_{data['filename']}.md`
  - Status: {status_line}
  - Key Finding: {data['key_finding'] if data['key_finding'] else 'Analysis complete'}
"""

    return entry


def main():
    """Extract and format TODO entries for 9 missing files."""

    missing_files = [
        "spew.c", "main.c", "trade.c", "trade.h",
        "randeven.c", "commands.c", "newlogin.c",
        "newlogin.h", "npc.c", "misc.c"
    ]

    base_path = Path("/projects/conquer-4.x/_modernization/claude/reports")

    entries = []

    for filename in missing_files:
        report_path = base_path / f"PHASE_10.02.02_refactor_report_on_{filename}.md"

        if not report_path.exists():
            print(f"❌ {filename}: Report not found")
            continue

        print(f"📖 Reading {filename}...")
        content = report_path.read_text()

        print(f"🔍 Extracting ratings...")
        data = extract_ratings(content, filename)

        print(f"✅ {filename}:")
        print(f"   Extraction: {'⭐' * data['extraction_stars'] if data['extraction_stars'] > 0 else '⛔'} {data['extraction_text']}")
        print(f"   Architecture: {'⭐' * data['architecture_stars']} {data['architecture_text']}")
        print(f"   Config: {'⭐' * data['config_stars']} {data['config_text']}")
        print(f"   Lines: {data['lines']}, Functions: {data['functions']}")
        print()

        entry = format_todo_entry(data)
        entries.append(entry)

    # Write all entries to output file
    output_file = base_path / "MISSING_TODO_ENTRIES.md"
    with open(output_file, 'w') as f:
        f.write("# Missing TODO Entries\n\n")
        f.write("These entries should be added to PHASE_10.02.02_deep_analysis_todo.md\n\n")
        f.write("---\n\n")
        for entry in entries:
            f.write(entry)
            f.write("\n")

    print(f"\n✅ Wrote {len(entries)} entries to {output_file.name}")
    print(f"\nNext: Review and add these to PHASE_10.02.02_deep_analysis_todo.md")


if __name__ == "__main__":
    main()
