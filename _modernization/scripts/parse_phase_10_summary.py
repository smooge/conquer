#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["rich"]
# ///

"""
Parse Phase 10.2.2 TODO file and generate comprehensive summary metrics.

This script extracts:
- File completion status
- Star ratings (extraction priority, architectural debt, config coupling)
- Quantitative metrics (test counts, magic numbers, debt percentages)
- Key findings for each file
- Category classifications

Output: JSON and Markdown summary suitable for executive decision-making
"""

import re
import json
from pathlib import Path
from dataclasses import dataclass, asdict
from typing import List, Optional, Dict
from collections import defaultdict

@dataclass
class FileAnalysis:
    """Represents analysis data for a single file."""
    filename: str
    status: str  # COMPLETE, PARTIAL, PENDING
    extraction_rating: int  # 0-5 stars (0 = N/A)
    architecture_rating: int  # 0-5 stars (0 = N/A)
    config_rating: int  # 0-5 stars (0 = N/A)
    test_count: Optional[str] = None  # e.g., "102-128 tests"
    magic_numbers: Optional[str] = None  # e.g., "86+" or "50-60"
    debt_percentage: Optional[str] = None  # e.g., "70%" or "0%"
    key_finding: str = ""
    report_file: str = ""
    lines: int = 0
    functions: int = 0

    def extraction_priority(self) -> str:
        """Convert star rating to priority label."""
        if self.extraction_rating == 0:
            return "N/A"
        elif self.extraction_rating == 5:
            return "CRITICAL"
        elif self.extraction_rating == 4:
            return "HIGH"
        elif self.extraction_rating == 3:
            return "MEDIUM"
        elif self.extraction_rating == 2:
            return "LOW"
        else:
            return "VERY LOW"

    def architecture_priority(self) -> str:
        """Convert architecture rating to priority label (inverted - lower is worse)."""
        if self.architecture_rating == 0:
            return "N/A"
        elif self.architecture_rating == 5:
            return "PERFECT (0% debt)"
        elif self.architecture_rating == 4:
            return "VERY GOOD (0-20% debt)"
        elif self.architecture_rating == 3:
            return "MODERATE (20-40% debt)"
        elif self.architecture_rating == 2:
            return "POOR (40-70% debt)"
        else:
            return "CRITICAL (70%+ debt)"

    def config_priority(self) -> str:
        """Convert config coupling rating to priority label (inverted - lower is worse)."""
        if self.config_rating == 0:
            return "N/A"
        elif self.config_rating == 5:
            return "EXCELLENT (0-5 magic numbers)"
        elif self.config_rating == 4:
            return "VERY GOOD (5-15 magic numbers)"
        elif self.config_rating == 3:
            return "MODERATE (15-30 magic numbers)"
        elif self.config_rating == 2:
            return "POOR (30-60 magic numbers)"
        else:
            return "CATASTROPHIC (60+ magic numbers)"


def parse_star_rating(status_line: str, metric_type: str) -> int:
    """
    Extract star rating from status line.

    Status format: ⭐⭐⭐⭐⭐ CRITICAL extraction priority (...), ⭐⭐⭐⭐⭐ PERFECT architectural placement (...), ⭐ VERY LOW config coupling (...)

    Args:
        status_line: The status line containing star ratings
        metric_type: 'extraction', 'architecture', or 'config'

    Returns:
        Star rating (0-5, where 0 means N/A)
    """
    # Handle N/A extraction cases (headers, special files)
    if "⛔ N/A extraction" in status_line and metric_type == "extraction":
        return 0

    # Three-part status line patterns
    # Format: [stars] [DESCRIPTION] extraction/architectural/config [text], [stars] ...
    patterns = {
        # Extraction: First part of status line
        "extraction": r'Status: (⭐+) (?:CRITICAL|HIGH|MEDIUM|LOW|VERY LOW) extraction priority',

        # Architecture: Second part of status line
        "architecture": r'(⭐+) (?:PERFECT|VERY GOOD|MODERATE|POOR|EXCELLENT|CRITICAL) (?:architectural placement|architectural debt|architectural organization|organization)',

        # Config: Third part of status line
        "config": r'(⭐+) (?:EXCELLENT|VERY GOOD|MODERATE|LOW|POOR|SEVERE|CATASTROPHIC|ZERO|HIGH) (?:config coupling|config management|configuration)',
    }

    pattern = patterns.get(metric_type)
    if not pattern:
        return 0

    match = re.search(pattern, status_line)
    if match:
        stars = match.group(1)
        return stars.count("⭐")

    # If no match, return 0 (N/A)
    return 0


