#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["pathlib", "argparse", "re"]
# ///

"""
analyze_function_signatures.py - Extract actual function signatures from source files

This script analyzes all C source files to extract actual function definitions
and their parameter lists, then generates proper ANSI C prototypes for data.h.

This solves the data.h extern declaration problem by using actual function
definitions rather than guessing parameter types.

Usage:
    python3 analyze_function_signatures.py --analyze
    python3 analyze_function_signatures.py --generate-prototypes > new_prototypes.h
"""

import argparse
import pathlib
import re
from collections import defaultdict
from typing import Dict, List, Tuple, Optional

class FunctionSignatureAnalyzer:
    def __init__(self):
        self.functions = {}  # function_name -> {'file': file, 'signature': signature, 'line': line}
        self.duplicates = defaultdict(list)  # Track functions defined in multiple files
        self.kr_functions = {}  # Track K&R style functions needing conversion

    def analyze_c_file(self, file_path: pathlib.Path):
        """Analyze a single C file for function definitions"""
        try:
            content = file_path.read_text(encoding='utf-8', errors='ignore')
            lines = content.split('\n')

            for i, line in enumerate(lines):
                # Look for function definitions - both ANSI and K&R style

                # ANSI style: return_type function_name(parameters) {
                ansi_match = re.match(r'^([a-zA-Z_][a-zA-Z0-9_\s\*]*)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\(\s*([^)]*)\)\s*\{?', line.strip())
                if ansi_match and not line.strip().startswith('//') and not line.strip().startswith('/*'):
                    return_type, func_name, params = ansi_match.groups()
                    return_type = return_type.strip()
                    params = params.strip()

                    # Skip obvious non-functions (struct definitions, etc.)
                    if any(keyword in return_type for keyword in ['struct', 'enum', 'typedef', 'if', 'while', 'for']):
                        continue

                    # Clean up parameters
                    if not params or params == 'void':
                        params = 'void'

                    signature = f"{return_type} {func_name}({params})"

                    self.add_function(func_name, {
                        'file': str(file_path),
                        'signature': signature,
                        'line': i + 1,
                        'style': 'ansi'
                    })

                # K&R style: function_name(param1, param2)
                # Look for this pattern followed by parameter declarations
                kr_match = re.match(r'^([a-zA-Z_][a-zA-Z0-9_]*)\s*\(\s*([^)]*)\)\s*$', line.strip())
                if kr_match and i + 1 < len(lines):
                    func_name, param_names = kr_match.groups()

                    # Check if next lines contain parameter type declarations
                    param_types = []
                    j = i + 1
                    while j < len(lines) and j < i + 10:  # Look ahead max 10 lines
                        next_line = lines[j].strip()
                        if next_line == '{':
                            # Found opening brace, this is a K&R function
                            if param_names.strip():
                                # Extract parameter types from previous lines
                                kr_params = self.extract_kr_parameters(lines[i+1:j], param_names)
                                signature = f"int {func_name}({kr_params})"  # Assume int return if not specified
                            else:
                                signature = f"int {func_name}(void)"

                            self.add_function(func_name, {
                                'file': str(file_path),
                                'signature': signature,
                                'line': i + 1,
                                'style': 'kr'
                            })
                            break
                        elif next_line == '' or next_line.startswith('/*') or next_line.startswith('//'):
                            j += 1
                            continue
                        elif re.match(r'^[a-zA-Z_][a-zA-Z0-9_\s\*]*\s+[a-zA-Z_][a-zA-Z0-9_]*\s*[,;]', next_line):
                            # This looks like a parameter type declaration
                            j += 1
                            continue
                        else:
                            # Not a K&R function
                            break

        except Exception as e:
            print(f"Warning: Could not analyze {file_path}: {e}")

    def extract_kr_parameters(self, param_lines: List[str], param_names: str) -> str:
        """Extract parameter types from K&R style declarations"""
        if not param_names.strip():
            return 'void'

        param_list = [p.strip() for p in param_names.split(',')]
        typed_params = []

        # Simple heuristic: assume int for parameters we can't determine
        for param in param_list:
            if param:
                typed_params.append(f"int {param}")

        return ', '.join(typed_params) if typed_params else 'void'

    def add_function(self, func_name: str, func_info: dict):
        """Add a function to our analysis, tracking duplicates"""
        if func_name in self.functions:
            # Track duplicate definitions
            self.duplicates[func_name].append(func_info)
            self.duplicates[func_name].append(self.functions[func_name])
        else:
            self.functions[func_name] = func_info

    def analyze_all_files(self):
        """Analyze all C files in current directory"""
        c_files = list(pathlib.Path('.').glob('*.c'))

        print(f"Analyzing {len(c_files)} C files for function definitions...")

        for c_file in c_files:
            print(f"  Analyzing {c_file}...")
            self.analyze_c_file(c_file)

        print(f"Found {len(self.functions)} unique functions")
        if self.duplicates:
            print(f"Warning: {len(self.duplicates)} functions have multiple definitions")

    def read_current_externs(self, data_h_path: str = 'data.h') -> Dict[str, str]:
        """Read current extern declarations from data.h"""
        extern_functions = {}

        try:
            content = pathlib.Path(data_h_path).read_text()
            lines = content.split('\n')

            in_extern_section = False
            for line in lines:
                if 'extern all subroutine calls' in line:
                    in_extern_section = True
                    continue
                elif in_extern_section and (line.strip().startswith('#') or line.strip() == '' or '/*' in line):
                    continue
                elif in_extern_section and line.strip().startswith('extern'):
                    # Parse extern declaration
                    match = re.match(r'extern\s+([^;]+);', line.strip())
                    if match:
                        decl = match.group(1).strip()
                        # Extract function names from declaration
                        # Handle multiple functions in one line like: extern int func1(), func2(), func3();
                        if '(' in decl:
                            parts = decl.split('(')[0].strip().split()
                            if len(parts) >= 2:
                                return_type = ' '.join(parts[:-1])
                                func_part = parts[-1]

                                # Handle comma-separated function names
                                if ',' in decl:
                                    # Multiple functions in one declaration
                                    func_names = re.findall(r'([a-zA-Z_][a-zA-Z0-9_]*)\s*\([^)]*\)', decl)
                                    for func_name in func_names:
                                        extern_functions[func_name] = f"{return_type} {func_name}"
                                else:
                                    # Single function
                                    func_match = re.search(r'([a-zA-Z_][a-zA-Z0-9_]*)\s*\(', decl)
                                    if func_match:
                                        func_name = func_match.group(1)
                                        extern_functions[func_name] = decl
                elif in_extern_section and line.strip() and not line.strip().startswith('extern'):
                    # End of extern section
                    break

        except Exception as e:
            print(f"Warning: Could not read current externs from {data_h_path}: {e}")

        return extern_functions

    def generate_prototypes(self) -> str:
        """Generate proper ANSI C prototypes based on analysis"""
        output = []
        output.append("/* Modernized function prototypes - Generated by analyze_function_signatures.py */")
        output.append("")

        # Read current extern declarations to see what's expected
        current_externs = self.read_current_externs()

        # Group functions by return type for cleaner output
        by_return_type = defaultdict(list)

        for func_name, func_info in sorted(self.functions.items()):
            signature = func_info['signature']

            # Extract return type
            match = re.match(r'^([^(]+?)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\(', signature)
            if match:
                return_type, name = match.groups()
                return_type = return_type.strip()
                by_return_type[return_type].append((name, signature, func_info))

        # Generate extern declarations
        for return_type in sorted(by_return_type.keys()):
            functions = by_return_type[return_type]

            output.append(f"/* {return_type} functions */")

            for func_name, signature, func_info in functions:
                # Check if this function was in the original extern list
                if func_name in current_externs:
                    extern_line = f"extern {signature};"
                    source_comment = f"  /* from {func_info['file']}:{func_info['line']} */"
                    output.append(extern_line + source_comment)

            output.append("")

        # Report functions that were in extern list but not found in source
        missing_functions = set(current_externs.keys()) - set(self.functions.keys())
        if missing_functions:
            output.append("/* Functions declared in data.h but not found in source files */")
            for func_name in sorted(missing_functions):
                output.append(f"/* MISSING: {func_name} - {current_externs[func_name]} */")
            output.append("")

        # Report duplicates
        if self.duplicates:
            output.append("/* Functions with multiple definitions (need review) */")
            for func_name, definitions in self.duplicates.items():
                output.append(f"/* DUPLICATE: {func_name} found in: */")
                for defn in definitions:
                    output.append(f"/*   {defn['file']}:{defn['line']} - {defn['signature']} */")
            output.append("")

        return '\n'.join(output)

    def generate_report(self) -> str:
        """Generate analysis report"""
        report = []
        report.append("# Function Signature Analysis Report")
        report.append("")
        report.append(f"**Total Functions Found**: {len(self.functions)}")
        report.append(f"**Functions with Duplicates**: {len(self.duplicates)}")
        report.append("")

        # Current extern analysis
        current_externs = self.read_current_externs()
        report.append(f"**Current extern declarations**: {len(current_externs)}")

        found_in_source = set(self.functions.keys()) & set(current_externs.keys())
        missing_from_source = set(current_externs.keys()) - set(self.functions.keys())
        new_functions = set(self.functions.keys()) - set(current_externs.keys())

        report.append(f"**Functions with source found**: {len(found_in_source)}")
        report.append(f"**Functions missing from source**: {len(missing_from_source)}")
        report.append(f"**New functions not in extern list**: {len(new_functions)}")
        report.append("")

        if missing_from_source:
            report.append("## Functions in data.h but not found in source:")
            for func in sorted(missing_from_source):
                report.append(f"- `{func}`: {current_externs[func]}")
            report.append("")

        if self.duplicates:
            report.append("## Functions with multiple definitions:")
            for func_name, definitions in self.duplicates.items():
                report.append(f"### {func_name}")
                for defn in definitions:
                    report.append(f"- {defn['file']}:{defn['line']} - `{defn['signature']}`")
            report.append("")

        return '\n'.join(report)

def main():
    parser = argparse.ArgumentParser(description='Analyze function signatures for data.h modernization')

    parser.add_argument('--analyze', action='store_true', help='Analyze all C files')
    parser.add_argument('--generate-prototypes', action='store_true', help='Generate modern prototypes')
    parser.add_argument('--report', action='store_true', help='Generate analysis report')
    parser.add_argument('--output', help='Output file for results')

    args = parser.parse_args()

    analyzer = FunctionSignatureAnalyzer()

    if args.analyze or args.generate_prototypes or args.report:
        analyzer.analyze_all_files()

    if args.generate_prototypes:
        prototypes = analyzer.generate_prototypes()
        if args.output:
            pathlib.Path(args.output).write_text(prototypes)
            print(f"Generated prototypes written to {args.output}")
        else:
            print(prototypes)

    elif args.report:
        report = analyzer.generate_report()
        if args.output:
            pathlib.Path(args.output).write_text(report)
            print(f"Analysis report written to {args.output}")
        else:
            print(report)

    elif args.analyze:
        print("Analysis complete. Use --report or --generate-prototypes to see results.")

    else:
        print("Specify --analyze, --generate-prototypes, or --report")

if __name__ == '__main__':
    main()