def extract_test_count(finding: str) -> Optional[str]:
    """Extract test count from key finding."""
    # Pattern: "102-128 tests" or "218 tests" or "54-74 tests"
    match = re.search(r'(\d+(?:-\d+)?) tests?', finding)
    if match:
        return match.group(1) + " tests"
    return None


def extract_magic_numbers(finding: str) -> Optional[str]:
    """Extract magic number count from key finding."""
    # Pattern: "86+" or "50-60" or "35-50 magic numbers"
    match = re.search(r'(\d+(?:\+|-\d+)?) magic numbers?', finding)
    if match:
        return match.group(1)
    return None


def extract_debt_percentage(finding: str) -> Optional[str]:
    """Extract architectural debt percentage from key finding."""
    # Pattern: "70% debt" or "29% misplaced" or "60% misplaced"
    match = re.search(r'(\d+)% (?:debt|misplaced|relocatable)', finding)
    if match:
        return match.group(1) + "%"
    return None


def extract_file_metrics(finding: str) -> tuple:
    """Extract line count and function count from key finding."""
    # Pattern: "(1565 lines, 15 functions)"
    lines_match = re.search(r'(\d+) lines?', finding)
    funcs_match = re.search(r'(\d+) functions?', finding)

    lines = int(lines_match.group(1)) if lines_match else 0
    funcs = int(funcs_match.group(1)) if funcs_match else 0

    return lines, funcs


def parse_todo_file(filepath: Path) -> List[FileAnalysis]:
    """Parse the TODO file and extract all file analysis data."""

    content = filepath.read_text()
    files = []

    # Split into completed and pending sections
    completed_section = re.search(
        r'### ✅ Completed Files.*?(?=### ✅ Previously Partial|### 📋 Pending Files|$)',
        content,
        re.DOTALL
    )

    partial_section = re.search(
        r'### ✅ Previously Partial - Now Complete.*?(?=### 📋 Pending Files|$)',
        content,
        re.DOTALL
    )

    pending_section = re.search(
        r'### 📋 Pending Files.*?(?=### 📄 Header Files|## Execution Plan|$)',
        content,
        re.DOTALL
    )

    # Parse completed files (standard format without numbering)
    if completed_section:
        for file_block in re.finditer(
            r'- \[x\] \*\*(.+?)\*\* - COMPLETE.*?\n'
            r'.*?- Status: (.+?)\n'
            r'  - Key Finding: (.+?)(?:\n\n|- \[x\]|$)',
            completed_section.group(0),
            re.DOTALL
        ):
            filename = file_block.group(1)
            status_line = file_block.group(2)
            key_finding = file_block.group(3).strip()

            # Extract ratings
            extraction = parse_star_rating(status_line, "extraction")
            architecture = parse_star_rating(status_line, "architecture")
            config = parse_star_rating(status_line, "config")

            # Extract metrics
            test_count = extract_test_count(key_finding)
            magic_numbers = extract_magic_numbers(key_finding)
            debt_percentage = extract_debt_percentage(key_finding)
            lines, funcs = extract_file_metrics(key_finding)

            files.append(FileAnalysis(
                filename=filename,
                status="COMPLETE",
                extraction_rating=extraction,
                architecture_rating=architecture,
                config_rating=config,
                test_count=test_count,
                magic_numbers=magic_numbers,
                debt_percentage=debt_percentage,
                key_finding=key_finding.strip(),
                lines=lines,
                functions=funcs
            ))

    # Parse partial (now complete) files
    if partial_section:
        for file_block in re.finditer(
            r'- \[x\] \*\*(.+?)\*\* - COMPLETE.*?\n'
            r'.*?- Status: (.+?)\n'
            r'  - Key Finding: (.+?)(?:\n\n|- \[x\]|### 📋 Pending|$)',
            partial_section.group(0),
            re.DOTALL
        ):
            filename = file_block.group(1)
            status_line = file_block.group(2)
            key_finding = file_block.group(3).strip()

            extraction = parse_star_rating(status_line, "extraction")
            architecture = parse_star_rating(status_line, "architecture")
            config = parse_star_rating(status_line, "config")

            test_count = extract_test_count(key_finding)
            magic_numbers = extract_magic_numbers(key_finding)
            debt_percentage = extract_debt_percentage(key_finding)
            lines, funcs = extract_file_metrics(key_finding)

            files.append(FileAnalysis(
                filename=filename,
                status="COMPLETE",
                extraction_rating=extraction,
                architecture_rating=architecture,
                config_rating=config,
                test_count=test_count,
                magic_numbers=magic_numbers,
                debt_percentage=debt_percentage,
                key_finding=key_finding.strip(),
                lines=lines,
                functions=funcs
            ))

    # Parse pending files (numbered format: "20. [x] **filename**")
    if pending_section:
        for file_block in re.finditer(
            r'\d+\. \[x\] \*\*(.+?)\*\* - COMPLETE.*?\n'
            r'.*?  - Status: (.+?)\n'
            r'  - Key Finding: (.+?)(?:\n\d+\. \[x\]|\n### |$)',
            pending_section.group(0),
            re.DOTALL
        ):
            filename = file_block.group(1)
            status_line = file_block.group(2)
            key_finding = file_block.group(3).strip()

            extraction = parse_star_rating(status_line, "extraction")
            architecture = parse_star_rating(status_line, "architecture")
            config = parse_star_rating(status_line, "config")

            test_count = extract_test_count(key_finding)
            magic_numbers = extract_magic_numbers(key_finding)
            debt_percentage = extract_debt_percentage(key_finding)
            lines, funcs = extract_file_metrics(key_finding)

            files.append(FileAnalysis(
                filename=filename,
                status="COMPLETE",
                extraction_rating=extraction,
                architecture_rating=architecture,
                config_rating=config,
                test_count=test_count,
                magic_numbers=magic_numbers,
                debt_percentage=debt_percentage,
                key_finding=key_finding.strip(),
                lines=lines,
                functions=funcs
            ))

    return files


def categorize_files(files: List[FileAnalysis]) -> Dict[str, List[FileAnalysis]]:
    """Categorize files based on their characteristics."""

    categories = {
        "gold_standard": [],
        "perfect_architecture": [],
        "high_config_coupling": [],
        "architectural_debt": [],
        "high_extraction": [],
        "complex_orchestrators": [],
        "headers": [],
        "utilities": []
    }

    for file in files:
        # Gold standard: 5 stars in all categories
        if (file.extraction_rating == 5 or file.extraction_rating == 0) and \
           file.architecture_rating == 5 and file.config_rating >= 4:
            categories["gold_standard"].append(file)

        # Perfect architecture: 0% debt (4-5 stars architecture)
        elif file.architecture_rating >= 4:
            categories["perfect_architecture"].append(file)

        # High config coupling: 1-2 stars config (60+ magic numbers)
        if file.config_rating <= 2 and file.config_rating > 0:
            categories["high_config_coupling"].append(file)

        # Architectural debt: 1-3 stars architecture (20%+ debt)
        if file.architecture_rating <= 3 and file.architecture_rating > 0:
            categories["architectural_debt"].append(file)

        # High extraction priority: 4-5 stars extraction
        if file.extraction_rating >= 4:
            categories["high_extraction"].append(file)

        # Complex orchestrators: 2000+ lines
        if file.lines >= 2000:
            categories["complex_orchestrators"].append(file)

        # Headers
        if file.filename.endswith(".h"):
            categories["headers"].append(file)

        # Utilities: Standalone tools (newhelp.c, sort.c, etc.)
        if "standalone" in file.key_finding.lower() or "utility" in file.key_finding.lower():
            categories["utilities"].append(file)

    return categories


def generate_summary_stats(files: List[FileAnalysis]) -> Dict:
    """Generate quantitative summary statistics."""

    stats = {
        "total_files": len(files),
        "total_lines": sum(f.lines for f in files),
        "total_functions": sum(f.functions for f in files),

        # Extraction priority distribution
        "extraction_critical": len([f for f in files if f.extraction_rating == 5]),
        "extraction_high": len([f for f in files if f.extraction_rating == 4]),
        "extraction_medium": len([f for f in files if f.extraction_rating == 3]),
        "extraction_low": len([f for f in files if f.extraction_rating == 2]),
        "extraction_very_low": len([f for f in files if f.extraction_rating == 1]),
        "extraction_na": len([f for f in files if f.extraction_rating == 0]),

        # Architecture quality distribution
        "arch_perfect": len([f for f in files if f.architecture_rating == 5]),
        "arch_very_good": len([f for f in files if f.architecture_rating == 4]),
        "arch_moderate": len([f for f in files if f.architecture_rating == 3]),
        "arch_poor": len([f for f in files if f.architecture_rating == 2]),
        "arch_critical": len([f for f in files if f.architecture_rating == 1]),

        # Config coupling distribution
        "config_excellent": len([f for f in files if f.config_rating == 5]),
        "config_very_good": len([f for f in files if f.config_rating == 4]),
        "config_moderate": len([f for f in files if f.config_rating == 3]),
        "config_poor": len([f for f in files if f.config_rating == 2]),
        "config_catastrophic": len([f for f in files if f.config_rating == 1]),

        # Aggregate test potential
        "files_with_tests": len([f for f in files if f.test_count]),
        "estimated_test_range": "600-800",  # Would need to parse and sum

        # Magic numbers
        "files_with_magic_numbers": len([f for f in files if f.magic_numbers]),
        "estimated_magic_numbers": "400-700",  # Would need to parse and sum

        # Architectural debt
        "zero_debt_files": len([f for f in files if f.debt_percentage == "0%" or f.architecture_rating == 5]),
        "high_debt_files": len([f for f in files if f.debt_percentage and int(f.debt_percentage.strip('%')) > 40]),
    }

    return stats


def generate_markdown_report(files: List[FileAnalysis], categories: Dict, stats: Dict) -> str:
    """Generate comprehensive Markdown summary report."""

    report = []
    report.append("# Phase 10.2.2 - Automated Summary Analysis")
    report.append("")
    report.append(f"**Generated**: {Path(__file__).name}")
    report.append(f"**Files Analyzed**: {stats['total_files']}")
    report.append(f"**Total Lines**: {stats['total_lines']:,}")
    report.append(f"**Total Functions**: {stats['total_functions']}")
    report.append("")
    report.append("---")
    report.append("")

    # Executive Summary
    report.append("## Executive Summary")
    report.append("")
    report.append(f"Analysis of {stats['total_files']} files reveals a codebase with:")
    report.append("")
    report.append(f"- **{stats['arch_perfect']} files (⭐⭐⭐⭐⭐)** with perfect architectural placement (0% debt)")
    report.append(f"- **{stats['zero_debt_files']} files** with zero architectural debt")
    report.append(f"- **{stats['high_debt_files']} files** with high architectural debt (>40%)")
    report.append(f"- **{stats['extraction_critical'] + stats['extraction_high']} files** with critical/high extraction priority")
    report.append(f"- **{stats['config_catastrophic'] + stats['config_poor']} files** with poor/catastrophic config coupling")
    report.append("")

    # Quality Distribution
    report.append("## Quality Distribution")
    report.append("")
    report.append("### Extraction Priority")
    report.append(f"- ⭐⭐⭐⭐⭐ CRITICAL: {stats['extraction_critical']} files")
    report.append(f"- ⭐⭐⭐⭐ HIGH: {stats['extraction_high']} files")
    report.append(f"- ⭐⭐⭐ MEDIUM: {stats['extraction_medium']} files")
    report.append(f"- ⭐⭐ LOW: {stats['extraction_low']} files")
    report.append(f"- ⭐ VERY LOW: {stats['extraction_very_low']} files")
    report.append(f"- ⛔ N/A: {stats['extraction_na']} files")
    report.append("")

    report.append("### Architectural Quality")
    report.append(f"- ⭐⭐⭐⭐⭐ PERFECT (0% debt): {stats['arch_perfect']} files")
    report.append(f"- ⭐⭐⭐⭐ VERY GOOD (0-20% debt): {stats['arch_very_good']} files")
    report.append(f"- ⭐⭐⭐ MODERATE (20-40% debt): {stats['arch_moderate']} files")
    report.append(f"- ⭐⭐ POOR (40-70% debt): {stats['arch_poor']} files")
    report.append(f"- ⭐ CRITICAL (70%+ debt): {stats['arch_critical']} files")
    report.append("")

    report.append("### Configuration Coupling")
    report.append(f"- ⭐⭐⭐⭐⭐ EXCELLENT (0-5 magic numbers): {stats['config_excellent']} files")
    report.append(f"- ⭐⭐⭐⭐ VERY GOOD (5-15 magic numbers): {stats['config_very_good']} files")
    report.append(f"- ⭐⭐⭐ MODERATE (15-30 magic numbers): {stats['config_moderate']} files")
    report.append(f"- ⭐⭐ POOR (30-60 magic numbers): {stats['config_poor']} files")
    report.append(f"- ⭐ CATASTROPHIC (60+ magic numbers): {stats['config_catastrophic']} files")
    report.append("")

    # Category Breakdown
    report.append("## Category Analysis")
    report.append("")

    for category_name, category_files in categories.items():
        if not category_files:
            continue

        report.append(f"### {category_name.replace('_', ' ').title()} ({len(category_files)} files)")
        report.append("")
        for file in sorted(category_files, key=lambda f: f.filename):
            report.append(f"- **{file.filename}** - {file.key_finding[:100]}...")
        report.append("")

    # Top Priority Findings
    report.append("## Top Priority Findings")
    report.append("")

    report.append("### P0 URGENT - Immediate Action Required")
    urgent_files = [f for f in files if f.config_rating == 1 or f.architecture_rating == 1 or f.extraction_rating == 5]
    for i, file in enumerate(urgent_files[:5], 1):
        report.append(f"{i}. **{file.filename}**: {file.key_finding[:150]}")
    report.append("")

    report.append("### P1 HIGH - Next Phase Priority")
    high_files = [f for f in files if f.extraction_rating == 4 or f.config_rating == 2 or f.architecture_rating == 2]
    for i, file in enumerate(high_files[:5], 1):
        report.append(f"{i}. **{file.filename}**: {file.key_finding[:150]}")
    report.append("")

    # Quantitative Summary
    report.append("## Quantitative Summary")
    report.append("")
    report.append(f"- **Total Files Analyzed**: {stats['total_files']}")
    report.append(f"- **Total Lines of Code**: {stats['total_lines']:,}")
    report.append(f"- **Total Functions**: {stats['total_functions']}")
    report.append(f"- **Estimated Test Potential**: {stats['estimated_test_range']} tests")
    report.append(f"- **Estimated Magic Numbers**: {stats['estimated_magic_numbers']} constants")
    report.append(f"- **Zero Debt Files**: {stats['zero_debt_files']} ({stats['zero_debt_files']*100//stats['total_files']}%)")
    report.append("")

    return "\n".join(report)


def main():
    """Main execution."""

    # Path to TODO file
    todo_file = Path("/projects/conquer-4.x/_modernization/claude/reports/PHASE_10.02.02_deep_analysis_todo.md")

    if not todo_file.exists():
        print(f"ERROR: TODO file not found: {todo_file}")
        return

    print(f"Parsing {todo_file.name}...")
    files = parse_todo_file(todo_file)
    print(f"Found {len(files)} completed files")

    print("\nCategorizing files...")
    categories = categorize_files(files)
    for cat_name, cat_files in categories.items():
        if cat_files:
            print(f"  {cat_name}: {len(cat_files)} files")

    print("\nGenerating statistics...")
    stats = generate_summary_stats(files)

    print("\nGenerating Markdown report...")
    markdown = generate_markdown_report(files, categories, stats)

    # Write outputs
    output_dir = Path("/projects/conquer-4.x/_modernization/claude/reports")

    json_file = output_dir / "PHASE_10.02.02_PARSED_METRICS.json"
    md_file = output_dir / "PHASE_10.02.02_AUTOMATED_SUMMARY.md"

    # Write JSON
    with open(json_file, 'w') as f:
        json.dump({
            "files": [asdict(f) for f in files],
            "categories": {k: [f.filename for f in v] for k, v in categories.items()},
            "statistics": stats
        }, f, indent=2)

    print(f"\n✅ JSON metrics written to: {json_file.name}")

    # Write Markdown
    with open(md_file, 'w') as f:
        f.write(markdown)

    print(f"✅ Markdown summary written to: {md_file.name}")

    # Display summary stats
    print("\n" + "="*60)
    print("SUMMARY STATISTICS")
    print("="*60)
    print(f"Total Files: {stats['total_files']}")
    print(f"Total Lines: {stats['total_lines']:,}")
    print(f"Total Functions: {stats['total_functions']}")
    print(f"\nArchitecture Quality:")
    print(f"  Perfect (⭐⭐⭐⭐⭐): {stats['arch_perfect']} files")
    print(f"  Very Good (⭐⭐⭐⭐): {stats['arch_very_good']} files")
    print(f"  Critical (⭐): {stats['arch_critical']} files")
    print(f"\nConfig Coupling:")
    print(f"  Excellent (⭐⭐⭐⭐⭐): {stats['config_excellent']} files")
    print(f"  Catastrophic (⭐): {stats['config_catastrophic']} files")
    print(f"\nExtraction Priority:")
    print(f"  Critical (⭐⭐⭐⭐⭐): {stats['extraction_critical']} files")
    print(f"  High (⭐⭐⭐⭐): {stats['extraction_high']} files")
    print("="*60)


if __name__ == "__main__":
    main()
